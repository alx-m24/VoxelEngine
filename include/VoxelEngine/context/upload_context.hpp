#pragma once

#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>

namespace VoxelEngine::Context {
    struct UploadContext {
        vk::Device device{};
        vk::CommandPool commandPool{};
        vk::Queue transferQueue{};

        VmaAllocator allocator{};
    };
}
