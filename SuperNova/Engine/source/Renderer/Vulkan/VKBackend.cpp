#include <Engine/Renderer/Vulkan/VKBackend.hpp>
#include <Engine/Core/Assert.hpp>
#include <Engine/Renderer/Vulkan/VKShaderCompiler.hpp>

#ifdef SNV_PLATFORM_WINDOWS
    #define NOMINMAX
        #include <Windows.h>    // NOTE(v.matushkin): The fuck am I supposed to do with this windows include shit?
        //#include <windef.h>
        //#include <libloaderapi.h>
    #undef NOMINMAX
    #undef ARRAYSIZE
    #include <vulkan/vulkan_win32.h>
    #include <Engine/Application/Window.hpp>
#endif

#include <limits>


// TODO(v.matushkin):
// - <SurfaceCreation>
//   I can put surface creation in Window class and use GLFW methods, but I think this is wrong
//     and will complicate things if I wanna make Vulkan support optional at some point.
//   Probably just add Window::GetWin32Instance() method so that Windows.h will be included in only one place.
//     But if I wanna do that I need to forward declare everything that vulkan_win32.h uses.
//     Then I cann pass this HWIND and HINSTANCE to Renderer in some platform independant way,
//     to remove Window.hpp include from DX* and VK Backends
//
// - <SwapchainCreation>
//   - <Format>
//     Swapchain format shouldn't be hardcoded. Configurable? Find the best one? What to do with color space?
//     Mismatch between DX12 and VK Backends, vulkan says that RGBA8888_UNORM is unsupported on my PC,
//       which means that setting DXGI_FORMAT_R8G8B8A8_UNORM in DX backend is wrong, but then, it would be reported as error, no?
//     - https://medium.com/@heypete/hello-triangle-meet-swift-and-wide-color-6f9e246616d9
//   - <ImageCount>
//   - <ImageExtent>
//
// - <SupportChecks>
//   - validation layers
//   - instance extensions
//   - device extensions
//   - check that there is at least one swapchain format and present mode
//
// - <PresentQueue>
//   There are devices with separate Graphics and Present queues, but I'm not handling this case, cause I can't test it anyway
//
// - <AcquirePresent>
//   Make sure that I'm doing it right(synchronization). Update to synchronization2 ?
//   [LINKS]
//     - https://www.reddit.com/r/vulkan/comments/jtuhmu/synchronizing_frames_in_flight/
//     - https://vkguide.dev/docs/chapter-4/double_buffering/


// NOTE(v.matushkin): Just use c++17 std::size() or c++20 std::ssize() ?
#define ARRAYSIZE(arr) (sizeof(arr) / sizeof(arr[0]))

// NOTE(v.matushkin): The are other validation layers
// NOTE(v.matushkin): Layers can be configured, options are listed in <layer>.json, but do I need to?
const char* vk_ValidationLayers[] = {
    "VK_LAYER_KHRONOS_validation",
    "VK_LAYER_LUNARG_monitor",
    // "VK_LAYER_KHRONOS_synchronization2", // TODO(v.matushkin): I need to use this
};

const char* vk_InstanceExtensions[] = {
    VK_KHR_SURFACE_EXTENSION_NAME,

#ifdef SNV_PLATFORM_WINDOWS
    VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif

#ifdef SNV_GPU_API_DEBUG_ENABLED
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
    VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME,
    // VK_EXT_DEBUG_MARKER_EXTENSION_NAME
#endif
};

const char* vk_DeviceExtensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

const VkFormat k_SwapchainFormat    = VK_FORMAT_B8G8R8A8_UNORM;
// TODO(v.matushkin): <RenderGraph>
static bool g_IsPipelineInitialized = false;


#ifdef SNV_GPU_API_DEBUG_ENABLED
// NOTE(v.matushkin): There is a lot more info in callbackData than I'm logging
// NOTE(v.matushkin): It would be good if I could format log string once and then use appropriate LOG_* function
//  may be I can just pass string formatted with fmtlib?
// NOTE(v.matushkin): Why the fuck pMessage includes messageIdNumber and pMessageIdName
static VKAPI_ATTR VkBool32 VKAPI_CALL vk_DebugMessageCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT             messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
    void* /*user_data*/
)
{
    // NOTE(v.matushkin): Ignore this spamming fuck
    if (std::strcmp(callbackData->pMessageIdName, "Loader Message") == 0)
    {
        return VK_FALSE;
    }

    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
    {
        // TODO(v.matushkin): should be logged with LOG_TRACE, but it doesn't work right now
        LOG_INFO(callbackData->pMessage);
    }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
    {
        LOG_INFO(callbackData->pMessage);
    }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        LOG_WARN(callbackData->pMessage);
    }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        LOG_ERROR(callbackData->pMessage);
    }

    return VK_FALSE;
}

static VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugUtilsMessengerEXT(
    VkInstance                                instance,
    const VkDebugUtilsMessengerCreateInfoEXT* create_info,
    const VkAllocationCallbacks*              alloc_callbacks,
    VkDebugUtilsMessengerEXT*                 debug_messenger)
{
    auto func =
        reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
    if (func == nullptr)
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }

    return func(instance, create_info, alloc_callbacks, debug_messenger);
}

static VKAPI_ATTR void VKAPI_CALL vkDestroyDebugUtilsMessengerEXT(
    VkInstance                   instance,
    VkDebugUtilsMessengerEXT     messenger,
    const VkAllocationCallbacks* pAllocator
)
{
    auto func =
        reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
    if (func != nullptr)
    {
        func(instance, messenger, pAllocator);
    }
}
#endif // SNV_GPU_API_DEBUG_ENABLED


namespace snv
{

VKBackend::VKBackend()
    : m_currentFrame(0)
{
    VKShaderCompiler::Init();

    CreateInstance();
#ifdef SNV_GPU_API_DEBUG_ENABLED
    CreateDebugUtilsMessenger();
#endif
    CreateSurface();
    CreateDevice();
    CreateSwapchain();
    CreateRenderPass();
    CreateFramebuffers();
    CreateCommandPool();
    CreateSyncronizationObjects();
}

VKBackend::~VKBackend()
{
    VKShaderCompiler::Shutdown();

    vkQueueWaitIdle(m_graphicsQueue);

    for (ui32 i = 0; i < k_BackBufferFrames; ++i)
    {
        vkDestroySemaphore(m_device, m_semaphoreImageAvailable[i], nullptr);
        vkDestroySemaphore(m_device, m_semaphoreRenderFinished[i], nullptr);

        vkDestroyFence(m_device, m_fences[i], nullptr);
    }

    vkDestroyCommandPool(m_device, m_commandPool, nullptr);

    // NOTE(v.matushkin): Delete them afetr pipeline creation?
    auto& shader = m_shaders.begin()->second;
    vkDestroyShaderModule(m_device, shader.Vertex, nullptr);
    vkDestroyShaderModule(m_device, shader.Fragment, nullptr);

    for (ui32 i = 0; i < k_BackBufferFrames; ++i)
    {
        vkDestroyFramebuffer(m_device, m_framebuffers[i], nullptr);
    }

    vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
    vkDestroyRenderPass(m_device, m_renderPass, nullptr);

    for (ui32 i = 0; i < k_BackBufferFrames; ++i)
    {
        vkDestroyImageView(m_device, m_backBuffers[i], nullptr);
    }
    vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);

    vkDestroyDevice(m_device, nullptr);
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
#ifdef SNV_GPU_API_DEBUG_ENABLED
    vkDestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
#endif
    vkDestroyInstance(m_instance, nullptr);
}


void VKBackend::EnableBlend()
{}

void VKBackend::EnableDepthTest()
{}

void VKBackend::SetBlendFunction(BlendFactor source, BlendFactor destination)
{}

void VKBackend::SetClearColor(f32 r, f32 g, f32 b, f32 a)
{
    VkClearColorValue vkClearColorValue = {
        .float32 = {r, g, b, a},
    };
    m_clearValue.color = vkClearColorValue;
}

void VKBackend::SetDepthFunction(DepthFunction depthFunction)
{}

void VKBackend::SetViewport(i32 x, i32 y, i32 width, i32 height)
{}


void VKBackend::Clear(BufferBit bufferBitMask)
{}


void VKBackend::BeginFrame(const glm::mat4x4& localToWorld, const glm::mat4x4& cameraView, const glm::mat4x4& cameraProjection)
{
    // TODO(v.matushkin): <RenderGraph>
    if (g_IsPipelineInitialized == false)
    {
        g_IsPipelineInitialized = true;
        CreatePipeline();
        CreateCommandBuffers();
    }

    // NOTE(v.mnatushkin): 0.04s, this will break for FPS <30, I'm sure I'm doing this acquire/present thing wrong
    const auto timeout = 40'000'000;

    auto semaphoreImageAvailable = m_semaphoreImageAvailable[m_currentFrame];

    vkAcquireNextImageKHR(m_device, m_swapchain, timeout, semaphoreImageAvailable, nullptr, &m_currentBackBufferIndex);

    auto fence = m_fences[m_currentBackBufferIndex];
    vkWaitForFences(m_device, 1, &fence, VK_TRUE, timeout);
    vkResetFences(m_device, 1, &fence);

    const VkPipelineStageFlags vkPipelineStageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    // NOTE(v.matushkin): VkSubmitInfo2KHR ?
    VkSubmitInfo vkSubmitInfo = {
        .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext                = nullptr,
        .waitSemaphoreCount   = 1,
        .pWaitSemaphores      = &semaphoreImageAvailable,
        .pWaitDstStageMask    = &vkPipelineStageFlags,
        .commandBufferCount   = 1,
        .pCommandBuffers      = &m_commandBuffers[m_currentBackBufferIndex],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores    = &m_semaphoreRenderFinished[m_currentFrame],
    };

    vkQueueSubmit(m_graphicsQueue, 1, &vkSubmitInfo, fence);
}

void VKBackend::EndFrame()
{
    auto semaphoreRenderFinished = m_semaphoreRenderFinished[m_currentFrame];

    VkPresentInfoKHR vkPresentInfo = {
        .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext              = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = &semaphoreRenderFinished,
        .swapchainCount     = 1,
        .pSwapchains        = &m_swapchain,
        .pImageIndices      = &m_currentBackBufferIndex,
        .pResults           = nullptr,
    };
    vkQueuePresentKHR(m_graphicsQueue, &vkPresentInfo);

    m_currentFrame = (m_currentFrame + 1) % k_BackBufferFrames;
}

void VKBackend::DrawBuffer(TextureHandle textureHandle, BufferHandle bufferHandle, i32 indexCount, i32 vertexCount)
{}

void VKBackend::DrawArrays(i32 count)
{}

void VKBackend::DrawElements(i32 count)
{}


BufferHandle VKBackend::CreateBuffer(
    std::span<const std::byte>              indexData,
    std::span<const std::byte>              vertexData,
    const std::vector<VertexAttributeDesc>& vertexLayout
)
{
    return BufferHandle();
}

TextureHandle VKBackend::CreateTexture(const TextureDesc& textureDesc, const ui8* textureData)
{
    return TextureHandle();
}

ShaderHandle VKBackend::CreateShader(std::span<const char> vertexSource, std::span<const char> fragmentSource)
{
    const auto vertexBytecode   = VKShaderCompiler::CompileShader(VKShaderCompiler::ShaderType::Vertex, vertexSource);
    const auto fragmentBytecode = VKShaderCompiler::CompileShader(VKShaderCompiler::ShaderType::Fragment, fragmentSource);

    VKShader shader;

    VkShaderModuleCreateInfo vkVertexShaderInfo = {
       .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
       .pNext    = nullptr,
       .flags    = 0,
       .codeSize = vertexBytecode.size() * sizeof(ui32),
       .pCode    = vertexBytecode.data(),
    };
    vkCreateShaderModule(m_device, &vkVertexShaderInfo, nullptr, &shader.Vertex);

    VkShaderModuleCreateInfo vkFragmentShaderInfo = {
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext    = nullptr,
        .flags    = 0,
        .codeSize = fragmentBytecode.size() * sizeof(ui32),
        .pCode    = fragmentBytecode.data(),
    };
    vkCreateShaderModule(m_device, &vkFragmentShaderInfo, nullptr, &shader.Fragment);

    static ui32 shader_handle_workaround = 0;
    auto        shaderHandle             = static_cast<ShaderHandle>(shader_handle_workaround++);

    m_shaders[shaderHandle] = shader;

    return shaderHandle;
}


void VKBackend::CreateInstance()
{
    VkApplicationInfo vkApplicationInfo = {
        .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext              = nullptr,
        .pApplicationName   = "SuperNova",
        .applicationVersion = VK_MAKE_VERSION(0, 1, 0),
        .pEngineName        = "SuperNova",
        .engineVersion      = VK_MAKE_VERSION(0, 1, 0),
        .apiVersion         = VK_MAKE_API_VERSION(0, 1, 2, 189),
    };
    VkInstanceCreateInfo vkInstanceInfo = {
        .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .flags                   = 0, // SPEC: reserved for future use
        .pApplicationInfo        = &vkApplicationInfo,
        .enabledExtensionCount   = ARRAYSIZE(vk_InstanceExtensions),
        .ppEnabledExtensionNames = vk_InstanceExtensions,
    };

#ifdef SNV_GPU_API_DEBUG_ENABLED
    //- Set Validation Layers
    vkInstanceInfo.enabledLayerCount   = ARRAYSIZE(vk_ValidationLayers);
    vkInstanceInfo.ppEnabledLayerNames = vk_ValidationLayers;

    //- Enable Best Practices Validation extension
    // VkValidationFeatureEnableEXT vkValidationFeaturesEnable[] = {
    //     VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
    //     VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT,
    //     VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
    // };
    // 
    // VkValidationFeaturesEXT vkValidationFeatures = {
    //     .sType                          = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
    //     .pNext                          = nullptr,
    //     .enabledValidationFeatureCount  = ARRAYSIZE(vkValidationFeaturesEnable),
    //     .pEnabledValidationFeatures     = vkValidationFeaturesEnable,
    //     .disabledValidationFeatureCount = 0,
    //     .pDisabledValidationFeatures    = nullptr,
    // };

    //- Create Instance Debug Messenger
    auto vkDebugUtilsMessengerInfo  = CreateDebugUtilsMessengerInfo();
    // vkDebugUtilsMessengerInfo.pNext = &vkValidationFeatures;

    vkInstanceInfo.pNext = &vkDebugUtilsMessengerInfo;
#endif

    vkCreateInstance(&vkInstanceInfo, nullptr, &m_instance);
}

void VKBackend::CreateSurface()
{
#ifdef SNV_PLATFORM_WINDOWS
    VkWin32SurfaceCreateInfoKHR vkWin32SurfaceInfo = {
        .sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .pNext     = nullptr,
        .flags     = 0, // SPEC: reserved for future use
        .hinstance = GetModuleHandle(nullptr),
        .hwnd      = Window::GetWin32Window(),
    };
    vkCreateWin32SurfaceKHR(m_instance, &vkWin32SurfaceInfo, nullptr, &m_surface);
#endif // SNV_PLATFORM_WINDOWS
}

void VKBackend::CreateDevice()
{
    //- Enumerate Physical Devices
    ui32 vkPhysicalDeviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &vkPhysicalDeviceCount, nullptr);

    SNV_ASSERT(vkPhysicalDeviceCount > 0, "No VkPhysicalDevice found");

    auto vkPhysicalDevices = new VkPhysicalDevice[vkPhysicalDeviceCount];
    vkEnumeratePhysicalDevices(m_instance, &vkPhysicalDeviceCount, vkPhysicalDevices);

     //- Pick suitable Physical Device
    VkPhysicalDevice vkPhysicalDevice = nullptr;
    ui32             vkGraphicsQueueFamily;
    for (ui32 i = 0; i < vkPhysicalDeviceCount; ++i)
    {
        const auto physicalDevice = vkPhysicalDevices[i];
        if (IsPhysicalDeviceSuitable(physicalDevice, vkGraphicsQueueFamily))
        {
            vkPhysicalDevice = physicalDevice;
            break;
        }
    }
    delete[] vkPhysicalDevices;
    SNV_ASSERT(vkPhysicalDevice != nullptr, "Failed to find a suitable GPU");

    m_physiacalDevice     = vkPhysicalDevice;
    m_graphicsQueueFamily = vkGraphicsQueueFamily;

    //- Create VkDevice with graphics queue
    const f32               k_QueuePriority   = 1.0f;
    // NOTE(v.matushkin): Use VkDeviceQueueGlobalPriorityCreateInfoEXT ?
    VkDeviceQueueCreateInfo vkDeviceQueueInfo = {
        .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext            = nullptr,
        .flags            = 0,
        .queueFamilyIndex = m_graphicsQueueFamily,
        .queueCount       = 1,
        .pQueuePriorities = &k_QueuePriority,
    };
    VkPhysicalDeviceFeatures vkPhysicalDeviceFeatures{};
    VkDeviceCreateInfo vkDeviceInfo = {
        .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext                   = nullptr,
        //.flags                   =,
        .queueCreateInfoCount    = 1,
        .pQueueCreateInfos       = &vkDeviceQueueInfo,
        .enabledLayerCount       = 0,
        .ppEnabledLayerNames     = nullptr,
        .enabledExtensionCount   = ARRAYSIZE(vk_DeviceExtensions),
        .ppEnabledExtensionNames = vk_DeviceExtensions,
        .pEnabledFeatures        = &vkPhysicalDeviceFeatures,
    };

    vkCreateDevice(m_physiacalDevice, &vkDeviceInfo, nullptr, &m_device);
    // NOTE(v.matushkin): Use vkGetDeviceQueue2() ?
    vkGetDeviceQueue(m_device, m_graphicsQueueFamily, 0, &m_graphicsQueue);
}

void VKBackend::CreateSwapchain()
{
    //- Surface format
    // TODO(v.matushkin): <SwapchainCreation/Format>
    // ui32 vkSurfaceFormatCount;
    // vkGetPhysicalDeviceSurfaceFormatsKHR(m_physiacalDevice, m_surface, &vkSurfaceFormatCount, nullptr);
    // auto vkSurfaceFormats = new VkSurfaceFormatKHR[vkSurfaceFormatCount];
    // vkGetPhysicalDeviceSurfaceFormatsKHR(m_physiacalDevice, m_surface, &vkSurfaceFormatCount, vkSurfaceFormats);
    // delete[] vkSurfaceFormats;

    VkSurfaceFormatKHR vkSurfaceFormat = {
        .format     = k_SwapchainFormat,
        .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
    };

    //- Surface capabilities
    // TODO(v.matushkin): Use vkGetPhysicalDeviceSurfaceCapabilities2KHR for FullscreenExclusive on windows
    VkSurfaceCapabilitiesKHR vkSurfaceCapabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physiacalDevice, m_surface, &vkSurfaceCapabilities);

    //-- Image Count
    ui32 vkMinImageCount = k_BackBufferFrames;
    {
        const auto minImageCount = vkSurfaceCapabilities.minImageCount;
        const auto maxImageCount = vkSurfaceCapabilities.maxImageCount;
        if (vkMinImageCount < minImageCount || (maxImageCount > 0 && maxImageCount < vkMinImageCount))
        {
            // TODO(v.matushkin): <SwapchainCreation/ImageCount>
            SNV_ASSERT(false, "<SwapchainCreation/ImageCount>");
        }
    }

    //-- Image Extent
    {
        const auto vkCurrentExtent = vkSurfaceCapabilities.currentExtent;
        if (vkCurrentExtent.width == std::numeric_limits<ui32>::max())
        {
            // TODO(v.matushkin): <SwapchainCreation/ImageExtent>
            SNV_ASSERT(false, "Do glfwGetFramebufferSize");
        }
        else
        {
            m_swapchainExtent = vkCurrentExtent;
        }
    }

    //-- Pre Transform
    // NOTE(v.matushkin): Which one?
    const auto vkPreTransform = vkSurfaceCapabilities.currentTransform;
    // const auto vkPreTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    if (!(vkSurfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR))
    {
        // NOTE(v.matushkin): Doubt I will have this fired, so I will not learn what it is
        LOG_WARN("surface doesn't support VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR");
    }
    //-- Composite Alpha
    const auto vkCompositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    if (!(vkSurfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR))
    {
        // NOTE(v.matushkin): Doubt I will have this fired, so I will not learn what it is
        LOG_WARN("surface doesn't support VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR");
    }

    //- Present Mode
    VkPresentModeKHR vkPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    {
        ui32 vkPresentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(m_physiacalDevice, m_surface, &vkPresentModeCount, nullptr);
        auto vkPresentModes = new VkPresentModeKHR[vkPresentModeCount];
        vkGetPhysicalDeviceSurfacePresentModesKHR(m_physiacalDevice, m_surface, &vkPresentModeCount, vkPresentModes);

        for (ui32 i = 0; i < vkPresentModeCount; ++i)
        {
            if (vkPresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                vkPresentMode = vkPresentModes[i];
            }
        }
        delete[] vkPresentModes;
    }

    //- Create Swapchain
    VkSwapchainCreateInfoKHR vkSwapchainInfo = {
        .sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext                 = nullptr,
        .flags                 = 0,
        .surface               = m_surface,
        .minImageCount         = vkMinImageCount,
        .imageFormat           = vkSurfaceFormat.format,
        .imageColorSpace       = vkSurfaceFormat.colorSpace,
        .imageExtent           = m_swapchainExtent,
        .imageArrayLayers      = 1, // This is always 1 unless you are developing a stereoscopic 3D application
        .imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, // NOTE(v.matushkin): When to use VK_IMAGE_USAGE_TRANSFER_DST_BIT?
        .imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE, // NOTE(v.matushkin): <PresentQueue>
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices   = nullptr,
        .preTransform          = vkPreTransform,
        .compositeAlpha        = vkCompositeAlpha, // NOTE(v.matushkin): Blending with other windows
        .presentMode           = vkPresentMode,
        .clipped               = VK_TRUE, // Don't care about pixels that are obscured (e.g. other window is in front of them)
        .oldSwapchain          = nullptr, // TODO(v.matushkin): Use for swapchain recreation
    };
    vkCreateSwapchainKHR(m_device, &vkSwapchainInfo, nullptr, &m_swapchain);

    //- Get Swapchain Images
    ui32 vkSwapchainImageCount;
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &vkSwapchainImageCount, nullptr);
    SNV_ASSERT(vkSwapchainImageCount == k_BackBufferFrames, "<SwapchainCreation/ImageCount>");
    VkImage vkSwapchainImages[k_BackBufferFrames];
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &vkSwapchainImageCount, vkSwapchainImages);

    //- Create Swapchain Image Views
    VkComponentMapping vkComponentMapping = {
        .r = VK_COMPONENT_SWIZZLE_R,
        .g = VK_COMPONENT_SWIZZLE_G,
        .b = VK_COMPONENT_SWIZZLE_B,
        .a = VK_COMPONENT_SWIZZLE_A,
    };
    VkImageSubresourceRange vkImageSubresourceRange = {
        .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel   = 0,
        .levelCount     = 1,
        .baseArrayLayer = 0, // Layers for stereographic 3D application
        .layerCount     = 1,
    };
    VkImageViewCreateInfo vkImageViewInfo = {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext            = nullptr,
        .flags            = 0,
        .viewType         = VK_IMAGE_VIEW_TYPE_2D,
        .format           = vkSurfaceFormat.format,
        .components       = vkComponentMapping,
        .subresourceRange = vkImageSubresourceRange,
    };
    for (ui32 i = 0; i < vkSwapchainImageCount; ++i)
    {
        vkImageViewInfo.image = vkSwapchainImages[i];
        vkCreateImageView(m_device, &vkImageViewInfo, nullptr, &m_backBuffers[i]);
    }
}

void VKBackend::CreateRenderPass()
{
    VkAttachmentDescription2 vkColorAttachmentDescription = {
        .sType          = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2,
        .pNext          = nullptr, // NOTE(v.matushkin): Can be used for some additional shit
        .flags          = 0,
        .format         = k_SwapchainFormat,
        .samples        = VK_SAMPLE_COUNT_1_BIT,
        .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };

    // NOTE(v.matushkin): separateDepthStencilLayouts ?
    VkAttachmentReference2 vkColorAttachmentRef = {
        .sType      = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
        .pNext      = nullptr,
        .attachment = 0,
        .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        // .aspectMask = , // NOTE(v.matushkin): The fuck is this?
    };
    VkSubpassDescription2 vkSubpassDescription = {
        .sType                   = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2,
        // NOTE(v.matushkin): VkFragmentShadingRateAttachmentInfoKHR, VkSubpassDescriptionDepthStencilResolve
        .pNext                   = nullptr,
        .flags                   = 0,
        .pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
        // .viewMask                = , // NOTE(v.matushkin): the fuck is multiview?
        // .inputAttachmentCount    = ,
        // .pInputAttachments       = ,
        .colorAttachmentCount    = 1,
        .pColorAttachments       = &vkColorAttachmentRef,
        // .pResolveAttachments     = ,
        // .pDepthStencilAttachment = ,
        // .preserveAttachmentCount = ,
        // .pPreserveAttachments    = ,
    };

    VkSubpassDependency2 vkSubpassDependency = {
        .sType           = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2,
        .pNext           = nullptr,
        .srcSubpass      = VK_SUBPASS_EXTERNAL,
        .dstSubpass      = 0,
        .srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask   = 0,
        .dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        // .dependencyFlags = ,
        // .viewOffset      = ,
    };

    VkRenderPassCreateInfo2 vkRenderPassInfo = {
        .sType                   = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2,
        .pNext                   = nullptr,
        .flags                   = 0,
        .attachmentCount         = 1,
        .pAttachments            = &vkColorAttachmentDescription,
        .subpassCount            = 1,
        .pSubpasses              = &vkSubpassDescription,
        .dependencyCount         = 1,
        .pDependencies           = &vkSubpassDependency,
        // .correlatedViewMaskCount = ,
        // .pCorrelatedViewMasks    = ,
    };
    vkCreateRenderPass2(m_device, &vkRenderPassInfo, nullptr, &m_renderPass);
}

void VKBackend::CreateFramebuffers()
{
    VkFramebufferCreateInfo vkFramebufferInfo = {
        .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .pNext           = nullptr,
        .flags           = 0,
        .renderPass      = m_renderPass,
        .attachmentCount = 1,
        .width           = m_swapchainExtent.width,
        .height          = m_swapchainExtent.height,
        .layers          = 1,
    };

    for (ui32 i = 0; i < k_BackBufferFrames; ++i)
    {
        vkFramebufferInfo.pAttachments = &m_backBuffers[i];
        vkCreateFramebuffer(m_device, &vkFramebufferInfo, nullptr, &m_framebuffers[i]);
    }
}

void VKBackend::CreatePipeline()
{
    //- ShaderStages
    auto& shader = m_shaders.begin()->second;

    VkPipelineShaderStageCreateInfo vkShaderStages[] = {
        // Vertex
        {
            .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext  = nullptr,
            .flags  = 0,
            .stage  = VK_SHADER_STAGE_VERTEX_BIT,
            .module = shader.Vertex,
            .pName  = "main",           // NOTE(v.matushkin): Shouldn't be hardcoded?
            // .pSpecializationInfo = , // NOTE(v.matushkin): For shader constants
        },
        // Fragment
        {
            .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext  = nullptr,
            .flags  = 0,
            .stage  = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = shader.Fragment,
            .pName  = "main",
        },
    };

    //- VertexInput
    // NOTE(v.matushkin): This will be complicated
    /*VkVertexInputBindingDescription vkVertexInputBindingDescription = {
        .binding   = 0,
        .stride    = ,
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };
    VkVertexInputAttributeDescription vkVertexInputAttributeDescription = {
        .location = ,
        .binding  = ,
        .format   = ,
        .offset   = ,
    };*/
    VkPipelineVertexInputStateCreateInfo vkVertexInputState = {
        .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext                           = nullptr,
        .flags                           = 0, // SPEC: reserved for future use
        .vertexBindingDescriptionCount   = 0,
        .pVertexBindingDescriptions      = nullptr,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions    = nullptr,
    };

    //- InputAssembly
    VkPipelineInputAssemblyStateCreateInfo vkInputAssemblyState = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext                  = nullptr,
        .flags                  = 0, // SPEC: reserved for future use
        .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    };

    //- Viewport
    VkViewport vkViewport = {
        .x        = 0.0f,
        .y        = 0.0f,
        .width    = static_cast<f32>(m_swapchainExtent.width),
        .height   = static_cast<f32>(m_swapchainExtent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    VkRect2D vkScissor = {
        .offset = {0,0},
        .extent = m_swapchainExtent,
    };
    VkPipelineViewportStateCreateInfo vkViewportState = {
        .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext         = nullptr,
        .flags         = 0, // SPEC: reserved for future use
        .viewportCount = 1,
        .pViewports    = &vkViewport,
        .scissorCount  = 1,
        .pScissors     = &vkScissor,
    };

    //- Rasterizer
    VkPipelineRasterizationStateCreateInfo vkRasterizationState = {
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext                   = nullptr,
        .flags                   = 0, // SPEC: reserved for future use
        .depthClampEnable        = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode             = VK_POLYGON_MODE_FILL,
        .cullMode                = VK_CULL_MODE_BACK_BIT,
        .frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable         = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp          = 0.0f,
        .depthBiasSlopeFactor    = 0.0f,
        .lineWidth               = 1.0f,
    };

    //- Multisample
    VkPipelineMultisampleStateCreateInfo vkMultisampleState = {
        .sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext                 = nullptr,
        .flags                 = 0, // SPEC: reserved for future use
        .rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable   = VK_FALSE,
        .minSampleShading      = 1.0f,
        .pSampleMask           = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable      = VK_FALSE,
    };

    //- Depth/Stencil
    // VkPipelineDepthStencilStateCreateInfo vkDepthStencilState = {
    //     .sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
    //     .pNext                 = nullptr,
    //     .flags                 = 0, // SPEC: reserved for future use
    //     .depthTestEnable       = ,
    //     .depthWriteEnable      = ,
    //     .depthCompareOp        = ,
    //     .depthBoundsTestEnable = ,
    //     .stencilTestEnable     = ,
    //     .front                 = ,
    //     .back                  = ,
    //     .minDepthBounds        = ,
    //     .maxDepthBounds        = ,
    // };

    //- ColorBlend
    // NOTE(v.matushkin): VkPipelineColorBlendAttachmentState - per framebuffer, VkPipelineColorBlendStateCreateInfo - global
    VkPipelineColorBlendAttachmentState vkColorBlendAttachment = {
        .blendEnable         = VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .colorBlendOp        = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp        = VK_BLEND_OP_ADD,
        .colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };
    VkPipelineColorBlendStateCreateInfo vkColorBlendState = {
        .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext           = nullptr,
        .flags           = 0, // SPEC: reserved for future use
        .logicOpEnable   = VK_FALSE,
        .logicOp         = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments    = &vkColorBlendAttachment,
        .blendConstants  = {0.0f, 0.0f, 0.0f, 0.0f},
    };

    //- DynamicState
    // NOTE(v.matushkin): Not needed right now
    // VkPipelineDynamicStateCreateInfo vkDynamicStateInfo;

    //- Create VkPipelineLayout
    VkPipelineLayoutCreateInfo vkPipelineLayoutInfo = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext                  = nullptr,
        .flags                  = 0, // SPEC: reserved for future use
        .setLayoutCount         = 0,
        .pSetLayouts            = nullptr,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges    = nullptr,
    };
    vkCreatePipelineLayout(m_device, &vkPipelineLayoutInfo, nullptr, &m_pipelineLayout);

    //- Create GraphicsPipeline
    VkGraphicsPipelineCreateInfo vkGraphicsPipelineInfo = {
        .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext               = nullptr,
        .flags               = 0, // NOTE(v.matushkin): There are a lot of them
        .stageCount          = ARRAYSIZE(vkShaderStages),
        .pStages             = vkShaderStages,
        .pVertexInputState   = &vkVertexInputState,
        .pInputAssemblyState = &vkInputAssemblyState,
        .pTessellationState  = nullptr,
        .pViewportState      = &vkViewportState,
        .pRasterizationState = &vkRasterizationState,
        .pMultisampleState   = &vkMultisampleState,
        .pDepthStencilState  = nullptr,
        .pColorBlendState    = &vkColorBlendState,
        .pDynamicState       = nullptr,
        .layout              = m_pipelineLayout,
        .renderPass          = m_renderPass,
        .subpass             = 0,
        .basePipelineHandle  = nullptr, // NOTE(v.matushkin): create a new graphics pipeline by deriving from an existing pipeline
        .basePipelineIndex   = -1,
    };
    // NOTE(v.matushkin): VkPipelineCache ?
    vkCreateGraphicsPipelines(m_device, nullptr, 1, &vkGraphicsPipelineInfo, nullptr, &m_graphicsPipeline);
}

void VKBackend::CreateCommandPool()
{
    // NOTE(v.matushkin): VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT ?
    //  what do they use un Vulkan-Samples ?
    VkCommandPoolCreateInfo vkCommandPoolInfo = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext            = nullptr,
        .flags            = 0,
        .queueFamilyIndex = m_graphicsQueueFamily,
    };
    vkCreateCommandPool(m_device, &vkCommandPoolInfo, nullptr, &m_commandPool);
}

void VKBackend::CreateCommandBuffers()
{
    VkCommandBufferAllocateInfo vkCommandBufferInfo = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext              = nullptr,
        .commandPool        = m_commandPool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = k_BackBufferFrames,
    };
    vkAllocateCommandBuffers(m_device, &vkCommandBufferInfo, m_commandBuffers);

    // NOTE(v.matushkin): VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT ?
    VkCommandBufferBeginInfo vkCommandBufferBegin = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext            = nullptr,
        .flags            = 0,
        .pInheritanceInfo = nullptr, // NOTE(v.matushkin): For secondary command buffers
    };
    VkRect2D vkRenderArea = {
        .offset = {0, 0},
        .extent = m_swapchainExtent,
    };
    VkRenderPassBeginInfo vkRenderPassBegin = {
        .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext           = nullptr,
        .renderPass      = m_renderPass,
        .renderArea      = vkRenderArea,
        .clearValueCount = 1,
        .pClearValues    = &m_clearValue,
    };

    for (ui32 i = 0; i < k_BackBufferFrames; ++i)
    {
        vkRenderPassBegin.framebuffer = m_framebuffers[i];
        auto commandBuffer            = m_commandBuffers[i];

        vkBeginCommandBuffer(commandBuffer, &vkCommandBufferBegin);
        // NOTE(v.matushkin): vkCmdBeginRenderPass2 ?
        vkCmdBeginRenderPass(commandBuffer, &vkRenderPassBegin, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

        vkCmdDraw(commandBuffer, 3, 1, 0, 0);

        vkCmdEndRenderPass(commandBuffer);
        vkEndCommandBuffer(commandBuffer);
    }
}

void VKBackend::CreateSyncronizationObjects()
{
    VkSemaphoreCreateInfo vkSemaphoreInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0, // SPEC: reserved for future use
    };
    VkFenceCreateInfo vkFenceInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };

    for (ui32 i = 0; i < k_BackBufferFrames; ++i)
    {
        vkCreateSemaphore(m_device, &vkSemaphoreInfo, nullptr, &m_semaphoreImageAvailable[i]);
        vkCreateSemaphore(m_device, &vkSemaphoreInfo, nullptr, &m_semaphoreRenderFinished[i]);

        vkCreateFence(m_device, &vkFenceInfo, nullptr, &m_fences[i]);
    }
}


#ifdef SNV_GPU_API_DEBUG_ENABLED
VkDebugUtilsMessengerCreateInfoEXT VKBackend::CreateDebugUtilsMessengerInfo()
{
    VkDebugUtilsMessageSeverityFlagsEXT vkSeverityFlags = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
                                                        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                                                        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                                                        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    VkDebugUtilsMessageTypeFlagsEXT vkTypeFlags = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                                                | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                                                | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

    VkDebugUtilsMessengerCreateInfoEXT vkDebugUtilsMessengerInfo = {
        .sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .pNext           = nullptr,
        .flags           = 0, // SPEC: reserved for future use
        .messageSeverity = vkSeverityFlags,
        .messageType     = vkTypeFlags,
        .pfnUserCallback = vk_DebugMessageCallback,
        .pUserData       = nullptr,
    };

    return vkDebugUtilsMessengerInfo;
}

void VKBackend::CreateDebugUtilsMessenger()
{
    const auto vkDebugUtilsMessengerInfo = CreateDebugUtilsMessengerInfo();
    vkCreateDebugUtilsMessengerEXT(m_instance, &vkDebugUtilsMessengerInfo, nullptr, &m_debugMessenger);
}
#endif // SNV_GPU_API_DEBUG_ENABLED


// NOTE(v.matushkin): Check for device extensions support?
bool VKBackend::IsPhysicalDeviceSuitable(VkPhysicalDevice physicalDevice, ui32& graphicsQueueFamily)
{
    VkPhysicalDeviceProperties vkDeviceProperties;
    //VkPhysicalDeviceFeatures   vkDeviceFeatures;
    vkGetPhysicalDeviceProperties(physicalDevice, &vkDeviceProperties);
    //vkGetPhysicalDeviceFeatures(physicalDevice, &vkDeviceFeatures);

    if (vkDeviceProperties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {
        return false;
    }

    //- Enumerate Physical Device Queues
    ui32 vkQueueFamilyPropertyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &vkQueueFamilyPropertyCount, nullptr);

    if (vkQueueFamilyPropertyCount < 1)
    {
        return false;
    }

    auto vkQueueFamilyProperties = new VkQueueFamilyProperties[vkQueueFamilyPropertyCount];
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &vkQueueFamilyPropertyCount, vkQueueFamilyProperties);

    //- Find suitable Physical Device Queue
    ui32 graphicsQueue;
    ui32 i = 0;
    for (; i < vkQueueFamilyPropertyCount; ++i)
    {
        const auto& vkQueueFamilyProperty = vkQueueFamilyProperties[i];

        VkBool32 supportsPresent = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, m_surface, &supportsPresent);
        // NOTE(v.matushkin): <PresentQueue>
        if (supportsPresent && (vkQueueFamilyProperty.queueFlags & VK_QUEUE_GRAPHICS_BIT))
        {
            graphicsQueue = i;
            break;
        }
    }
    delete[] vkQueueFamilyProperties;

    if (i == vkQueueFamilyPropertyCount)
    {
        return false;
    }

    graphicsQueueFamily = graphicsQueue;

    return true;
}


//void VKBackend::EnumerateInstanceLayerProperties()
//{
//    ui32 vkLayerCount;
//    vkEnumerateInstanceLayerProperties(&vkLayerCount, nullptr);
//    auto vkLayerProperties = new VkLayerProperties[vkLayerCount];
//    vkEnumerateInstanceLayerProperties(&vkLayerCount, vkLayerProperties);
//    for (ui32 i = 0; i < vkLayerCount; ++i)
//    {
//        const auto& vkLayerProperty = vkLayerProperties[i];
//        LOG_INFO("{}, Name: {}\n\tDescription: {}", i, vkLayerProperty.layerName, vkLayerProperty.description);
//    }
//    delete[] vkLayerProperties;
//}

//void VKBackend::EnumerateInstanceExtensionProperties()
//{
//    ui32 vkExtensionCount;
//    vkEnumerateInstanceExtensionProperties(nullptr, &vkExtensionCount, nullptr);
//    auto vkExtensionProperties = new VkExtensionProperties[vkExtensionCount];
//    vkEnumerateInstanceExtensionProperties(nullptr, &vkExtensionCount, vkExtensionProperties);
//    for (ui32 i = 0; i < vkExtensionCount; ++i)
//    {
//        const auto& vkExtensionProperty = vkExtensionProperties[i];
//        LOG_INFO("{}, Name: {} | Spec version: {}", i, vkExtensionProperty.extensionName, vkExtensionProperty.specVersion);
//    }
//    delete[] vkExtensionProperties;
//}

//void VKBackend::CheckDeviceExtensionsSupport(VkPhysicalDevice physicalDevice)
//{
//    ui32 vkDeviceExtensionsCount;
//    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &vkDeviceExtensionsCount, nullptr);
//    auto vkDeviceExtensions = new VkExtensionProperties[vkDeviceExtensionsCount];
//    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &vkDeviceExtensionsCount, vkDeviceExtensions);
//    for (ui32 i = 0; i < vkDeviceExtensionsCount; ++i)
//    {
//        const auto& vkDeviceExtension = vkDeviceExtensions[i];
//        LOG_INFO("{}: {}", i, vkDeviceExtension.extensionName);
//    }
//    delete[] vkDeviceExtensions;
//}

} // namespace snv
