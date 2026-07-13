# VoxelEngine

A voxel engine written in c++, this is a rewrite of my old project

# Table of Contents

- [GPU Sparse Voxel Octree (SVO) Design](#gpu-sparse-voxel-octree-svo-design)
  - [Goals](#goals)
  - [High-Level Architecture](#high-level-architecture)
  - [GPU Memory Layout](#gpu-memory-layout)
  - [Static Tree Layout](#static-tree-layout)
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
  - [Three-Pass Pipeline](#three-pass-pipeline)
    - [Pass 1 — Primary Ray March](#pass-1--primary-ray-march-2d-dispatch-per-pixel)
    - [Pass 2 — Shadow Calculation](#pass-2--shadow-calculation-dispatch-over-voxels-not-pixels)
    - [Pass 3 — Composite / Shade](#pass-3--composite--shade-2d-dispatch-per-pixel-again)
  - [Key Clarifications](#key-clarifications-things-that-felt-confusing-but-arent)
  - [Open Decision](#open-decision)
  - [Next Steps / Still Open](#next-steps--still-open)

---

# GPU Sparse Voxel Octree (SVO) Design

## Goals

- GPU-friendly SVO generation.
- Simple architecture with predictable memory.
- Chunk streaming support.
- No dynamic GPU allocation.
- Fast ray traversal.
- Future-proof for LOD.

---

# High-Level Architecture

The world is divided into chunks.

Each chunk owns one SVO.

```
World
├── Chunk (0,0)
├── Chunk (1,0)
├── Chunk (2,0)
└── ...
```

Only nearby chunks are streamed into GPU memory.

Each chunk has metadata:

```cpp
struct ChunkInfo
{
    uint rootIndex;      // Beginning of this chunk's SVO
    ivec3 worldPosition;
    uint maxDepth;       // Maximum traversal depth (LOD)
};
```

---

# GPU Memory Layout

A single SSBO stores every loaded chunk.

```
+----------------------------------------------------------------+
| Chunk 0 | Chunk 1 | Chunk 2 | Chunk 3 | Chunk 4 | ...          |
+----------------------------------------------------------------+
```

Each chunk occupies one contiguous memory block.

### Advantages

| Benefit | Explanation |
|----------|-------------|
| Fast uploads | One `glBufferSubData()` call uploads the entire SVO. |
| Cache friendly | Nodes from the same chunk remain close together. |
| Easy streaming | Chunks are uploaded and removed independently. |
| Simple addressing | Only the root index needs updating. |

---

# Static Tree Layout

Instead of dynamically allocating nodes, every possible node exists.

Example for a chunk:

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

---

# Node Structure

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

| Field | Purpose |
|--------|---------|
| voxelCount | Better colour averaging |
| childMask | Faster traversal |
| normal | Lighting |
| materialID | Material lookup |
| emission | Lighting |

---

# GPU Construction

Generation happens entirely on the GPU using compute shaders.

## Pass 1 — Deepest Level

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

Pseudo-code:

```cpp
node.occupied = ContainsVoxel(nodeBounds);
node.terminal = true;

if(node.occupied)
{
    node.averageColor = ComputeAverageColor(nodeBounds);
}
```

No synchronization is required because every thread writes to a unique node.

---

## Higher Levels

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

Each parent simply inspects its eight children.

Pseudo-code:

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

Notice that parents never access voxel memory.

After the deepest layer, every pass operates entirely on the SVO.

---

# Dispatch Strategy

Each tree level is generated separately.

```cpp
for(depth = maxDepth; depth >= 0; depth--)
{
    glDispatchCompute(...);

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}
```

### Why?

| Benefit | Reason |
|----------|--------|
| Simple | Parents always read completed children. |
| No recursion | Pure iterative generation. |
| No races | One dispatch per depth. |
| Predictable | Every pass is independent. |

---

# Streaming

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

---

# Double Buffering (Optional)

Maintain two SVO buffers.

```
Current Buffer
Building Buffer
```

During rebuild:

```
Render
        ↓
Current Buffer

Compute Shader
        ↓
Building Buffer
```

Once complete:

```cpp
std::swap(currentBuffer, buildingBuffer);
```

Advantages:

| Benefit | Explanation |
|----------|-------------|
| No race conditions | Renderer never sees partial data. |
| Simple synchronization | Swap once generation completes. |
| Stable rendering | Previous frame remains valid. |

---

# Ray Traversal

Traversal begins from the chunk's root.

```
Ray
    ↓
Chunk
    ↓
rootIndex
    ↓
Traverse
```

Traversal stops when:

- `occupied == false`
- `terminal == true`
- Maximum LOD depth reached

---

# Why Static Memory?

Traditional GPU SVO builders require:

- Dynamic allocation
- Atomics
- Prefix sums
- Node compaction

This design avoids all of that.

| Traditional Builder | Proposed Builder |
|---------------------|------------------|
| Dynamic nodes | Fixed nodes |
| GPU allocator | Mathematical indexing |
| Atomics | None |
| Recursive | Iterative |
| Difficult synchronization | One barrier per depth |
| Memory efficient | Compute efficient |

The trade-off is additional memory usage in exchange for dramatically simpler GPU algorithms.

---

# Advantages

| Feature | Status |
|----------|--------|
| GPU parallel generation | ✅ |
| Chunk streaming | ✅ |
| Predictable memory | ✅ |
| No dynamic allocation | ✅ |
| Contiguous uploads | ✅ |
| Cache friendly | ✅ |
| Bottom-up generation | ✅ |
| Easy synchronization | ✅ |
| Future LOD support | ✅ |

---

# Future Improvements

- Child bitmask compression.
- Voxel count weighted colour averaging.
- Material IDs.
- Surface normals.
- Ambient occlusion.
- Per-chunk GPU allocator.
- Asynchronous SVO rebuilding.
- Compression for distant chunks.
  
---

# Per-Voxel Deferred Shadow Pipeline

## Problem

A full-screen shader naturally binds computation to **pixels**, not **voxels**. Casting a shadow ray directly inside that pass (e.g. a naive `reflect`/shadow-ray-per-hit approach) computes one shadow sample per pixel, which:

- Produces smooth, sub-voxel-precision shadow edges (wrong stylistically — we want blocky, per-voxel shadows).
- Wastes work: a voxel covering 500 pixels gets its shadow recomputed 500 times instead of once.

The fix is to decouple shadow computation from screen-space entirely, using a deferred, voxel-indexed approach — analogous to G-buffer deferred shading, but keyed on voxel ID instead of triangle/pixel.

## Core Fix (mandatory regardless of approach)

Snap the shadow ray origin to the voxel (grid cell / voxel center), not the exact continuous hit point:

```glsl
ivec3 voxelCoord = ivec3(floor(hitPos + normal * 0.001));
vec3 shadowOrigin = vec3(voxelCoord) + 0.5;
```

This alone guarantees every pixel touching the same voxel produces the same shadow result — the prerequisite for the blocky look.

## Three-Pass Pipeline

### Pass 1 — Primary ray march (2D dispatch, per pixel)

Existing DDA/ray-marching pass. No change to the traversal itself — just store the result instead of shading immediately.

```glsl
ivec3 voxelID = ddaTraversal(rayOrigin, rayDir);
imageStore(gbufferVoxelID, pixelCoord, voxelID); // 2D texture, ivec3-valued texel
```

`gbufferVoxelID` is a normal 2D G-buffer texture — just like a normal/albedo buffer — that happens to store 3 components (a voxel coordinate) per texel. No reprojection, no camera matrix involved beyond what Pass 1 already does.

### Pass 2 — Shadow calculation (dispatch over nodes, NOT pixels)

Never touches screen coordinates. Operates purely in SVO node-index space.

```glsl
uint nodeIndex = <from dispatch: either full node-array pass or unique-list entry>;

// derive world position from the SVO structure itself (see "SVO Storage" below),
// NOT from a 3D grid coordinate
vec3 voxelWorldPos = recoverWorldPos(nodeIndex);

bool inShadow = shadowRayMarch(voxelWorldPos, lightDir); // ONE ray per unique node

shadowBuffer[nodeIndex] = inShadow ? 0.0 : 1.0; // flat buffer, parallel to node array
```

Two ways to feed this pass, chosen based on tree/world characteristics:

- **Small/bounded tree**: skip dedup entirely. Dispatch Pass 2 over the *entire node array* (or every node up to some max depth), one thread per node. Simpler, no atomics; pay for unseen/off-screen nodes but often cheaper overall for small trees.
- **Large/streaming world**: build a compacted unique-node list first.
  - In Pass 1, per pixel: hash `nodeIndex`, use `atomicCompSwap` on a claim table to detect "first thread to touch this node," and append winners to a compacted list (atomic counter + append).
  - Dispatch Pass 2 only over that list — exactly one shadow ray per unique node actually visible, no wasted work on unseen parts of the tree.

(Alternative considered: sort G-buffer entries by node index and run-length-encode duplicates. More overhead — only worth it if a sort pass already exists in the pipeline for other reasons.)

### Pass 3 — Composite / shade (2D dispatch, per pixel again)

No raymarching here — just two reads and a multiply.

```glsl
uint nodeIndex = imageLoad(gbufferNodeIndex, pixelCoord).x;  // 2D fetch, from Pass 1
float shadow    = shadowBuffer[nodeIndex];                    // flat buffer fetch, keyed by node index
outColor = albedo * shadow * NdotL;
```

## SVO Storage & World-Position Recovery

The voxel world is stored as a **flattened SVO (sparse voxel octree)**, built depth-first. Stopping traversal/build at a given depth naturally gives an averaged/coarser node — LOD is inherent to the structure, not a separate system.

```cpp
struct Node {
    uint32_t id;
    uint32_t depth;
    uint32_t firstChild;   // index of first of 8 contiguous children, or INVALID if leaf
    uint32_t parentIndex;  // needed for child -> parent walk (see below)
};
```

**Why `nodeIndex` alone doesn't give position for free:** because the array is built depth-first, sibling subtrees have variable size (depending on pruning), so there is no fixed arithmetic like a heap's `parent = (i-1)/8` that works in reverse. Recovering a node's position/depth requires walking up via stored parent links, not computed offsets.

**Recovery walk (Pass 2, once per unique node — cheap in absolute terms since it's not per-pixel):**

```glsl
vec3 pos = vec3(0);
float size = worldSize;
uint idx = nodeIndex;
while (idx != 0) {
    uint parent = nodes[idx].parentIndex;
    uint octant = idx - nodes[parent].firstChild; // requires all 8 child slots reserved per node,
                                                    // even pruned/leaf ones, so offset is consistent
    size *= 0.5;
    pos += size * octantOffset(octant); // octantOffset: (0,0,0)..(1,1,1) per bit of x/y/z
    idx = parent;
}
voxelWorldPos = chunkOrigin + pos;
```

This is pure integer/ALU work plus O(depth) direct memory reads (no search, no dependent pointer-chasing beyond one parent read per level) — negligible next to the cost of the shadow ray march itself, and only paid once per unique visible node rather than once per pixel.

**Shadow buffer is a flat array, not a 3D texture:** since the addressing key is now a flattened node index rather than a 3D grid coordinate, the shadow buffer mirrors the node array directly — same length, same index, hotswapped together whenever the SVO is rebuilt/reloaded.

```glsl
layout(std430, binding = X) buffer ShadowBuffer {
    float shadow[]; // shadow[nodeIndex] corresponds to nodes[nodeIndex]
};
```

## Key Clarifications (things that felt confusing but aren't)

- **No reverse-projection needed anywhere.** The only direction ever computed is pixel → nodeIndex (Pass 1, via existing traversal) and nodeIndex → world position (Pass 2, via the parent-walk above). Nothing ever goes node → screen.
- **`gbufferNodeIndex` is a 2D buffer**, despite carrying tree-structural information. It's addressed by `pixelCoord` just like a normal/albedo G-buffer texture — component meaning is unrelated to dispatch dimensionality.
- **`shadowBuffer` is a flat 1D array**, addressed by node index — a completely different address space from the screen, and no longer tied to any 3D grid coordinate system.
- **Pass 3 never re-traverses the tree.** Traversal happens exactly once (Pass 1); its result (`nodeIndex`) is carried forward via the G-buffer.
- **LOD is inherent, not bolted on.** A ray hitting a coarse/averaged node at a shallow depth simply returns that node's index — Pass 2 computes one shadow ray for it, same as any other node. No separate per-LOD buffers or lookup logic needed.

## Open Decision

Whether to shadow **per voxel face** vs **per voxel/node cell**:

- **Per face**: slightly better quality (top face lit, side face shadowed can differ on the same node). Requires storing/addressing by (nodeIndex, faceIndex) instead of just nodeIndex.
- **Per cell**: simpler, faster, more overtly blocky — matches the stylistic goal more directly.

## Next Steps / Still Open

- Decide full-array-brute-force vs unique-list-with-atomics for Pass 2, based on actual live/visible node count.
- Decide face vs cell shadow granularity.
- Nail down light orientation handling (fixed sun vs arbitrary) for the Pass 2 ray-march/propagation strategy.
- Confirm build process always reserves all 8 child slots per node (even pruned/leaf children) — required for the `octant = idx - nodes[parent].firstChild` trick to stay valid.
- Consider whether ray-march step size in Pass 2 needs to adapt to a node's depth (coarser nodes = larger effective voxel size = larger safe step), mirroring how the main traversal already needs to handle LOD boundaries.
