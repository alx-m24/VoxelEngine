#pragma once

#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>
#include <vector>

#include "VoxelEngine/mesh/vertex.hpp"
#include "VoxelEngine/buffer/buffer.hpp"
#include "VoxelEngine/context/draw_context.hpp"

namespace VoxelEngine {
    template<typename IndexRep = uint16_t>
    class Mesh {
        protected:
            std::vector<Vertex>   vertices{};
            std::vector<IndexRep> indices{};

        private:
            vk::Buffer         vertexBuffer = nullptr, indexBuffer = nullptr;
            VmaAllocation      vertexAlloc = nullptr, indexAlloc = nullptr;

            VmaAllocator allocator{};
        public:
            // Should fill the vertices and indices
            Mesh() = default;
            ~Mesh() {
                Delete();
            }

            Mesh(const Mesh&) = delete;
            Mesh& operator=(const Mesh&) = delete;

            inline Type::Status Upload(const Context::UploadContext& uploadCtx) {
                Delete();

                allocator = uploadCtx.allocator;

                Type::Status status{};
                status = Buffer::createDeviceBuffer(uploadCtx, vertices.data(), vertices.size() * sizeof(Vertex),
                           vk::BufferUsageFlagBits::eVertexBuffer, vertexBuffer, vertexAlloc);
                if (!status) {
                    Delete();
                    return status;
                }

                status = Buffer::createDeviceBuffer(uploadCtx, indices.data(), indices.size() * sizeof(IndexRep),
                           vk::BufferUsageFlagBits::eIndexBuffer, indexBuffer, indexAlloc);
                if (!status) {
                    Delete();
                    return status;
                }

                return status;
            }

            inline void Draw(Context::DrawContext& drawCtx) {
                drawCtx.commadBuffer.bindVertexBuffers(0, vertexBuffer, vk::DeviceSize{0});
                drawCtx.commadBuffer.bindIndexBuffer(indexBuffer, 0, vk::IndexType::eUint16);
                drawCtx.commadBuffer.drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
            }

            inline void Delete() {
                if (allocator) {
                    if (vertexBuffer && vertexAlloc) {
                        vmaDestroyBuffer(allocator, vertexBuffer, vertexAlloc);
                    }

                    if (indexBuffer && indexAlloc) {
                        vmaDestroyBuffer(allocator, indexBuffer, indexAlloc);
                    }
                }

                vertexBuffer = nullptr;
                indexBuffer = nullptr;
                vertexAlloc = nullptr;
                indexAlloc = nullptr;
                allocator = nullptr;
            }
    };
}
