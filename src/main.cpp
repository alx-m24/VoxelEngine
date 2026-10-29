#include <vulkan/vulkan.hpp>
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

#define GLFW_INCLUDE_NONE      // don't pull in OpenGL headers
#define GLFW_INCLUDE_VULKAN    // glfwCreateWindowSurface & friends
#include <GLFW/glfw3.h>

#include <vk_mem_alloc.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <filesystem>
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "VoxelEngine/api/vma.hpp"
#include "VoxelEngine/mesh/primitive/cube.hpp"
#include "VoxelEngine/mesh/primitive/quad.hpp"

namespace VoxelEngine {

constexpr uint32_t   kFramesInFlight = 2;
constexpr vk::Format kDepthFormat    = vk::Format::eD32Sfloat;

struct PushConstants {      // 128 bytes = the guaranteed minimum push-constant size
    glm::mat4 mvp;
    glm::mat4 model;
};

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                             VkDebugUtilsMessageTypeFlagsEXT,
                                             const VkDebugUtilsMessengerCallbackDataEXT* data, void*) {
    if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        std::fprintf(stderr, "[vulkan] %s\n", data->pMessage);
    return VK_FALSE;
}

std::vector<uint32_t> readSpirv(const std::string& path) {
    std::error_code ec;
    if (!std::filesystem::is_regular_file(path, ec))
        throw std::runtime_error("shader is missing or not a regular file: " + path);
    const auto bytes = std::filesystem::file_size(path, ec);
    if (ec || bytes == 0 || bytes % sizeof(uint32_t) != 0)
        throw std::runtime_error("bad SPIR-V size (" + std::to_string(bytes) + " bytes): " + path);

    std::ifstream file(path, std::ios::binary);
    std::vector<uint32_t> code(bytes / sizeof(uint32_t));
    file.read(reinterpret_cast<char*>(code.data()), static_cast<std::streamsize>(bytes));
    if (!file) throw std::runtime_error("failed to read: " + path);
    return code;
}

class App {
public:
    void run() {
        initWindow();
        initVulkan();
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            drawFrame();
        }
        cleanup();
    }

private:
    // --- window ---
    GLFWwindow* window  = nullptr;
    bool        resized = false;

    // --- core ---
    vk::DynamicLoader          loader;      // keeps the Vulkan loader library alive
    vk::Instance               instance;
    vk::DebugUtilsMessengerEXT messenger;
    vk::SurfaceKHR             surface;
    vk::PhysicalDevice         physical;
    vk::Device                 device;
    uint32_t                   queueFamily = 0;
    vk::Queue                  queue;
    VmaAllocator               allocator = nullptr;

    // --- swapchain-sized resources ---
    vk::SwapchainKHR             swapchain;
    vk::Format                   swapFormat = vk::Format::eUndefined;
    vk::Extent2D                 extent;
    std::vector<vk::Image>       swapImages;
    std::vector<vk::ImageView>   swapViews;
    std::vector<vk::Semaphore>   renderDone;          // one per swapchain image
    std::array<vk::Image, kFramesInFlight>       depthImages;
    std::array<vk::ImageView, kFramesInFlight>   depthViews;
    std::array<VmaAllocation, kFramesInFlight>   depthAllocs;

    // --- scene ---
    vk::PipelineLayout meshPipelineLayout;
    vk::Pipeline       meshPipeline;

    vk::PipelineLayout quadLayout;
    vk::Pipeline       quadPipeline;

    Primitives::Cube cubeMesh{};
    Primitives::Quad quadMesh{};

    // --- per-frame ---
    vk::CommandPool                              cmdPool;
    std::array<vk::CommandBuffer, kFramesInFlight> cmds;
    std::array<vk::Fence, kFramesInFlight>         inFlight;
    std::array<vk::Semaphore, kFramesInFlight>     imageAvailable;
    uint32_t                                       frame = 0;

    // ------------------------------------------------------------------ init
    void initWindow() {
        if (!glfwInit()) throw std::runtime_error("glfwInit failed");
        if (!glfwVulkanSupported()) throw std::runtime_error("GLFW: Vulkan loader not found");
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        window = glfwCreateWindow(1024, 640, "VoxelEngine - Vulkan", nullptr, nullptr);
        if (!window) throw std::runtime_error("glfwCreateWindow failed");
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, [](GLFWwindow* w, int, int) {
            static_cast<App*>(glfwGetWindowUserPointer(w))->resized = true;
        });
    }

    void initVulkan() {
        createInstance();
        createSurface();
        pickPhysicalDevice();
        createDevice();
        createAllocator();
        createCommandObjects();
        createSwapchain();
        createDepth();
        createPipeline();
        createQuadPipeline();
        cubeMesh.Upload({ .device = device, .commandPool = cmdPool, .transferQueue = queue, .allocator = allocator });
        quadMesh.Upload({ .device = device, .commandPool = cmdPool, .transferQueue = queue, .allocator = allocator });
    }

    void createInstance() {
        VULKAN_HPP_DEFAULT_DISPATCHER.init(loader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr"));

        uint32_t     count = 0;
        const char** glfwExt = glfwGetRequiredInstanceExtensions(&count);
        std::vector<const char*> extensions(glfwExt, glfwExt + count);
        std::vector<const char*> layers;

        bool validation = false;
#ifndef NDEBUG
        for (const auto& l : vk::enumerateInstanceLayerProperties())
            if (std::strcmp(l.layerName, "VK_LAYER_KHRONOS_validation") == 0) validation = true;
        if (validation) {
            layers.push_back("VK_LAYER_KHRONOS_validation");
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            extensions.push_back(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME);
        } else {
            std::fprintf(stderr, "Validation layer not found; continuing without it.\n");
        }
#endif
        const vk::ValidationFeatureEnableEXT enables[] = { 
            vk::ValidationFeatureEnableEXT::eSynchronizationValidation
        };

        vk::ValidationFeaturesEXT validationFeatures{ 
            .enabledValidationFeatureCount = 1, 
            .pEnabledValidationFeatures = enables
        };

        vk::ApplicationInfo appInfo{ .pApplicationName = "VoxelEngine", .apiVersion = VK_API_VERSION_1_3 };
        instance = vk::createInstance({
            .pNext = validation ? &validationFeatures : nullptr,
            .pApplicationInfo        = &appInfo,
            .enabledLayerCount       = static_cast<uint32_t>(layers.size()),
            .ppEnabledLayerNames     = layers.data(),
            .enabledExtensionCount   = static_cast<uint32_t>(extensions.size()),
            .ppEnabledExtensionNames = extensions.data() });
        VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);

        if (validation) {
            messenger = instance.createDebugUtilsMessengerEXT({
                .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                                   vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
                .messageType     = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                                   vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                                   vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
                .pfnUserCallback = debugCallback });
        }
    }

    void createSurface() {
        VkSurfaceKHR s = VK_NULL_HANDLE;
        if (glfwCreateWindowSurface(static_cast<VkInstance>(instance), window, nullptr, &s) != VK_SUCCESS)
            throw std::runtime_error("glfwCreateWindowSurface failed");
        surface = s;
    }

    void pickPhysicalDevice() {
        int bestScore = -1;
        for (vk::PhysicalDevice pd : instance.enumeratePhysicalDevices()) {
            const auto props = pd.getProperties();
            if (props.apiVersion < VK_API_VERSION_1_3) continue;

            bool hasSwapchain = false;
            for (const auto& e : pd.enumerateDeviceExtensionProperties())
                if (std::strcmp(e.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) hasSwapchain = true;
            if (!hasSwapchain) continue;

            const auto features = pd.getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features>();
            const auto& f13 = features.get<vk::PhysicalDeviceVulkan13Features>();
            if (!f13.dynamicRendering || !f13.synchronization2) continue;

            if (!(pd.getFormatProperties(kDepthFormat).optimalTilingFeatures &
                  vk::FormatFeatureFlagBits::eDepthStencilAttachment)) continue;

            // One queue family that can do graphics + present keeps this demo simple.
            const auto families = pd.getQueueFamilyProperties();
            uint32_t family = UINT32_MAX;
            for (uint32_t i = 0; i < families.size(); ++i)
                if ((families[i].queueFlags & vk::QueueFlagBits::eGraphics) && pd.getSurfaceSupportKHR(i, surface)) {
                    family = i;
                    break;
                }
            if (family == UINT32_MAX) continue;

            const int score = props.deviceType == vk::PhysicalDeviceType::eDiscreteGpu ? 1000 : 0;
            if (score > bestScore) {
                bestScore   = score;
                physical    = pd;
                queueFamily = family;
            }
        }
        if (bestScore < 0) throw std::runtime_error("no suitable Vulkan 1.3 GPU found");
        std::printf("GPU: %s\n", physical.getProperties().deviceName.data());
    }

    void createDevice() {
        const float priority = 1.0f;
        vk::DeviceQueueCreateInfo queueInfo{ .queueFamilyIndex = queueFamily, .queueCount = 1, .pQueuePriorities = &priority };

        vk::PhysicalDeviceVulkan13Features f13{ .synchronization2 = true, .dynamicRendering = true };
        const char* extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

        device = physical.createDevice({
            .pNext                   = &f13,
            .queueCreateInfoCount    = 1,
            .pQueueCreateInfos       = &queueInfo,
            .enabledExtensionCount   = 1,
            .ppEnabledExtensionNames = extensions });
        VULKAN_HPP_DEFAULT_DISPATCHER.init(device);
        queue = device.getQueue(queueFamily, 0);
    }

    void createAllocator() {
        VmaVulkanFunctions fns{};
        fns.vkGetInstanceProcAddr = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetInstanceProcAddr;
        fns.vkGetDeviceProcAddr   = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetDeviceProcAddr;

        VmaAllocatorCreateInfo info{};
        info.physicalDevice   = static_cast<VkPhysicalDevice>(physical);
        info.device           = static_cast<VkDevice>(device);
        info.instance         = static_cast<VkInstance>(instance);
        info.vulkanApiVersion = VK_API_VERSION_1_3;
        info.pVulkanFunctions = &fns;
        vmaCheck(vmaCreateAllocator(&info, &allocator), "vmaCreateAllocator");
    }

    void createCommandObjects() {
        cmdPool = device.createCommandPool({ .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                                             .queueFamilyIndex = queueFamily });
        const auto buffers = device.allocateCommandBuffers({ .commandPool = cmdPool,
                                                             .level = vk::CommandBufferLevel::ePrimary,
                                                             .commandBufferCount = kFramesInFlight });
        for (uint32_t i = 0; i < kFramesInFlight; ++i) {
            cmds[i]           = buffers[i];
            inFlight[i]       = device.createFence({ .flags = vk::FenceCreateFlagBits::eSignaled });
            imageAvailable[i] = device.createSemaphore({});
        }
    }

    // ------------------------------------------------------------- swapchain
    void createSwapchain() {
        const auto caps    = physical.getSurfaceCapabilitiesKHR(surface);
        const auto formats = physical.getSurfaceFormatsKHR(surface);

        vk::SurfaceFormatKHR chosen = formats[0];
        for (const auto& f : formats)
            if (f.format == vk::Format::eB8G8R8A8Srgb && f.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) chosen = f;

        if (caps.currentExtent.width != UINT32_MAX) {
            extent = caps.currentExtent;
        } else {
            int w = 0, h = 0;
            glfwGetFramebufferSize(window, &w, &h);
            extent = vk::Extent2D{ std::clamp<uint32_t>(static_cast<uint32_t>(w), caps.minImageExtent.width, caps.maxImageExtent.width),
                       std::clamp<uint32_t>(static_cast<uint32_t>(h), caps.minImageExtent.height, caps.maxImageExtent.height) };
        }

        uint32_t imageCount = caps.minImageCount + 1;
        if (caps.maxImageCount > 0) imageCount = std::min(imageCount, caps.maxImageCount);

        swapFormat = chosen.format;
        vk::SwapchainKHR old = swapchain;
        swapchain = device.createSwapchainKHR({
            .surface          = surface,
            .minImageCount    = imageCount,
            .imageFormat      = chosen.format,
            .imageColorSpace  = chosen.colorSpace,
            .imageExtent      = extent,
            .imageArrayLayers = 1,
            .imageUsage       = vk::ImageUsageFlagBits::eColorAttachment,
            .imageSharingMode = vk::SharingMode::eExclusive,
            .preTransform     = caps.currentTransform,
            .compositeAlpha   = vk::CompositeAlphaFlagBitsKHR::eOpaque,
            .presentMode      = vk::PresentModeKHR::eFifo,   // vsync; always available
            .clipped          = VK_TRUE,
            .oldSwapchain     = old });
        if (old) device.destroySwapchainKHR(old);

        swapImages = device.getSwapchainImagesKHR(swapchain);
        for (vk::Image img : swapImages) {
            swapViews.push_back(device.createImageView({
                .image            = img,
                .viewType         = vk::ImageViewType::e2D,
                .format           = swapFormat,
                .subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 } }));
            renderDone.push_back(device.createSemaphore({}));
        }
    }

    void createDepth() {
        vk::ImageCreateInfo imageInfo{
            .imageType     = vk::ImageType::e2D,
            .format        = kDepthFormat,
            .extent        = { extent.width, extent.height, 1 },
            .mipLevels     = 1,
            .arrayLayers   = 1,
            .samples       = vk::SampleCountFlagBits::e1,
            .tiling        = vk::ImageTiling::eOptimal,
            .usage         = vk::ImageUsageFlagBits::eDepthStencilAttachment,
            .initialLayout = vk::ImageLayout::eUndefined };

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage    = VMA_MEMORY_USAGE_AUTO;
        allocInfo.flags    = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
        allocInfo.priority = 1.0f;

        for (uint32_t i = 0; i < kFramesInFlight; ++i) {
            vk::Image& depthImage = depthImages[i];
            vk::ImageView& depthView = depthViews[i];
            VmaAllocation& depthAlloc = depthAllocs[i];

            VkImage raw = VK_NULL_HANDLE;
            vmaCheck(vmaCreateImage(allocator, native(imageInfo), &allocInfo, &raw, &depthAlloc, nullptr), "depth image");
            depthImage = raw;
            depthView  = device.createImageView({
                .image            = depthImage,
                .viewType         = vk::ImageViewType::e2D,
                .format           = kDepthFormat,
                .subresourceRange = { vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1 } });
        }
    }

    void destroySwapchainResources() {
        for (auto s : renderDone) device.destroySemaphore(s);
        for (auto v : swapViews) device.destroyImageView(v);
        for (auto& d : depthViews) { device.destroyImageView(d); d = vk::ImageView{}; }
        for (uint32_t i = 0; i < kFramesInFlight; ++i) {
            vmaDestroyImage(allocator, static_cast<VkImage>(depthImages[i]), depthAllocs[i]);
            depthImages[i] = vk::Image{};
            depthAllocs[i] = VmaAllocation{};
        }
        renderDone.clear();
        swapViews.clear();
    }

    void recreateSwapchain() {
        int w = 0, h = 0;
        glfwGetFramebufferSize(window, &w, &h);
        while (w == 0 || h == 0) {               // minimized: sleep until we have pixels again
            glfwWaitEvents();
            glfwGetFramebufferSize(window, &w, &h);
        }
        device.waitIdle();
        destroySwapchainResources();
        createSwapchain();                        // passes the old swapchain and destroys it
        createDepth();
    }

    // -------------------------------------------------------------- pipeline
    vk::ShaderModule loadShader(const char* name) {
        const auto code = readSpirv(std::string(SHADER_DIR) + "/" + name + ".spv");
        return device.createShaderModule({ .codeSize = code.size() * sizeof(uint32_t), .pCode = code.data() });
    }

    void createQuadPipeline() {
        vk::ShaderModule vert = loadShader("voxels/stage1.vert");
        vk::ShaderModule frag = loadShader("voxels/stage1.frag");
        const vk::PipelineShaderStageCreateInfo stages[] = {
            { .stage = vk::ShaderStageFlagBits::eVertex,   .module = vert, .pName = "main" },
            { .stage = vk::ShaderStageFlagBits::eFragment, .module = frag, .pName = "main" } };
    
        // same buffer layout as the cube (stride = sizeof(Vertex)), but only the position is read
        const vk::VertexInputBindingDescription binding{ .binding = 0, .stride = sizeof(Vertex), .inputRate = vk::VertexInputRate::eVertex };
        const vk::VertexInputAttributeDescription attribute{ .location = 0, .binding = 0, .format = vk::Format::eR32G32B32Sfloat, .offset = offsetof(Vertex, pos) };
        const vk::PipelineVertexInputStateCreateInfo vertexInput{
            .vertexBindingDescriptionCount = 1, .pVertexBindingDescriptions = &binding,
            .vertexAttributeDescriptionCount = 1, .pVertexAttributeDescriptions = &attribute };
    
        const vk::PipelineInputAssemblyStateCreateInfo inputAssembly{ .topology = vk::PrimitiveTopology::eTriangleList };
        const vk::PipelineViewportStateCreateInfo viewport{ .viewportCount = 1, .scissorCount = 1 };
        const vk::PipelineRasterizationStateCreateInfo raster{
            .polygonMode = vk::PolygonMode::eFill, .cullMode = vk::CullModeFlagBits::eBack,
            .frontFace = vk::FrontFace::eCounterClockwise, .lineWidth = 1.0f };
        const vk::PipelineMultisampleStateCreateInfo multisample{ .rasterizationSamples = vk::SampleCountFlagBits::e1 };
        const vk::PipelineDepthStencilStateCreateInfo depth{ .depthTestEnable = VK_FALSE, .depthWriteEnable = VK_FALSE };
        const vk::PipelineColorBlendAttachmentState blendAttachment{
            .blendEnable = VK_FALSE,
            .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                              vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA };
        const vk::PipelineColorBlendStateCreateInfo blend{ .attachmentCount = 1, .pAttachments = &blendAttachment };
        const vk::DynamicState dynamicStates[] = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
        const vk::PipelineDynamicStateCreateInfo dynamic{ .dynamicStateCount = 2, .pDynamicStates = dynamicStates };
    
        // the fragment stage reads it, so the range says eFragment
        const vk::PushConstantRange pushRange{ .stageFlags = vk::ShaderStageFlagBits::eFragment, .offset = 0, .size = sizeof(glm::vec2) };
        quadLayout = device.createPipelineLayout({ .pushConstantRangeCount = 1, .pPushConstantRanges = &pushRange });
    
        // must match the render pass attachments, depth included, even though depth is unused here
        const vk::PipelineRenderingCreateInfo rendering{
            .colorAttachmentCount = 1, .pColorAttachmentFormats = &swapFormat, .depthAttachmentFormat = kDepthFormat };
    
        quadPipeline = device.createGraphicsPipeline(nullptr, {
            .pNext = &rendering, .stageCount = 2, .pStages = stages,
            .pVertexInputState = &vertexInput, .pInputAssemblyState = &inputAssembly,
            .pViewportState = &viewport, .pRasterizationState = &raster, .pMultisampleState = &multisample,
            .pDepthStencilState = &depth, .pColorBlendState = &blend, .pDynamicState = &dynamic,
            .layout = quadLayout }).value;
    
        device.destroyShaderModule(vert);
        device.destroyShaderModule(frag);
    }

    void createPipeline() {
        vk::ShaderModule vert = loadShader("cube.vert");
        vk::ShaderModule frag = loadShader("cube.frag");

        const vk::PipelineShaderStageCreateInfo stages[] = {
            { .stage = vk::ShaderStageFlagBits::eVertex,   .module = vert, .pName = "main" },
            { .stage = vk::ShaderStageFlagBits::eFragment, .module = frag, .pName = "main" } };

        const vk::VertexInputBindingDescription binding{ .binding = 0, .stride = sizeof(Vertex),
                                                         .inputRate = vk::VertexInputRate::eVertex };
        const vk::VertexInputAttributeDescription attributes[] = {
            { .location = 0, .binding = 0, .format = vk::Format::eR32G32B32Sfloat, .offset = offsetof(Vertex, pos) },
            { .location = 1, .binding = 0, .format = vk::Format::eR32G32B32Sfloat, .offset = offsetof(Vertex, normal) },
            { .location = 2, .binding = 0, .format = vk::Format::eR32G32B32Sfloat, .offset = offsetof(Vertex, color) } };
        const vk::PipelineVertexInputStateCreateInfo vertexInput{
            .vertexBindingDescriptionCount   = 1, .pVertexBindingDescriptions   = &binding,
            .vertexAttributeDescriptionCount = 3, .pVertexAttributeDescriptions = attributes };

        const vk::PipelineInputAssemblyStateCreateInfo inputAssembly{ .topology = vk::PrimitiveTopology::eTriangleList };
        const vk::PipelineViewportStateCreateInfo viewport{ .viewportCount = 1, .scissorCount = 1 };   // dynamic
        const vk::PipelineRasterizationStateCreateInfo raster{
            .polygonMode = vk::PolygonMode::eFill,
            .cullMode    = vk::CullModeFlagBits::eBack,
            .frontFace   = vk::FrontFace::eCounterClockwise,
            .lineWidth   = 1.0f };
        const vk::PipelineMultisampleStateCreateInfo multisample{ .rasterizationSamples = vk::SampleCountFlagBits::e1 };
        const vk::PipelineDepthStencilStateCreateInfo depth{
            .depthTestEnable = VK_TRUE, .depthWriteEnable = VK_TRUE, .depthCompareOp = vk::CompareOp::eLess };
        const vk::PipelineColorBlendAttachmentState blendAttachment{
            .blendEnable    = VK_FALSE,
            .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                              vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA };
        const vk::PipelineColorBlendStateCreateInfo blend{ .attachmentCount = 1, .pAttachments = &blendAttachment };

        const vk::DynamicState dynamicStates[] = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
        const vk::PipelineDynamicStateCreateInfo dynamic{ .dynamicStateCount = 2, .pDynamicStates = dynamicStates };

        const vk::PushConstantRange pushRange{ .stageFlags = vk::ShaderStageFlagBits::eVertex, .offset = 0,
                                               .size = sizeof(PushConstants) };
        meshPipelineLayout = device.createPipelineLayout({ .pushConstantRangeCount = 1, .pPushConstantRanges = &pushRange });

        // Dynamic rendering: attachment formats go here instead of in a VkRenderPass.
        const vk::PipelineRenderingCreateInfo rendering{
            .colorAttachmentCount = 1, .pColorAttachmentFormats = &swapFormat, .depthAttachmentFormat = kDepthFormat };

        auto result = device.createGraphicsPipeline(nullptr, {
            .pNext               = &rendering,
            .stageCount          = 2,
            .pStages             = stages,
            .pVertexInputState   = &vertexInput,
            .pInputAssemblyState = &inputAssembly,
            .pViewportState      = &viewport,
            .pRasterizationState = &raster,
            .pMultisampleState   = &multisample,
            .pDepthStencilState  = &depth,
            .pColorBlendState    = &blend,
            .pDynamicState       = &dynamic,
            .layout              = meshPipelineLayout });
        meshPipeline = result.value;

        device.destroyShaderModule(vert);
        device.destroyShaderModule(frag);
    }

    // ------------------------------------------------------------------ draw
    static void transition(vk::CommandBuffer cmd, vk::Image image, vk::ImageAspectFlags aspect,
                           vk::ImageLayout from, vk::ImageLayout to,
                           vk::PipelineStageFlags2 srcStage, vk::AccessFlags2 srcAccess,
                           vk::PipelineStageFlags2 dstStage, vk::AccessFlags2 dstAccess) {
        const vk::ImageMemoryBarrier2 barrier{
            .srcStageMask = srcStage, .srcAccessMask = srcAccess,
            .dstStageMask = dstStage, .dstAccessMask = dstAccess,
            .oldLayout = from, .newLayout = to,
            .srcQueueFamilyIndex = vk::QueueFamilyIgnored, .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
            .image = image, .subresourceRange = { aspect, 0, 1, 0, 1 } };
        cmd.pipelineBarrier2({ .imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &barrier });
    }

    void recordFrame(vk::CommandBuffer cmd, uint32_t imageIndex) {
        using Stage  = vk::PipelineStageFlagBits2;
        using Access = vk::AccessFlagBits2;

        cmd.reset();
        cmd.begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

        // swapchain image: undefined -> color attachment (waits on the acquire semaphore's stage)
        transition(cmd, swapImages[imageIndex], vk::ImageAspectFlagBits::eColor,
                   vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal,
                   Stage::eColorAttachmentOutput, Access::eNone,
                   Stage::eColorAttachmentOutput, Access::eColorAttachmentWrite);
        // depth: Per-frame depth image: previous use has completed because we waited
        //          for this frame slot's fence. Discard old contents and transition layout.
        transition(cmd, depthImages[frame], vk::ImageAspectFlagBits::eDepth,
                   vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthAttachmentOptimal,
                   Stage::eNone, Access::eNone,
                   Stage::eEarlyFragmentTests | Stage::eLateFragmentTests,
                   Access::eDepthStencilAttachmentRead | Access::eDepthStencilAttachmentWrite);

        const vk::RenderingAttachmentInfo color{
            .imageView   = swapViews[imageIndex],
            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp      = vk::AttachmentLoadOp::eClear,
            .storeOp     = vk::AttachmentStoreOp::eStore,
            .clearValue  = vk::ClearValue{ vk::ClearColorValue{ std::array<float, 4>{ 0.05f, 0.06f, 0.09f, 1.0f } } } };
        const vk::RenderingAttachmentInfo depth{
            .imageView   = depthViews[frame],
            .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
            .loadOp      = vk::AttachmentLoadOp::eClear,
            .storeOp     = vk::AttachmentStoreOp::eDontCare,
            .clearValue  = vk::ClearValue{ vk::ClearDepthStencilValue{ 1.0f, 0 } } };

        cmd.beginRendering({
            .renderArea           = { { 0, 0 }, extent },
            .layerCount           = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments    = &color,
            .pDepthAttachment     = &depth });
        
        cmd.setViewport(0, vk::Viewport{ 0.0f, static_cast<float>(extent.height),
                                             static_cast<float>(extent.width), -static_cast<float>(extent.height), 0.0f, 1.0f });
        cmd.setScissor(0, vk::Rect2D{ { 0, 0 }, extent });

        Context::DrawContext drawCtx = { cmd };

        { // Quad
            cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, quadPipeline);
            // (viewport + scissor as you already set them)
            const glm::vec2 resolution{ float(extent.width), float(extent.height) };
            cmd.pushConstants<glm::vec2>(quadLayout, vk::ShaderStageFlagBits::eFragment, 0, resolution);   // stage must match the range
            quadMesh.Draw(drawCtx);
            
        }

        { // Cube
            cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, meshPipeline);
            // Negative-height viewport flips Y so GLM's (y up) projection can be used unchanged.
            
            const float t = static_cast<float>(glfwGetTime());
            const glm::mat4 model = glm::rotate(glm::rotate(glm::mat4(1.0f), t * 0.9f, glm::vec3(0, 1, 0)),
                                                t * 0.5f, glm::vec3(1, 0, 0));
            const glm::mat4 view  = glm::lookAt(glm::vec3(0.0f, 1.2f, 3.2f), glm::vec3(0.0f), glm::vec3(0, 1, 0));
            const glm::mat4 proj  = glm::perspective(glm::radians(60.0f),
                                                     static_cast<float>(extent.width) / static_cast<float>(extent.height),
                                                     0.1f, 100.0f);
            const PushConstants push{ proj * view * model, model };
            cmd.pushConstants<PushConstants>(meshPipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, push);

            cubeMesh.Draw(drawCtx);
        }
        
        cmd.endRendering();

        transition(cmd, swapImages[imageIndex], vk::ImageAspectFlagBits::eColor,
                   vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR,
                   Stage::eColorAttachmentOutput, Access::eColorAttachmentWrite,
                   Stage::eNone, Access::eNone);
        cmd.end();
    }

    void drawFrame() {
        if (device.waitForFences(inFlight[frame], VK_TRUE, UINT64_MAX) != vk::Result::eSuccess)
            throw std::runtime_error("waitForFences failed");

        uint32_t imageIndex = 0;
        try {
            imageIndex = device.acquireNextImageKHR(swapchain, UINT64_MAX, imageAvailable[frame], nullptr).value;
        } catch (const vk::OutOfDateKHRError&) {
            recreateSwapchain();
            return;                                   // fence still signaled: nothing was submitted
        }
        device.resetFences(inFlight[frame]);

        recordFrame(cmds[frame], imageIndex);

        const vk::SemaphoreSubmitInfo wait{ .semaphore = imageAvailable[frame],
                                            .stageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput };
        const vk::SemaphoreSubmitInfo signal{ .semaphore = renderDone[imageIndex],
                                              .stageMask = vk::PipelineStageFlagBits2::eAllCommands };
        const vk::CommandBufferSubmitInfo cb{ .commandBuffer = cmds[frame] };
        queue.submit2(vk::SubmitInfo2{
            .waitSemaphoreInfoCount   = 1, .pWaitSemaphoreInfos   = &wait,
            .commandBufferInfoCount   = 1, .pCommandBufferInfos   = &cb,
            .signalSemaphoreInfoCount = 1, .pSignalSemaphoreInfos = &signal }, inFlight[frame]);

        vk::Result presentResult = vk::Result::eSuccess;
        try {
            presentResult = queue.presentKHR({ .waitSemaphoreCount = 1, .pWaitSemaphores = &renderDone[imageIndex],
                                               .swapchainCount = 1, .pSwapchains = &swapchain,
                                               .pImageIndices = &imageIndex });
        } catch (const vk::OutOfDateKHRError&) {
            presentResult = vk::Result::eErrorOutOfDateKHR;
        }
        if (presentResult != vk::Result::eSuccess || resized) {      // suboptimal / out of date / resized
            resized = false;
            recreateSwapchain();
        }
        frame = (frame + 1) % kFramesInFlight;
    }

    // --------------------------------------------------------------- cleanup
    void cleanup() {
        device.waitIdle();

        destroySwapchainResources();
        device.destroySwapchainKHR(swapchain);

        cubeMesh.Delete();
        device.destroyPipeline(meshPipeline);
        device.destroyPipelineLayout(meshPipelineLayout);

        quadMesh.Delete();
        device.destroyPipeline(quadPipeline);
        device.destroyPipelineLayout(quadLayout);

        for (uint32_t i = 0; i < kFramesInFlight; ++i) {
            device.destroyFence(inFlight[i]);
            device.destroySemaphore(imageAvailable[i]);
        }
        device.destroyCommandPool(cmdPool);

        vmaDestroyAllocator(allocator);      // must come before the device is destroyed
        device.destroy();
        instance.destroySurfaceKHR(surface);
        if (messenger) instance.destroyDebugUtilsMessengerEXT(messenger);
        instance.destroy();

        glfwDestroyWindow(window);
        glfwTerminate();
    }
};

}  // namespace

int main() {
    try {
        VoxelEngine::App app;
        app.run();
    } catch (const std::exception& e) {
        std::fprintf(stderr, "Fatal: %s\n", e.what());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
