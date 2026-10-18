#pragma once

#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>

// vulkan.hpp struct -> pointer to the equivalent C struct (for VMA's C API).
template <class T>
const typename T::NativeType* native(const T& v) {
    return &static_cast<const typename T::NativeType&>(v);
}

inline void vmaCheck(VkResult r, const char* what) {
    if (r != VK_SUCCESS) throw std::runtime_error(std::string("VMA call failed: ") + what);
}
