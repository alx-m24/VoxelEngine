#pragma once

#include <vulkan/vulkan.hpp>

namespace VoxelEngine::Context {
    struct DrawContext {
        vk::CommandBuffer& commadBuffer;
    };
}
