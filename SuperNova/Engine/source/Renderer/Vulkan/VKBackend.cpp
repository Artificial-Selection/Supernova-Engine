#include <Engine/Renderer/Vulkan/VKBackend.hpp>
#include <Engine/Core/Assert.hpp>

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


// NOTE(v.matushkin): Just use c++17 std::size() or c++20 std::ssize() ?
#define ARRAYSIZE(arr) (sizeof(arr) / sizeof(arr[0]))

// NOTE(v.matushkin): The are other validation layers
// NOTE(v.matushkin): Layers can be configured, options are listed in <layer>.json, but do I need to?
const char* vk_ValidationLayers[] = {
    "VK_LAYER_KHRONOS_validation",
    // "VK_LAYER_KHRONOS_synchronization2", // TODO(v.matushkin): I need to use this
};

const char* vk_InstanceExtensions[] = {
    VK_KHR_SURFACE_EXTENSION_NAME,

#ifdef SNV_PLATFORM_WINDOWS
    VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif

#ifdef SNV_GPU_API_DEBUG_ENABLED
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
    // VK_EXT_DEBUG_MARKER_EXTENSION_NAME
#endif
};

const char* vk_DeviceExtensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};


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
{
    CreateInstance();
#ifdef SNV_GPU_API_DEBUG_ENABLED
    CreateDebugUtilsMessenger();
#endif
    CreateSurface();
    CreateDevice();
    CreateSwapchain();
}

VKBackend::~VKBackend()
{
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
{}

void VKBackend::SetDepthFunction(DepthFunction depthFunction)
{}

void VKBackend::SetViewport(i32 x, i32 y, i32 width, i32 height)
{}


void VKBackend::Clear(BufferBit bufferBitMask)
{}


void VKBackend::BeginFrame(const glm::mat4x4& localToWorld, const glm::mat4x4& cameraView, const glm::mat4x4& cameraProjection)
{}

void VKBackend::EndFrame()
{}

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
    return ShaderHandle();
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

    //- Create Instance Debug Messenger
    const auto vkDebugUtilsMessengerInfo = CreateDebugUtilsMessengerInfo();
    vkInstanceInfo.pNext                 = &vkDebugUtilsMessengerInfo;
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
        .format     = VK_FORMAT_B8G8R8A8_UNORM,
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
    VkExtent2D vkImageExtent;
    {
        const auto vkCurrentExtent = vkSurfaceCapabilities.currentExtent;
        if (vkCurrentExtent.width == std::numeric_limits<ui32>::max())
        {
            // TODO(v.matushkin): <SwapchainCreation/ImageExtent>
            SNV_ASSERT(false, "Do glfwGetFramebufferSize");
        }
        else
        {
            vkImageExtent = vkCurrentExtent;
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
        .imageExtent           = vkImageExtent,
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

void VKBackend::CreateGraphicsPipeline()
{

}


#ifdef SNV_GPU_API_DEBUG_ENABLED
VkDebugUtilsMessengerCreateInfoEXT VKBackend::CreateDebugUtilsMessengerInfo()
{
    //NOTE(v.matushkin): Verbose gives a lot of useless shit
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
