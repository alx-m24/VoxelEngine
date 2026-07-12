# VoxelEngine

A voxel engine written in c++, this is a rewrite of my old project

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
