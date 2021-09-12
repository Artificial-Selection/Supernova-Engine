#include <Engine/Renderer/Vulkan/VKBackend.hpp>
#include <Engine/Core/Log.hpp>


// NOTE(v.matushkin): Just use c++17 std::size() or c++20 std::ssize() ?
#define ARRAYSIZE(arr) (sizeof(arr) / sizeof(arr[0]))

// NOTE(v.matushkin): The are other validation layers
// NOTE(v.matushkin): Layers can be configured, options are listed in <layer>.json, but do I need to?
const char* vk_ValidationLayers[] = {
    "VK_LAYER_KHRONOS_validation",
    // TODO(v.matushkin): VK_LAYER_KHRONOS_validation already includes this? Need to check
    "VK_LAYER_KHRONOS_synchronization2",
};

const char* vk_InstanceExtensions[] = {
    VK_KHR_SURFACE_EXTENSION_NAME,

#ifdef SNV_PLATFORM_WINDOWS
    "VK_KHR_win32_surface", // VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#endif

#ifdef SNV_GPU_API_DEBUG_ENABLED
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
    // VK_EXT_DEBUG_MARKER_EXTENSION_NAME
#endif
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
}

VKBackend::~VKBackend()
{
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


// TODO(v.matushkin): Check for validation layers support?
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

// TODO(v.matushkin): Check that required extensions are supported?
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

} // namespace snv
