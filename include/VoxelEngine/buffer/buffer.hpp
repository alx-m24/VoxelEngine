#pragma once

#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>

#include "VoxelEngine/context/upload_context.hpp"
#include "VoxelEngine/types/status.hpp"
#include "VoxelEngine/api/vma.hpp"

namespace VoxelEngine::Buffer {
    inline Type::Status createDeviceBuffer(const Context::UploadContext& uploadCtx, const void* data, vk::DeviceSize size, vk::BufferUsageFlags usage,
                            vk::Buffer& outBuffer, VmaAllocation& outAlloc) {
        // 1. host-visible staging buffer
        const vk::BufferCreateInfo stagingInfo{ .size = size, .usage = vk::BufferUsageFlagBits::eTransferSrc };
        VmaAllocationCreateInfo stagingAlloc{};
        stagingAlloc.usage = VMA_MEMORY_USAGE_AUTO;
        stagingAlloc.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        VkBuffer      staging      = VK_NULL_HANDLE;
        VmaAllocation stagingMemory = nullptr;
        try {
            vmaCheck(vmaCreateBuffer(uploadCtx.allocator, native(stagingInfo), &stagingAlloc, &staging, &stagingMemory, nullptr), "staging");
            vmaCheck(vmaCopyMemoryToAllocation(uploadCtx.allocator, data, stagingMemory, 0, size), "staging copy");

            // 2. uploadCtx.device-local destination
            const vk::BufferCreateInfo bufferInfo{ .size = size, .usage = usage | vk::BufferUsageFlagBits::eTransferDst };
            VmaAllocationCreateInfo bufferAlloc{};
            bufferAlloc.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
            VkBuffer raw = VK_NULL_HANDLE;
            vmaCheck(vmaCreateBuffer(uploadCtx.allocator, native(bufferInfo), &bufferAlloc, &raw, &outAlloc, nullptr), "device buffer");
            outBuffer = raw;
        }
        catch (const std::runtime_error& e) {
            return { e.what() };
        }

        // 3. one-shot copy
        vk::CommandBuffer cmd = uploadCtx.device.allocateCommandBuffers({ .commandPool = uploadCtx.commandPool,
                                                                .level = vk::CommandBufferLevel::ePrimary,
                                                                .commandBufferCount = 1 })[0];
        cmd.begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
        cmd.copyBuffer(staging, outBuffer, vk::BufferCopy{ .srcOffset = 0, .dstOffset = 0, .size = size });
        cmd.end();
        uploadCtx.transferQueue.submit(vk::SubmitInfo{ .commandBufferCount = 1, .pCommandBuffers = &cmd });
        uploadCtx.transferQueue.waitIdle();

        uploadCtx.device.freeCommandBuffers(uploadCtx.commandPool, cmd);
        vmaDestroyBuffer(uploadCtx.allocator, staging, stagingMemory);

        return {};
    }
}
