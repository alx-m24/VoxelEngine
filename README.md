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

### Pass 2 — Shadow calculation (dispatch over voxels, NOT pixels)

Never touches screen coordinates. Operates purely in voxel-grid space.

```glsl
ivec3 voxelID = <from dispatch: either full-grid index or unique-list entry>;

// grid arithmetic only — no inverse-view, no projection
vec3 voxelWorldPos = gridOrigin + vec3(voxelID) * voxelSize + voxelSize * 0.5;

bool inShadow = shadowRayMarch(voxelWorldPos, lightDir); // ONE ray per voxel

imageStore(voxelShadowBuffer, voxelID, inShadow ? 0.0 : 1.0); // 3D texture, addressed by voxel coords
```

Two ways to feed this pass, chosen based on grid characteristics:

- **Dense/bounded grid** (small enough, e.g. up to a few million cells): skip dedup entirely. Dispatch Pass 2 over the *entire grid*, one thread per cell. Simpler, no atomics; pay for unseen cells but often cheaper overall for small grids.
- **Sparse/huge/streaming world**: build a compacted unique-voxel list first.
  - In Pass 1, per pixel: hash `voxelID`, use `atomicCompSwap` on a claim table to detect "first thread to touch this voxel," and append winners to a compacted list (atomic counter + append).
  - Dispatch Pass 2 only over that list — exactly one shadow ray per unique voxel actually visible, no wasted work on empty/unseen space.

(Alternative considered: sort G-buffer entries by voxel ID and run-length-encode duplicates. More overhead — only worth it if a sort pass already exists in the pipeline for other reasons.)

### Pass 3 — Composite / shade (2D dispatch, per pixel again)

No raymarching here — just two texture reads and a multiply.

```glsl
ivec3 voxelID = imageLoad(gbufferVoxelID, pixelCoord).xyz;   // 2D fetch, from Pass 1
float shadow  = imageLoad(voxelShadowBuffer, voxelID).r;      // 3D fetch, keyed by voxelID
outColor = albedo * shadow * NdotL;
```

## Key Clarifications (things that felt confusing but aren't)

- **No reverse-projection needed anywhere.** The only direction ever computed is pixel → voxelID (Pass 1, via existing DDA) and voxelID → world position (Pass 2, via trivial `origin + id * size` arithmetic). Nothing ever goes voxel → screen.
- **`gbufferVoxelID` is 2D, not 3D**, despite storing an `ivec3`. Component count (3 numbers per texel) is unrelated to dispatch dimensionality (2D, addressed by `pixelCoord`). Same as how a normal buffer stores a `vec3` without requiring a re-lighting pass to read it.
- **`voxelShadowBuffer` is a separate 3D volume**, addressed by voxel grid coordinates — a completely different address space from the screen.
- **Pass 3 never re-raymarches.** The DDA traversal happens exactly once (Pass 1); its result is carried forward via the G-buffer.

## Open Decision

Whether to shadow **per voxel face** vs **per voxel cell**:

- **Per face**: slightly better quality (top face lit, side face shadowed can differ on the same voxel). Requires storing/addressing by (voxelID, faceIndex) instead of just voxelID.
- **Per cell**: simpler, faster, more overtly blocky — matches the stylistic goal more directly.

## Next Steps / Still Open

- Decide dense-grid-brute-force vs unique-list-with-atomics based on actual live voxel count and whether the world streams in chunks.
- Decide face vs cell shadow granularity.
- Nail down light orientation handling (fixed sun vs arbitrary) for the Pass 2 ray-march/propagation strategy.
- Decide voxel data storage layout (dense 3D texture vs sparse/brick map) — affects both `voxelWorldPos` computation and how `voxelShadowBuffer` is allocated/addressed.
