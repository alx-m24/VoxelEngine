# VoxelEngine

A voxel engine written in C++. Design notes for the GPU SVO system and the per-voxel deferred shadow pipeline built on top of it.

---

# Table of Contents

- [GPU Sparse Voxel Octree (SVO) Design](#gpu-sparse-voxel-octree-svo-design)
  - [Goals](#goals)
  - [High-Level Architecture](#high-level-architecture)
  - [GPU Memory Layout](#gpu-memory-layout)
  - [Static Tree Layout](#static-tree-layout)
  - [Deepest Level = 1 Voxel](#deepest-level--1-voxel)
  - [Node Structure](#node-structure)
  - [GPU Construction](#gpu-construction)
    - [Pass 1 — Deepest Level](#pass-1--deepest-level)
    - [Higher Levels](#higher-levels)
  - [Dispatch Strategy](#dispatch-strategy)
  - [Streaming](#streaming)
  - [Double Buffering (Optional)](#double-buffering-optional)
  - [Ray Traversal](#ray-traversal)
  - [Why Static Memory?](#why-static-memory)
  - [Advantages](#advantages)
  - [Future Improvements](#future-improvements)
- [Per-Voxel Deferred Shadow Pipeline](#per-voxel-deferred-shadow-pipeline)
  - [Problem](#problem)
  - [Core Fix](#core-fix-mandatory-regardless-of-approach)
  - [Why the Static Tree Layout Matters Here](#why-the-static-tree-layout-matters-here)
  - [Three-Pass Pipeline](#three-pass-pipeline)
    - [Pass 1 — Primary Ray March](#pass-1--primary-ray-march-2d-dispatch-per-pixel)
    - [Pass 2 — Shadow Calculation](#pass-2--shadow-calculation-dispatch-over-nodes-not-pixels)
    - [Pass 3 — Composite / Shade](#pass-3--composite--shade-2d-dispatch-per-pixel-again)
  - [Position Recovery](#position-recovery-pure-arithmetic-no-extra-fields)
  - [Key Clarifications](#key-clarifications-things-that-felt-confusing-but-arent)
  - [Open Decision](#open-decision)
  - [Next Steps / Still Open](#next-steps--still-open)

# GPU Sparse Voxel Octree (SVO) Design

## Goals

- GPU-friendly SVO generation.
- Simple architecture with predictable memory.
- Chunk streaming support.
- No dynamic GPU allocation.
- Fast ray traversal.
- Future-proof for LOD.

## High-Level Architecture

The world is divided into chunks. Each chunk owns one SVO.

```
World
├── Chunk (0,0)
├── Chunk (1,0)
├── Chunk (2,0)
└── ...
```

Only nearby chunks are streamed into GPU memory. Each chunk has metadata:

```cpp
struct ChunkInfo
{
    uint rootIndex;      // Beginning of this chunk's SVO
    ivec3 worldPosition;
    uint maxDepth;       // Maximum traversal depth (LOD)
};
```

## GPU Memory Layout

A single SSBO stores every loaded chunk:

```
+----------------------------------------------------------------+
| Chunk 0 | Chunk 1 | Chunk 2 | Chunk 3 | Chunk 4 | ...          |
+----------------------------------------------------------------+
```

Each chunk occupies one contiguous memory block.

| Benefit           | Explanation                                          |
| ------------------ | ----------------------------------------------------- |
| Fast uploads       | One `glBufferSubData()` call uploads the entire SVO.  |
| Cache friendly     | Nodes from the same chunk remain close together.      |
| Easy streaming     | Chunks are uploaded and removed independently.         |
| Simple addressing  | Only the root index needs updating.                    |

## Static Tree Layout

Instead of dynamically allocating nodes, every possible node exists.

```
Depth 0 :      1
Depth 1 :      8
Depth 2 :     64
Depth 3 :    512
Depth 4 :   4096
...
```

This means:

- No pointers
- No GPU allocator
- No atomics
- Child indices are computed mathematically

## Deepest Level = 1 Voxel

The tree's deepest level maps 1:1 to a single voxel — this is the standard definition of an SVO, not a variant. Combined with the static/complete layout above, this means the deepest level alone contains `8^maxDepth` nodes, and since every shallower level also fully exists, the whole tree costs roughly that number plus another ~1/7th on top (geometric series over the levels above).

This is a deliberate trade of memory for speed:

- **What we pay**: full per-chunk memory scales fast with depth (32³ voxels at depth 5, 64³ at depth 6, 128³ at depth 7, and so on). At roughly 20-24 bytes per node, a chunk pushed all the way to 1-voxel depth can run into tens of MB by itself.
- **What we get**: zero atomics, zero GPU allocator, zero pointer-chasing — a node's children, parent, and world position are all recoverable by pure `(idx-1)/8` / `(idx-1)%8` arithmetic (see the Per-Voxel Deferred Shadow Pipeline section below, which depends directly on this property). This is the same trade the "Why Static Memory?" section calls out, just made concrete at the leaf level specifically.

**Why this is affordable in practice:**

- **LOD caps the worst case.** `ChunkInfo.maxDepth` is per-chunk, so only *near* chunks are ever pushed to full 1-voxel depth. Distant chunks stop at a shallower depth, and memory drops by roughly 8x per level dropped — so the real worst case is a handful of near chunks at full depth plus many distant chunks at progressively cheaper depths, not every resident chunk paying the full cost.
- **Only close chunks are fully resident at all.** Streaming means far chunks aren't just shallower, they may not be loaded yet in the first place.
- **Modern GPU VRAM comfortably absorbs this.** Tens of MB per near chunk, times a small number of near chunks, is trivial next to several GB of typical VRAM — SSBOs in the hundreds of MB to low GB range are routine for open-world/voxel titles today.

Worth pinning down empirically once render distance is chosen: pick a render distance (in chunks), a full-depth chunk size, and a depth-falloff curve, then compute actual worst-case resident memory. This is a quick calculation and gives a concrete answer rather than an assumption.

## Node Structure

Current design:

```cpp
struct Node
{
    uint firstChild;

    vec3 averageColor;

    bool occupied;      // Region contains at least one voxel
    bool terminal;      // Stop traversal here
};
```

Possible future additions:

| Field      | Purpose                 |
| ---------- | ----------------------- |
| voxelCount | Better colour averaging |
| childMask  | Faster traversal        |
| normal     | Lighting                |
| materialID | Material lookup         |
| emission   | Lighting                |

## GPU Construction

Generation happens entirely on the GPU using compute shaders.

### Pass 1 — Deepest Level

Each invocation corresponds to exactly one node.

```
Thread
    ↓
Compute voxel region
    ↓
Contains voxel?
    ↓
Compute average colour
    ↓
Write node
```

```cpp
node.occupied = ContainsVoxel(nodeBounds);
node.terminal = true;

if(node.occupied)
{
    node.averageColor = ComputeAverageColor(nodeBounds);
}
```

No synchronization is required because every thread writes to a unique node.

### Higher Levels

Generation proceeds bottom-up.

```
Leaves
   ↑
Parents
   ↑
Grandparents
   ↑
Root
```

Each parent simply inspects its eight children:

```cpp
bool occupied = false;

vec3 colour = vec3(0.0);
uint count = 0;

for(int i = 0; i < 8; i++)
{
    Node child = children[i];

    occupied |= child.occupied;

    if(child.occupied)
    {
        colour += child.averageColor;
        count++;
    }
}

node.occupied = occupied;
node.terminal = !occupied;

if(count > 0)
    node.averageColor = colour / count;
```

Parents never access voxel memory. After the deepest layer, every pass operates entirely on the SVO.

## Dispatch Strategy

Each tree level is generated separately:

```cpp
for(depth = maxDepth; depth >= 0; depth--)
{
    glDispatchCompute(...);

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}
```

| Benefit      | Reason                                  |
| ------------ | ---------------------------------------- |
| Simple       | Parents always read completed children.  |
| No recursion | Pure iterative generation.               |
| No races     | One dispatch per depth.                  |
| Predictable  | Every pass is independent.               |

## Streaming

Chunk loading:

```
CPU builds voxel data
        ↓
Upload voxel data
        ↓
Generate SVO
        ↓
Upload contiguous node block
        ↓
Update ChunkInfo.rootIndex
```

Chunk unloading:

```
Free contiguous memory block
```

Potential allocator:

- Free-list allocator
- Occasional memory compaction

## Double Buffering (Optional)

Maintain two SVO buffers — a current buffer and a building buffer:

```
Render          Compute Shader
   ↓                 ↓
Current Buffer   Building Buffer
```

Once complete: `std::swap(currentBuffer, buildingBuffer);`

| Benefit                | Explanation                       |
| ----------------------- | ---------------------------------- |
| No race conditions      | Renderer never sees partial data. |
| Simple synchronization  | Swap once generation completes.    |
| Stable rendering        | Previous frame remains valid.      |

## Ray Traversal

Traversal begins from the chunk's root:

```
Ray → Chunk → rootIndex → Traverse
```

Traversal stops when:

- `occupied == false`
- `terminal == true`
- Maximum LOD depth reached

## Why Static Memory?

Traditional GPU SVO builders require dynamic allocation, atomics, prefix sums, and node compaction. This design avoids all of that:

| Traditional Builder       | Proposed Builder      |
| -------------------------- | ---------------------- |
| Dynamic nodes              | Fixed nodes            |
| GPU allocator               | Mathematical indexing  |
| Atomics                    | None                   |
| Recursive                  | Iterative              |
| Difficult synchronization  | One barrier per depth  |
| Memory efficient           | Compute efficient      |

The trade-off is additional memory usage in exchange for dramatically simpler GPU algorithms.

## Advantages

| Feature                 | Status |
| ------------------------ | ------ |
| GPU parallel generation  | ✅      |
| Chunk streaming          | ✅      |
| Predictable memory       | ✅      |
| No dynamic allocation    | ✅      |
| Contiguous uploads       | ✅      |
| Cache friendly           | ✅      |
| Bottom-up generation     | ✅      |
| Easy synchronization     | ✅      |
| Future LOD support       | ✅      |

## Future Improvements

- Child bitmask compression.
- Voxel count weighted colour averaging.
- Material IDs.
- Surface normals.
- Ambient occlusion.
- Per-chunk GPU allocator.
- Asynchronous SVO rebuilding.
- Compression for distant chunks.

# Per-Voxel Deferred Shadow Pipeline

## Problem

A full-screen shader naturally binds computation to **pixels**, not **voxels**. Casting a shadow ray directly inside that pass (e.g. a naive `reflect`/shadow-ray-per-hit approach) computes one shadow sample per pixel, which:

- Produces smooth, sub-voxel-precision shadow edges (wrong stylistically — we want blocky, per-voxel shadows).
- Wastes work: a voxel/node covering 500 pixels gets its shadow recomputed 500 times instead of once.

The fix is to decouple shadow computation from screen-space entirely, using a deferred, node-indexed approach — analogous to G-buffer deferred shading, but keyed on SVO node index instead of triangle/pixel.

## Core Fix (mandatory regardless of approach)

Snap the shadow ray origin to the hit node's cell (not the exact continuous hit point):

```glsl
ivec3 voxelCoord = ivec3(floor(hitPos + normal * 0.001));
vec3 shadowOrigin = vec3(voxelCoord) + 0.5;
```

This alone guarantees every pixel touching the same node produces the same shadow result — the prerequisite for the blocky look.

## Why the Static Tree Layout Matters Here

Because every chunk's tree is static/complete (per the SVO design above — every possible node at every depth exists, no pruning, no variable subtree sizes), a node's ancestry and octant path fall out of its local index within the chunk using plain integer math:

```glsl
// localIdx is a node's index relative to its chunk's rootIndex
uint parentLocal = (localIdx - 1) / 8;
uint octant      = (localIdx - 1) % 8;
```

This is the property the shadow pipeline leans on directly: world position is recoverable from the index alone, with zero extra stored fields (no parent pointer, no packed path) and zero dependent memory-chasing reads. It would not hold under a compacted/pruned layout, which is why this pipeline is built specifically on top of the static-tree design rather than being layout-agnostic.

## Three-Pass Pipeline

### Pass 1 — Primary ray march (2D dispatch, per pixel)

Existing traversal pass (chunk lookup → `rootIndex` → descend). No change to the traversal logic itself — just store the result instead of shading immediately.

```glsl
uint globalNodeIndex = traverse(rayOrigin, rayDir); // absolute index into the SSBO
imageStore(gbufferNodeIndex, pixelCoord, globalNodeIndex); // 2D G-buffer, one uint per texel
```

`gbufferNodeIndex` is a normal 2D G-buffer texture — just like a normal/albedo buffer — that happens to store a node index instead of a color. No reprojection, no camera matrix involved beyond what Pass 1 already does.

### Pass 2 — Shadow calculation (dispatch over nodes, NOT pixels)

Never touches screen coordinates. Operates purely in SSBO node-index space.

```glsl
uint globalNodeIndex = <from dispatch: either full node-array pass or unique-list entry>;

// derive world position via pure arithmetic — see Position Recovery below
vec3 voxelWorldPos = recoverWorldPos(globalNodeIndex);

bool inShadow = shadowRayMarch(voxelWorldPos, lightDir); // ONE ray per unique node

shadowBuffer[globalNodeIndex] = inShadow ? 0.0 : 1.0; // flat buffer, parallel to the node SSBO
```

Two ways to feed this pass, chosen based on how many chunks/nodes are resident:

- **Small/bounded set of loaded chunks**: skip dedup entirely. Dispatch Pass 2 over the entire resident node range (or every node up to each chunk's `maxDepth`), one thread per node. Simpler, no atomics; pay for unseen/off-screen nodes but often cheaper overall when the resident set is small.
- **Large/streaming world with many chunks resident**: build a compacted unique-node list first. In Pass 1, per pixel, hash `globalNodeIndex`, use `atomicCompSwap` on a claim table to detect "first thread to touch this node," and append winners to a compacted list (atomic counter + append). Dispatch Pass 2 only over that list — exactly one shadow ray per unique node actually visible, no wasted work on unseen parts of any resident tree.

(Alternative considered: sort G-buffer entries by node index and run-length-encode duplicates. More overhead — only worth it if a sort pass already exists in the pipeline for other reasons.)

### Pass 3 — Composite / shade (2D dispatch, per pixel again)

No raymarching here — just two reads and a multiply.

```glsl
uint globalNodeIndex = imageLoad(gbufferNodeIndex, pixelCoord).x; // 2D fetch, from Pass 1
float shadow = shadowBuffer[globalNodeIndex];                      // flat buffer fetch, keyed by node index
outColor = albedo * shadow * NdotL;
```

## Position Recovery (pure arithmetic, no extra fields)

Walking up from the hit node to its chunk's root, using only `/8` and `%8`:

```glsl
vec3 recoverWorldPos(uint globalNodeIndex, ChunkInfo chunk) {
    uint localIdx = globalNodeIndex - chunk.rootIndex;

    vec3 pos = vec3(0);
    float size = chunkSize;

    while (localIdx != 0) {
        uint octant = (localIdx - 1) % 8;
        localIdx    = (localIdx - 1) / 8;
        size *= 0.5;
        pos += size * octantOffset(octant); // octantOffset: (0,0,0)..(1,1,1) per bit of x/y/z
    }

    return chunk.worldPosition + pos;
}
```

No `parentIndex` field, no `packedPath` field, no dependent pointer-chasing memory reads — just register-only integer math, O(depth) iterations. Cheap in absolute terms since it's paid once per unique visible node, not once per pixel.

Shadow buffer is a flat array, mirroring the node SSBO 1:1:

```glsl
layout(std430, binding = X) buffer ShadowBuffer {
    float shadow[]; // shadow[globalNodeIndex] corresponds to nodes[globalNodeIndex]
};
```

Same size, same indexing, same lifecycle as the node SSBO — allocated/streamed/hotswapped per chunk alongside it, mirroring the engine's existing chunk load/unload flow (build → upload node block → update `ChunkInfo.rootIndex` → now also alongside a same-sized shadow block).

## Key Clarifications (things that felt confusing but aren't)

- No reverse-projection needed anywhere. The only direction ever computed is pixel → nodeIndex (Pass 1, via existing traversal) and nodeIndex → world position (Pass 2, via the arithmetic walk above). Nothing ever goes node → screen.
- `gbufferNodeIndex` is a 2D buffer, despite carrying tree-structural information. It's addressed by `pixelCoord` just like a normal/albedo G-buffer texture — the meaning of the stored value is unrelated to dispatch dimensionality.
- `shadowBuffer` is a flat 1D array, addressed by node index — a completely different address space from the screen, with no 3D grid coordinate system involved.
- Pass 3 never re-traverses the tree. Traversal happens exactly once (Pass 1); its result (`globalNodeIndex`) is carried forward via the G-buffer.
- LOD is inherent, not bolted on. A ray stopped early at `chunk.maxDepth` (or at a `terminal` node) simply returns that node's index — Pass 2 computes one shadow ray for it, same as any other node. No separate per-LOD buffers or lookup logic needed.

## Open Decision

Whether to shadow per voxel face vs per voxel/node cell:

- **Per face**: slightly better quality (top face lit, side face shadowed can differ on the same node). Requires storing/addressing by (nodeIndex, faceIndex) instead of just nodeIndex.
- **Per cell**: simpler, faster, more overtly blocky — matches the stylistic goal more directly.

## Next Steps / Still Open

- Decide full-array-brute-force vs unique-list-with-atomics for Pass 2, based on actual resident node count across loaded chunks.
- Decide face vs cell shadow granularity.
- Nail down light orientation handling (fixed sun vs arbitrary) for the Pass 2 ray-march/propagation strategy.
- Consider whether ray-march step size in Pass 2 needs to adapt to a node's depth (coarser nodes = larger effective voxel size = larger safe step), mirroring how the main traversal already handles `maxDepth`/LOD.
- Decide how the shadow SSBO block is sized/streamed relative to each chunk's node block during load/unload (should mirror the existing `ChunkInfo`-driven streaming flow).
