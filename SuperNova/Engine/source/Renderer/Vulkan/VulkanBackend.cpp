#include <Engine/Renderer/Vulkan/VulkanBackend.hpp>
#include <Engine/Core/Assert.hpp>
#include <Engine/Renderer/Vulkan/VulkanShaderCompiler.hpp>

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
//
// - <CommandBuffers>
//   Not sure that I'm using/reusing them the right way. Learn about:
//     - VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
//     - VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
//     - other relevant flags?
//
// - <UniformBuffers>
//   - Should I use 'alignas(256)' ?
//   - Query minUniformBufferOffsetAlignment with VkPhysicalDeviceProperties to align individual uniform buffer members?
//     GLM_FORCE_DEFAULT_ALIGNED_GENTYPES ?
//   - Place all UniformBuffers in one VkDeviceMemory?
//   - Always mapped?
//   [LINKS]
//     - https://developer.nvidia.com/vulkan-shader-resource-binding
//     - Not sure how relevant this is http://kylehalladay.com/blog/tutorial/vulkan/2017/08/13/Vulkan-Uniform-Buffers.html
//     - http://kylehalladay.com/blog/tutorial/2017/11/27/Vulkan-Material-System.html
//
// - <DescriptorSet>
//   What are the values for
//     - VkDescriptorPoolSize::descriptorCount
//     - VkDescriptorPoolCreateInfo::maxSets
//     - VkDescriptorSetAllocateInfo::descriptorSetCount
//   should be?
//
// - <BufferAndImageCreation>
//   - There is a lot of code repetition in creating VkBuffer/VkImage and allocating memory for them
//   - There is even more code repetition in CreateMesh/CreateTexture
//   - There is a code repetition in FindImageMemoryTypeIndex and FindBufferMemoryTypeIndex
//
// - <SubpassDependency>
//   - Not sure that stage and access masks specified correctly in VkSubpassDependency2.
//   - Do I need to use one VkSubpassDependency2 for color and one for depth?
//   [LINKS]
//     - https://stackoverflow.com/questions/62371266/why-is-a-single-depth-buffer-sufficient-for-this-vulkan-swapchain-render-loop


// NOTE(v.matushkin): Just use c++17 std::size() or c++20 std::ssize() ?
#define ARRAYSIZE(arr) (sizeof(arr) / sizeof(arr[0]))


const VkFormat vk_TextureFormat[] = {
    VK_FORMAT_R8_UINT,
    VK_FORMAT_R16_UINT,
    VK_FORMAT_R16_SFLOAT,
    VK_FORMAT_R32_SFLOAT,
    VK_FORMAT_R8G8_UINT,
    VK_FORMAT_R16G16_UINT,
    // VK_FORMAT_R8G8B8A8_UNORM,     // RGB8
    // VK_FORMAT_R16G16B16A16_UINT,  // RGB16
    VK_FORMAT_R8G8B8A8_UNORM,
    VK_FORMAT_R16G16B16A16_SFLOAT,
    VK_FORMAT_D16_UNORM,
    // VK_FORMAT_D24_UNORM_S8_UINT,  // DEPTH24
    VK_FORMAT_D24_UNORM_S8_UINT, // DEPTH32
    VK_FORMAT_D32_SFLOAT
};

namespace ShaderSet
{
    const ui32 Camera   = 0;
    const ui32 Material = 1;
}
namespace ShaderBinding
{
    //- Set 0
    const ui32 ubPerFrame    = 0;
    const ui32 ubPerDraw     = 1;
    const ui32 sSampler      = 2;
    //- Set 1
    const ui32 tBaseColorMap = 0;
}

const VkFormat k_SwapchainFormat    = VK_FORMAT_B8G8R8A8_UNORM;
const VkFormat k_DepthStencilFormat = VK_FORMAT_D32_SFLOAT;
const f32      k_DepthClearValue    = 1.0f;

// NOTE(v.mnatushkin): 0.04s, this will break for FPS <30, I'm sure I'm doing this acquire/present thing wrong
const ui64 k_Timeout = 40'000'000;

// TODO(v.matushkin): <RenderGraph>
static bool g_IsPipelineInitialized = false;


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
        return false;
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

    return false;
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

VulkanBackend::VulkanBackend()
    : m_currentFrame(0)
{
    m_clearValues[0].color        = {.float32 = {1.0f, 0.0f, 0.0f, 0.0f}};
    m_clearValues[1].depthStencil = {.depth = k_DepthClearValue, .stencil = 0};

    VulkanShaderCompiler::Init();

    CreateInstance();
#ifdef SNV_GPU_API_DEBUG_ENABLED
    CreateDebugUtilsMessenger();
#endif
    CreateSurface();
    CreateDevice();
    CreateSwapchain();
    CreateDepthBuffer();
    CreateRenderPass();
    CreateFramebuffers();

    CreateCommandPool();
    CreateCommandBuffers();
    FindMemoryTypeIndices();
    CreateSyncronizationObjects();

    CreateUniformBuffers();
    CreateTextureSampler();
    CreateDescriptorPool();
    CreateDescriptorSetLayouts();
    CreateDescriptorSets();
}

VulkanBackend::~VulkanBackend()
{
    VulkanShaderCompiler::Shutdown();

    vkQueueWaitIdle(m_graphicsQueue);

    //- Resources
    //-- Uniform Buffers
    for (ui32 i = 0; i < k_BackBufferFrames; ++i)
    {
        vkDestroyBuffer(m_device, m_ubPerDraw[i], nullptr);
        vkDestroyBuffer(m_device, m_ubPerFrame[i], nullptr);

        vkFreeMemory(m_device, m_ubPerDrawMemory[i], nullptr);
        vkFreeMemory(m_device, m_ubPerFrameMemory[i], nullptr);
    }
    //-- Meshes
    for (auto& handleAndBuffer : m_buffers)
    {
        auto& buffer = handleAndBuffer.second;

        vkDestroyBuffer(m_device, buffer.Index, nullptr);
        vkDestroyBuffer(m_device, buffer.Position, nullptr);
        vkDestroyBuffer(m_device, buffer.Normal, nullptr);
        vkDestroyBuffer(m_device, buffer.TexCoord0, nullptr);

        vkFreeMemory(m_device, buffer.IndexMemory, nullptr);
        vkFreeMemory(m_device, buffer.PositionMemory, nullptr);
        vkFreeMemory(m_device, buffer.NormalMemory, nullptr);
        vkFreeMemory(m_device, buffer.TexCoord0Memory, nullptr);
    }
    //-- Textures
    vkDestroySampler(m_device, m_sampler, nullptr);

    for (auto& handleAndTexture : m_textures)
    {
        auto& texture = handleAndTexture.second;

        vkDestroyImageView(m_device, texture.View, nullptr);
        vkDestroyImage(m_device, texture.Image, nullptr);
        vkFreeMemory(m_device, texture.Memory, nullptr);
    }
    //-- Shaders
    // NOTE(v.matushkin): Delete them afetr pipeline creation?
    for (auto& handleAndShader : m_shaders)
    {
        auto& shader = handleAndShader.second;

        vkDestroyShaderModule(m_device, shader.Vertex, nullptr);
        vkDestroyShaderModule(m_device, shader.Fragment, nullptr);
    }

    //- Descriptors
    vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayoutCamera, nullptr);
    vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayourMaterial, nullptr);

    //- Syncronization Objects
    for (ui32 i = 0; i < k_BackBufferFrames; ++i)
    {
        vkDestroySemaphore(m_device, m_semaphoreImageAvailable[i], nullptr);
        vkDestroySemaphore(m_device, m_semaphoreRenderFinished[i], nullptr);

        vkDestroyFence(m_device, m_fences[i], nullptr);
    }

    vkDestroyCommandPool(m_device, m_commandPool, nullptr);

    //- Graphics Pipeline
    vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
    vkDestroyRenderPass(m_device, m_renderPass, nullptr);

    //- Swapchain
    //-- Depth
    vkDestroyImageView(m_device, m_depthImageView, nullptr);
    vkDestroyImage(m_device, m_depthImage, nullptr);
    vkFreeMemory(m_device, m_depthImageMemory, nullptr);
    //-- Color
    for (ui32 i = 0; i < k_BackBufferFrames; ++i)
    {
        vkDestroyFramebuffer(m_device, m_framebuffers[i], nullptr);
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


void VulkanBackend::EnableBlend()
{}

void VulkanBackend::EnableDepthTest()
{}

void VulkanBackend::SetBlendFunction(BlendFactor source, BlendFactor destination)
{}

void VulkanBackend::SetClearColor(f32 r, f32 g, f32 b, f32 a)
{
    m_clearValues[0].color = {.float32 = {r, g, b, a}};
}

void VulkanBackend::SetDepthFunction(DepthFunction depthFunction)
{}

void VulkanBackend::SetViewport(i32 x, i32 y, i32 width, i32 height)
{}


void VulkanBackend::Clear(BufferBit bufferBitMask)
{}


void VulkanBackend::BeginFrame(
    const glm::mat4x4& localToWorld,
    const glm::mat4x4& cameraView,
    const glm::mat4x4& cameraProjection
)
{
    // TODO(v.matushkin): <RenderGraph>
    if (g_IsPipelineInitialized == false)
    {
        g_IsPipelineInitialized = true;
        CreatePipeline();
    }

    auto semaphoreImageAvailable = m_semaphoreImageAvailable[m_currentFrame];
    vkAcquireNextImageKHR(m_device, m_swapchain, k_Timeout, semaphoreImageAvailable, nullptr, &m_currentBackBufferIndex);

    auto fence = m_fences[m_currentBackBufferIndex];
    vkWaitForFences(m_device, 1, &fence, true, k_Timeout);
    vkResetFences(m_device, 1, &fence);

    // NOTE(v.matushkin): VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT ?
    VkCommandBufferBeginInfo vkCommandBufferBegin = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext            = nullptr,
        .flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
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
        .framebuffer     = m_framebuffers[m_currentBackBufferIndex],
        .renderArea      = vkRenderArea,
        .clearValueCount = ARRAYSIZE(m_clearValues),
        .pClearValues    = m_clearValues, // NOTE(v.matushkin): The order should should be identical to the order of attachments
    };

    // NOTE(v.matushkin): Just make a m_currentCommandBuffer member?
    auto commandBuffer = m_commandBuffers[m_currentBackBufferIndex];
    vkResetCommandBuffer(commandBuffer, 0);
    vkBeginCommandBuffer(commandBuffer, &vkCommandBufferBegin);
    // NOTE(v.matushkin): vkCmdBeginRenderPass2 ?
    vkCmdBeginRenderPass(commandBuffer, &vkRenderPassBegin, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

    //- Update Uniform Buffers
    {
        void* data;
        //-- PerFrame
        PerFrame ubPerFrame = {
            ._CameraView       = cameraView,
            ._CameraProjection = cameraProjection,
        };
        auto ubPerFrameMemory = m_ubPerFrameMemory[m_currentBackBufferIndex];

        vkMapMemory(m_device, ubPerFrameMemory, 0, sizeof(PerFrame), 0, &data);
        std::memcpy(data, &ubPerFrame, sizeof(PerFrame));
        vkUnmapMemory(m_device, ubPerFrameMemory);

        //-- PerDraw
        PerDraw ubPerDraw = {
            ._ObjectToWorld = localToWorld
        };
        auto ubPerDrawMemory = m_ubPerDrawMemory[m_currentBackBufferIndex];

        vkMapMemory(m_device, ubPerDrawMemory, 0, sizeof(PerDraw), 0, &data);
        std::memcpy(data, &ubPerDraw, sizeof(PerDraw));
        vkUnmapMemory(m_device, ubPerDrawMemory);
    }

    vkCmdBindDescriptorSets(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_pipelineLayout,
        ShaderSet::Camera,
        1,
        &m_descriptorSets[m_currentBackBufferIndex],
        0,
        nullptr
    );
}

void VulkanBackend::EndFrame()
{
    auto commandBuffer = m_commandBuffers[m_currentBackBufferIndex];
    vkCmdEndRenderPass(commandBuffer);
    vkEndCommandBuffer(commandBuffer);

    auto semaphoreImageAvailable = m_semaphoreImageAvailable[m_currentFrame];
    auto semaphoreRenderFinished = m_semaphoreRenderFinished[m_currentFrame];

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
    vkQueueSubmit(m_graphicsQueue, 1, &vkSubmitInfo, m_fences[m_currentBackBufferIndex]);

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


void VulkanBackend::DrawBuffer(TextureHandle textureHandle, BufferHandle bufferHandle, i32 indexCount, i32 vertexCount)
{
    auto commandBuffer = m_commandBuffers[m_currentBackBufferIndex];

    //- Set Index/Vertex buffers
    const auto& buffer = m_buffers[bufferHandle];
    VkBuffer     vkVertexBuffers[] = {buffer.Position, buffer.Normal, buffer.TexCoord0};
    VkDeviceSize vkOffsets[]       = {0, 0, 0};
    // TODO(v.matushkin): Vertex type shouldn't be hardcoded
    vkCmdBindIndexBuffer(commandBuffer, buffer.Index, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindVertexBuffers(commandBuffer, 0, 3, vkVertexBuffers, vkOffsets);
    
    //- Set Material Texture
    const auto& texture = m_textures[textureHandle];
    vkCmdBindDescriptorSets(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_pipelineLayout,
        ShaderSet::Material,
        1,
        &m_descriptorSetMaterials[texture.DescriptorSetIndex],
        0,
        nullptr
    );

    vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
}


BufferHandle VulkanBackend::CreateBuffer(
    std::span<const std::byte>              indexData,
    std::span<const std::byte>              vertexData,
    const std::vector<VertexAttributeDesc>& vertexLayout
)
{
    // NOTE(v.matushkin): It almost like it got slower after moving VkBuffer
    // from VISIBLE|COHERENT to DEVICE LOCAL, it can't be, right?

    VulkanBuffer vulkanBuffer;

    VkBuffer* vkBuffers[] = {
        &vulkanBuffer.Position,
        &vulkanBuffer.Normal,
        &vulkanBuffer.TexCoord0,
    };
    VkDeviceMemory* vkBuffersMemory[] = {
        &vulkanBuffer.PositionMemory,
        &vulkanBuffer.NormalMemory,
        &vulkanBuffer.TexCoord0Memory,
    };

    VkBuffer       vkStagingBuffers[4];
    VkDeviceMemory vkStagingBuffersMemory[4];

    //- Create CPU->GPU buffer transfer VkCommnadBuffer
    VkCommandBuffer vkCommandBuffer;
    {
        VkCommandBufferAllocateInfo vkCommandBufferInfo = {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool        = m_commandPool,
            .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };
        vkAllocateCommandBuffers(m_device, &vkCommandBufferInfo, &vkCommandBuffer);

        VkCommandBufferBeginInfo vkCommandBufferBeginInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        };
        vkBeginCommandBuffer(vkCommandBuffer, &vkCommandBufferBeginInfo);
    }

    VkBufferCopy vkBufferCopyRegion = {
        .srcOffset = 0,
        .dstOffset = 0,
    };

    //- Create Vertex buffers
    for (ui32 i = 0; i < vertexLayout.size(); ++i)
    {
        // TODO(v.matushkin): Adjust VertexAttributeDescriptor to remove this hacks
        const auto& vertexAttribute = vertexLayout[i];
        const auto  currOffset      = vertexAttribute.Offset;
        const auto  nextOffset      = (i + 1) < vertexLayout.size() ? vertexLayout[i + 1].Offset : vertexData.size_bytes();
        const auto  attributeSize   = (nextOffset - currOffset);

        //-- Create staging VkBuffer
        auto& vkStagingBuffer       = vkStagingBuffers[i];
        auto& vkStagingBufferMemory = vkStagingBuffersMemory[i];
        {
            VkBufferCreateInfo vkBufferInfo = {
                .sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .pNext                 = nullptr,
                .flags                 = 0,
                .size                  = attributeSize,
                .usage                 = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount = 0, // NOTE(v.matushkin): This is for VK_SHARING_MODE_CONCURRENT ?
                .pQueueFamilyIndices   = nullptr,
            };
            vkCreateBuffer(m_device, &vkBufferInfo, nullptr, &vkStagingBuffer);

            // NOTE(v.matushkin): VkMemoryRequirements2, VkMemoryDedicatedRequirements ?
            VkMemoryRequirements vkMemoryRequirements;
            vkGetBufferMemoryRequirements(m_device, vkStagingBuffer, &vkMemoryRequirements);

            //--- Allocate VkDeviceMemory
            VkMemoryAllocateInfo vkAllocateInfo = {
                .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .pNext           = nullptr,
                .allocationSize  = vkMemoryRequirements.size,
                .memoryTypeIndex = m_bufferMemoryTypeIndex.CPUtoGPU,
            };
            vkAllocateMemory(m_device, &vkAllocateInfo, nullptr, &vkStagingBufferMemory);

            //--- Bind VkDeviceMemory to VkBuffer
            // NOTE(v.matushkin): Use vkBindBufferMemory2 to bind multiple buffers at once?
            vkBindBufferMemory(m_device, vkStagingBuffer, vkStagingBufferMemory, 0);

            //--- Copy vertex data to VkDeviceMemory
            void* data;
            vkMapMemory(m_device, vkStagingBufferMemory, 0, attributeSize, 0, &data);
            std::memcpy(data, vertexData.data() + currOffset, attributeSize);
            vkUnmapMemory(m_device, vkStagingBufferMemory);
        }
        //-- Create GPU VkBuffer
        auto vkBuffer       = vkBuffers[i];
        auto vkBufferMemory = vkBuffersMemory[i];
        {
            VkBufferCreateInfo vkStagingBufferInfo = {
                .sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .pNext                 = nullptr,
                .flags                 = 0,
                .size                  = attributeSize,
                .usage                 = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount = 0, // NOTE(v.matushkin): This is for VK_SHARING_MODE_CONCURRENT ?
                .pQueueFamilyIndices   = nullptr,
            };
            vkCreateBuffer(m_device, &vkStagingBufferInfo, nullptr, vkBuffer);

            // NOTE(v.matushkin): VkMemoryRequirements2, VkMemoryDedicatedRequirements ?
            VkMemoryRequirements vkMemoryRequirements;
            vkGetBufferMemoryRequirements(m_device, *vkBuffer, &vkMemoryRequirements);

            //--- Allocate VkDeviceMemory
            VkMemoryAllocateInfo vkAllocateInfo = {
                .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .pNext           = nullptr,
                .allocationSize  = vkMemoryRequirements.size,
                .memoryTypeIndex = m_bufferMemoryTypeIndex.GPUVertex,
            };
            vkAllocateMemory(m_device, &vkAllocateInfo, nullptr, vkBufferMemory);

            //--- Bind VkDeviceMemory to VkBuffer
            // NOTE(v.matushkin): Use vkBindBufferMemory2 to bind multiple buffers at once?
            vkBindBufferMemory(m_device, *vkBuffer, *vkBufferMemory, 0);
        }

        vkBufferCopyRegion.size = attributeSize;
        // NOTE(v.matushkin): vkCmdCopyBuffer2KHR ?
        vkCmdCopyBuffer(vkCommandBuffer, vkStagingBuffer, *vkBuffer, 1, &vkBufferCopyRegion);
    }

    //- Create Index buffer
    //-- Create staging VkBuffer
    auto& vkIndexStagingBuffer       = vkStagingBuffers[3];
    auto& vkIndexStagingBufferMemory = vkStagingBuffersMemory[3];
    {
        VkBufferCreateInfo vkBufferInfo = {
            .sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext                 = nullptr,
            .flags                 = 0,
            .size                  = indexData.size_bytes(),
            .usage                 = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0, // NOTE(v.matushkin): This is for VK_SHARING_MODE_CONCURRENT ?
            .pQueueFamilyIndices   = nullptr,
        };
        vkCreateBuffer(m_device, &vkBufferInfo, nullptr, &vkIndexStagingBuffer);

        // NOTE(v.matushkin): VkMemoryRequirements2, VkMemoryDedicatedRequirements ?
        VkMemoryRequirements vkMemoryRequirements;
        vkGetBufferMemoryRequirements(m_device, vkIndexStagingBuffer, &vkMemoryRequirements);

        //--- Allocate VkDeviceMemory
        VkMemoryAllocateInfo vkAllocateInfo = {
            .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext           = nullptr,
            .allocationSize  = vkMemoryRequirements.size,
            .memoryTypeIndex = m_bufferMemoryTypeIndex.CPUtoGPU,
        };
        vkAllocateMemory(m_device, &vkAllocateInfo, nullptr, &vkIndexStagingBufferMemory);

        //--- Bind VkDeviceMemory to VkBuffer
        // NOTE(v.matushkin): Use vkBindBufferMemory2 to bind multiple buffers at once?
        vkBindBufferMemory(m_device, vkIndexStagingBuffer, vkIndexStagingBufferMemory, 0);

        //--- Copy vertex data to VkDeviceMemory
        void* data;
        vkMapMemory(m_device, vkIndexStagingBufferMemory, 0, indexData.size_bytes(), 0, &data);
        std::memcpy(data, indexData.data(), indexData.size_bytes());
        vkUnmapMemory(m_device, vkIndexStagingBufferMemory);
    }
    //-- Create GPU VkBuffer
    {
        VkBufferCreateInfo vkStagingBufferInfo = {
            .sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext                 = nullptr,
            .flags                 = 0,
            .size                  = indexData.size_bytes(),
            .usage                 = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0, // NOTE(v.matushkin): This is for VK_SHARING_MODE_CONCURRENT ?
            .pQueueFamilyIndices   = nullptr,
        };
        vkCreateBuffer(m_device, &vkStagingBufferInfo, nullptr, &vulkanBuffer.Index);

        // NOTE(v.matushkin): VkMemoryRequirements2, VkMemoryDedicatedRequirements ?
        VkMemoryRequirements vkMemoryRequirements;
        vkGetBufferMemoryRequirements(m_device, vulkanBuffer.Index, &vkMemoryRequirements);

        //--- Allocate VkDeviceMemory
        VkMemoryAllocateInfo vkAllocateInfo = {
            .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext           = nullptr,
            .allocationSize  = vkMemoryRequirements.size,
            .memoryTypeIndex = m_bufferMemoryTypeIndex.GPUVertex,
        };
        vkAllocateMemory(m_device, &vkAllocateInfo, nullptr, &vulkanBuffer.IndexMemory);

        //--- Bind VkDeviceMemory to VkBuffer
        // NOTE(v.matushkin): Use vkBindBufferMemory2 to bind multiple buffers at once?
        vkBindBufferMemory(m_device, vulkanBuffer.Index, vulkanBuffer.IndexMemory, 0);
    }

    vkBufferCopyRegion.size = indexData.size_bytes();
    vkCmdCopyBuffer(vkCommandBuffer, vkStagingBuffers[3], vulkanBuffer.Index, 1, &vkBufferCopyRegion);

    //- Copy vertex data from staging buffers to GPU
    vkEndCommandBuffer(vkCommandBuffer);
    VkSubmitInfo vkSubmitInfo = {
        .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers    = &vkCommandBuffer,
    };
    vkQueueSubmit(m_graphicsQueue, 1, &vkSubmitInfo, nullptr);
    vkQueueWaitIdle(m_graphicsQueue);

    vkFreeCommandBuffers(m_device, m_commandPool, 1, &vkCommandBuffer);

    //- Destroy staging buffers
    for (ui32 i = 0; i < 4; ++i)
    {
        vkDestroyBuffer(m_device, vkStagingBuffers[i], nullptr);
        vkFreeMemory(m_device, vkStagingBuffersMemory[i], nullptr);
    }

    static ui32 buffer_handle_workaround = 0;
    auto        bufferHandle     = static_cast<BufferHandle>(buffer_handle_workaround++);

    m_buffers[bufferHandle] = vulkanBuffer;

    return bufferHandle;
}

TextureHandle VulkanBackend::CreateTexture(const TextureDesc& textureDesc, const ui8* textureData)
{
    // TODO(v.matushkin): Assuming texture format is RGBA8888. Shouldn't be hardcoded
    //  Seems like there was no need to know the actual texture data size in another backends, which is strange.
    //  But here I need it.
    //  Pass textureData as std::span? Add size member to TextureDesc? Convert texture format to pixel size?
    //  May be if I used VkImage as a staging buffer instead of VkBuffer, texture width/height/format would be enough.
    //  But here - https://developer.nvidia.com/vulkan-memory-management, the say that it is better to use VkBuffer.
    const auto textureSize = textureDesc.Width * textureDesc.Height * sizeof(ui8) * 4;

    const auto vkTextureFormat = vk_TextureFormat[static_cast<ui8>(textureDesc.Format)];

    VulkanTexture vulkanTexture;

    //- Create staging texture VkBuffer
    VkBuffer vkTextureStagingBuffer;
    VkDeviceMemory vkTextureStagingBufferMemory;
    {
        VkBufferCreateInfo vkBufferInfo = {
            .sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext                 = nullptr,
            .flags                 = 0,
            .size                  = textureSize,
            .usage                 = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0, // NOTE(v.matushkin): This is for VK_SHARING_MODE_CONCURRENT ?
            .pQueueFamilyIndices   = nullptr,
        };
        vkCreateBuffer(m_device, &vkBufferInfo, nullptr, &vkTextureStagingBuffer);

        // NOTE(v.matushkin): VkMemoryRequirements2, VkMemoryDedicatedRequirements ?
        VkMemoryRequirements vkMemoryRequirements;
        vkGetBufferMemoryRequirements(m_device, vkTextureStagingBuffer, &vkMemoryRequirements);

        //-- Allocate VkDeviceMemory
        VkMemoryAllocateInfo vkAllocateInfo = {
            .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext           = nullptr,
            .allocationSize  = vkMemoryRequirements.size,
            .memoryTypeIndex = m_bufferMemoryTypeIndex.CPUtoGPU,
        };
        vkAllocateMemory(m_device, &vkAllocateInfo, nullptr, &vkTextureStagingBufferMemory);

        //-- Bind VkDeviceMemory to VkBuffer
        // NOTE(v.matushkin): Use vkBindBufferMemory2 to bind multiple buffers at once?
        vkBindBufferMemory(m_device, vkTextureStagingBuffer, vkTextureStagingBufferMemory, 0);

        //-- Copy texture data to VkDeviceMemory
        void* data;
        vkMapMemory(m_device, vkTextureStagingBufferMemory, 0, textureSize, 0, &data);
        std::memcpy(data, textureData, textureSize);
        vkUnmapMemory(m_device, vkTextureStagingBufferMemory);
    }
    //- Create VkImage
    {
        VkImageCreateInfo vkImageInfo = {
            .sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext                 = nullptr,
            .flags                 = 0,
            .imageType             = VK_IMAGE_TYPE_2D,
            .format                = vkTextureFormat,
            .extent                = {textureDesc.Width, textureDesc.Height, 1},
            .mipLevels             = 1,
            .arrayLayers           = 1,
            .samples               = VK_SAMPLE_COUNT_1_BIT,
            .tiling                = VK_IMAGE_TILING_OPTIMAL,
            .usage                 = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,       // TODO(v.matushkin): The fuck is this?
            .pQueueFamilyIndices   = nullptr, // TODO(v.matushkin): The fuck is this?
            .initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED,
        };
        vkCreateImage(m_device, &vkImageInfo, nullptr, &vulkanTexture.Image);

        // NOTE(v.matushkin): VkMemoryRequirements2, VkMemoryDedicatedRequirements ?
        VkMemoryRequirements vkMemoryRequirements;
        vkGetImageMemoryRequirements(m_device, vulkanTexture.Image, &vkMemoryRequirements);

        //-- Allocate VkDeviceMemory
        VkMemoryAllocateInfo vkAllocateInfo = {
            .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext           = nullptr,
            .allocationSize  = vkMemoryRequirements.size,
            .memoryTypeIndex = m_bufferMemoryTypeIndex.GPUTexture,
        };
        vkAllocateMemory(m_device, &vkAllocateInfo, nullptr, &vulkanTexture.Memory);

        //-- Bind VkDeviceMemory to VkImage
        // NOTE(v.matushkin): vkBindImageMemory2 ?
        vkBindImageMemory(m_device, vulkanTexture.Image, vulkanTexture.Memory, 0);
    }
    //- Transfer texture from CPU to GPU
    {
        VkCommandBuffer vkCommandBuffer;

        //- Allocate
        VkCommandBufferAllocateInfo vkCommandBufferInfo = {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool        = m_commandPool,
            .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };
        vkAllocateCommandBuffers(m_device, &vkCommandBufferInfo, &vkCommandBuffer);
        //- Begin
        VkCommandBufferBeginInfo vkCommandBufferBeginInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        };
        vkBeginCommandBuffer(vkCommandBuffer, &vkCommandBufferBeginInfo);
        //- VkImage Memory Barrier UNDEFINED->DST_OPTIMAL
        VkImageSubresourceRange vkImageSubresourceRange = {
            .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel   = 0,
            .levelCount     = 1,
            .baseArrayLayer = 0,
            .layerCount     = 1,
        };
        // TODO(v.matushkin): VkImageMemoryBarrier2KHR
        VkImageMemoryBarrier vkImageMemoryBarrier = {
            .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext               = nullptr,
            .srcAccessMask       = 0,
            .dstAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT,
            .oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image               = vulkanTexture.Image,
            .subresourceRange    = vkImageSubresourceRange,
        };
        // TODO(v.matushkin): vkCmdPipelineBarrier2KHR?
        vkCmdPipelineBarrier(
            vkCommandBuffer,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0, // NOTE(v.matushkin): ???
            0, nullptr,
            0, nullptr,
            1, &vkImageMemoryBarrier
        );
        //- Copy VkBuffer to VkImage
        VkImageSubresourceLayers vkImageSubresourceLayers = {
            .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel       = 0,
            .baseArrayLayer = 0,
            .layerCount     = 1,
        };
        // NOTE(v.matushkin): VkBufferImageCopy2KHR?
        VkBufferImageCopy vkBufferImageCopy = {
            .bufferOffset      = 0,
            .bufferRowLength   = 0,
            .bufferImageHeight = 0,
            .imageSubresource  = vkImageSubresourceLayers,
            .imageOffset       = {0, 0, 0},
            .imageExtent       = {textureDesc.Width, textureDesc.Height, 1},
        };
        vkCmdCopyBufferToImage(
            vkCommandBuffer,
            vkTextureStagingBuffer,
            vulkanTexture.Image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &vkBufferImageCopy
        );
        //- VkImage Memory Barrier DST_OPTIMAL->SHADER_READ_ONLY_OPTIMAL
        vkImageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        vkImageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkImageMemoryBarrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        vkImageMemoryBarrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        vkCmdPipelineBarrier(
            vkCommandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0, // NOTE(v.matushkin): ???
            0, nullptr,
            0, nullptr,
            1, &vkImageMemoryBarrier
        );

        //- Submit
        vkEndCommandBuffer(vkCommandBuffer);

        VkSubmitInfo vkSubmitInfo = {
            .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers    = &vkCommandBuffer,
        };
        vkQueueSubmit(m_graphicsQueue, 1, &vkSubmitInfo, nullptr);
        vkQueueWaitIdle(m_graphicsQueue);

        vkFreeCommandBuffers(m_device, m_commandPool, 1, &vkCommandBuffer);
    }
    vkDestroyBuffer(m_device, vkTextureStagingBuffer, nullptr);
    vkFreeMemory(m_device, vkTextureStagingBufferMemory, nullptr);

    //- Create VkImageView
    {
        VkComponentMapping vkComponentMappingMapping = {
            .r = VK_COMPONENT_SWIZZLE_R,
            .g = VK_COMPONENT_SWIZZLE_G,
            .b = VK_COMPONENT_SWIZZLE_B,
            .a = VK_COMPONENT_SWIZZLE_A,
        };
        VkImageSubresourceRange vkImageSubresourceRange = {
            .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel   = 0,
            .levelCount     = 1,
            .baseArrayLayer = 0,
            .layerCount     = 1,
        };
        VkImageViewCreateInfo vkImageViewInfo = {
            .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext            = nullptr,
            .flags            = 0,
            .image            = vulkanTexture.Image,
            .viewType         = VK_IMAGE_VIEW_TYPE_2D,
            .format           = vkTextureFormat,
            .components       = vkComponentMappingMapping,
            .subresourceRange = vkImageSubresourceRange,
        };
        vkCreateImageView(m_device, &vkImageViewInfo, nullptr, &vulkanTexture.View);
    }

    static ui32 texture_handle_workaround = 0;

    //- Create Texture descriptor
    vulkanTexture.DescriptorSetIndex = texture_handle_workaround;
    {
        //-- Allocate DescriptorSet
        VkDescriptorSetAllocateInfo vkDescriptorSetInfo = {
            .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .pNext              = nullptr,
            .descriptorPool     = m_descriptorPool,
            .descriptorSetCount = 1,
            .pSetLayouts        = &m_descriptorSetLayourMaterial,
        };
        vkAllocateDescriptorSets(m_device, &vkDescriptorSetInfo, &m_descriptorSetMaterials[texture_handle_workaround]);
        //-- Write to DescriptorSet
        VkDescriptorImageInfo vkDescriptorImageInfo = {
            .sampler     = nullptr, // NOTE(v.matushkin): What to set when using immutable sampler?
            .imageView   = vulkanTexture.View,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
        VkWriteDescriptorSet vkWriteDescriptorSet = {
            .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext            = nullptr,
            .dstSet           = m_descriptorSetMaterials[texture_handle_workaround],
            .dstBinding       = ShaderBinding::tBaseColorMap,
            .dstArrayElement  = 0,
            .descriptorCount  = 1,
            .descriptorType   = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .pImageInfo       = &vkDescriptorImageInfo,
            .pBufferInfo      = nullptr,
            .pTexelBufferView = nullptr,
        };
        vkUpdateDescriptorSets(m_device, 1, &vkWriteDescriptorSet, 0, nullptr);
    }

    auto textureHandle = static_cast<TextureHandle>(texture_handle_workaround++);

    m_textures[textureHandle] = vulkanTexture;

    return textureHandle;
}

ShaderHandle VulkanBackend::CreateShader(std::span<const char> vertexSource, std::span<const char> fragmentSource)
{
    const auto vertexBytecode   = VulkanShaderCompiler::CompileShader(VulkanShaderCompiler::ShaderType::Vertex, vertexSource);
    const auto fragmentBytecode = VulkanShaderCompiler::CompileShader(VulkanShaderCompiler::ShaderType::Fragment, fragmentSource);

    VulkanShader vulkanShader;

    VkShaderModuleCreateInfo vkVertexShaderInfo = {
       .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
       .pNext    = nullptr,
       .flags    = 0,
       .codeSize = vertexBytecode.size() * sizeof(ui32),
       .pCode    = vertexBytecode.data(),
    };
    vkCreateShaderModule(m_device, &vkVertexShaderInfo, nullptr, &vulkanShader.Vertex);

    VkShaderModuleCreateInfo vkFragmentShaderInfo = {
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext    = nullptr,
        .flags    = 0,
        .codeSize = fragmentBytecode.size() * sizeof(ui32),
        .pCode    = fragmentBytecode.data(),
    };
    vkCreateShaderModule(m_device, &vkFragmentShaderInfo, nullptr, &vulkanShader.Fragment);

    static ui32 shader_handle_workaround = 0;
    auto        shaderHandle             = static_cast<ShaderHandle>(shader_handle_workaround++);

    m_shaders[shaderHandle] = vulkanShader;

    return shaderHandle;
}


void VulkanBackend::CreateInstance()
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
    VkValidationFeatureEnableEXT vkValidationFeaturesEnable[] = {
        VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
        //VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT,
        //VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
    };
    
    VkValidationFeaturesEXT vkValidationFeatures = {
        .sType                          = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
        .pNext                          = nullptr,
        .enabledValidationFeatureCount  = ARRAYSIZE(vkValidationFeaturesEnable),
        .pEnabledValidationFeatures     = vkValidationFeaturesEnable,
        .disabledValidationFeatureCount = 0,
        .pDisabledValidationFeatures    = nullptr,
    };

    //- Create Instance Debug Messenger
    auto vkDebugUtilsMessengerInfo  = CreateDebugUtilsMessengerInfo();
    // vkDebugUtilsMessengerInfo.pNext = &vkValidationFeatures;

    vkInstanceInfo.pNext = &vkDebugUtilsMessengerInfo;
#endif

    vkCreateInstance(&vkInstanceInfo, nullptr, &m_instance);
}

void VulkanBackend::CreateSurface()
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

void VulkanBackend::CreateDevice()
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
    // NOTE(v.matushkin): Check for SeparateDepthStencilLayoutsFeatures support?
    VkPhysicalDeviceSeparateDepthStencilLayoutsFeatures vkSeparateDepthStencilLayout = {
        .sType                       = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SEPARATE_DEPTH_STENCIL_LAYOUTS_FEATURES,
        .pNext                       = nullptr,
        .separateDepthStencilLayouts = true,
    };
    VkDeviceCreateInfo vkDeviceInfo = {
        .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext                   = &vkSeparateDepthStencilLayout,
        .flags                   = 0,
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

void VulkanBackend::CreateSwapchain()
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
        .queueFamilyIndexCount = 0,       // TODO(v.matushkin): The fuck is this?
        .pQueueFamilyIndices   = nullptr, // TODO(v.matushkin): The fuck is this?
        .preTransform          = vkPreTransform,
        .compositeAlpha        = vkCompositeAlpha, // NOTE(v.matushkin): Blending with other windows
        .presentMode           = vkPresentMode,
        .clipped               = true,    // Don't care about pixels that are obscured (e.g. other window is in front of them)
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

void VulkanBackend::CreateDepthBuffer()
{
    // NOTE(v.matushkin): Check for depth formats support with vkGetPhysicalDeviceFormatProperties ?

    //- Create VkImage
    {
        VkImageCreateInfo vkImageInfo = {
            .sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext                 = nullptr,
            .flags                 = 0,
            .imageType             = VK_IMAGE_TYPE_2D,
            .format                = k_DepthStencilFormat,
            .extent                = {m_swapchainExtent.width, m_swapchainExtent.height, 1},
            .mipLevels             = 1,
            .arrayLayers           = 1,
            .samples               = VK_SAMPLE_COUNT_1_BIT,
            .tiling                = VK_IMAGE_TILING_OPTIMAL,
            .usage                 = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices   = nullptr,
            .initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED,
        };
        vkCreateImage(m_device, &vkImageInfo, nullptr, &m_depthImage);

        // NOTE(v.matushkin): VkMemoryRequirements2, VkMemoryDedicatedRequirements ?
        VkMemoryRequirements vkMemoryRequirements;
        vkGetImageMemoryRequirements(m_device, m_depthImage, &vkMemoryRequirements);

        //- Allocate VkDeviceMemory
        VkMemoryAllocateInfo vkAllocateInfo = {
            .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext           = nullptr,
            .allocationSize  = vkMemoryRequirements.size,
            .memoryTypeIndex = m_bufferMemoryTypeIndex.GPUTexture,
        };
        vkAllocateMemory(m_device, &vkAllocateInfo, nullptr, &m_depthImageMemory);

        //- Bind VkDeviceMemory to VkImage
        // NOTE(v.matushkin): vkBindImageMemory2 ?
        vkBindImageMemory(m_device, m_depthImage, m_depthImageMemory, 0);
    }
    //- Create VkImageView
    {
        VkComponentMapping vkComponentMappingMapping = {
            .r = VK_COMPONENT_SWIZZLE_R,
            .g = VK_COMPONENT_SWIZZLE_G,
            .b = VK_COMPONENT_SWIZZLE_B,
            .a = VK_COMPONENT_SWIZZLE_A,
        };
        VkImageSubresourceRange vkImageSubresourceRange = {
            .aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT,
            .baseMipLevel   = 0,
            .levelCount     = 1,
            .baseArrayLayer = 0,
            .layerCount     = 1,
        };
        VkImageViewCreateInfo vkImageViewInfo = {
            .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext            = nullptr,
            .flags            = 0,
            .image            = m_depthImage,
            .viewType         = VK_IMAGE_VIEW_TYPE_2D,
            .format           = k_DepthStencilFormat,
            .components       = vkComponentMappingMapping,
            .subresourceRange = vkImageSubresourceRange,
        };
        vkCreateImageView(m_device, &vkImageViewInfo, nullptr, &m_depthImageView);
    }
}

void VulkanBackend::CreateRenderPass()
{
    VkAttachmentDescription2 vkAttachmentDescriptions[] = {
        // Color
        {
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
        },
        // Depth
        {
            .sType          = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2,
            .pNext          = nullptr,
            .flags          = 0,
            .format         = k_DepthStencilFormat,
            .samples        = VK_SAMPLE_COUNT_1_BIT,
            .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout    = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        }
    };

    VkAttachmentReference2 vkColorAttachmentRef = {
        .sType      = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
        .pNext      = nullptr,
        .attachment = 0,
        .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        // .aspectMask = , // NOTE(v.matushkin): The fuck is this?
    };
    // NOTE(v.matushkin): separateDepthStencilLayouts ?
    VkAttachmentReference2 vkDepthAttachmentRef = {
        .sType      = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
        .pNext      = nullptr,
        .attachment = 1,
        .layout     = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
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
        .pDepthStencilAttachment = &vkDepthAttachmentRef,
        // .preserveAttachmentCount = ,
        // .pPreserveAttachments    = ,
    };

    VkSubpassDependency2 vkSubpassDependency = {
        .sType           = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2,
        .pNext           = nullptr,
        .srcSubpass      = VK_SUBPASS_EXTERNAL,
        .dstSubpass      = 0,
        .srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .srcAccessMask   = 0,
        .dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        // .dependencyFlags = ,
        // .viewOffset      = ,
    };

    VkRenderPassCreateInfo2 vkRenderPassInfo = {
        .sType                   = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2,
        .pNext                   = nullptr,
        .flags                   = 0,
        .attachmentCount         = 2,
        .pAttachments            = vkAttachmentDescriptions,
        .subpassCount            = 1,
        .pSubpasses              = &vkSubpassDescription,
        .dependencyCount         = 1,
        .pDependencies           = &vkSubpassDependency,
        // .correlatedViewMaskCount = ,
        // .pCorrelatedViewMasks    = ,
    };
    vkCreateRenderPass2(m_device, &vkRenderPassInfo, nullptr, &m_renderPass);
}

void VulkanBackend::CreateFramebuffers()
{
    VkImageView vkAttachments[2];
    vkAttachments[1] = m_depthImageView;

    VkFramebufferCreateInfo vkFramebufferInfo = {
        .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .pNext           = nullptr,
        .flags           = 0,
        .renderPass      = m_renderPass,
        .attachmentCount = ARRAYSIZE(vkAttachments),
        .width           = m_swapchainExtent.width,
        .height          = m_swapchainExtent.height,
        .layers          = 1,
    };

    for (ui32 i = 0; i < k_BackBufferFrames; ++i)
    {
        vkAttachments[0] = m_backBuffers[i];
        vkFramebufferInfo.pAttachments = vkAttachments;
        vkCreateFramebuffer(m_device, &vkFramebufferInfo, nullptr, &m_framebuffers[i]);
    }
}

void VulkanBackend::CreateDescriptorSetLayouts()
{
    //- Set 0
    {
        VkDescriptorSetLayoutBinding vkDescriptorSetLayoutBindings[] = {
            // PerFrame
            {
                .binding            = ShaderBinding::ubPerFrame,
                .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount    = 1,
                .stageFlags         = VK_SHADER_STAGE_VERTEX_BIT,
                .pImmutableSamplers = nullptr,
            },
            // PerDraw
            {
                .binding            = ShaderBinding::ubPerDraw,
                .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount    = 1,
                .stageFlags         = VK_SHADER_STAGE_VERTEX_BIT,
                .pImmutableSamplers = nullptr,
            },
            // Texture Sampler
            {
                .binding            = ShaderBinding::sSampler,
                .descriptorType     = VK_DESCRIPTOR_TYPE_SAMPLER,
                .descriptorCount    = 1,
                .stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT,
                .pImmutableSamplers = &m_sampler,
            }
        };
        VkDescriptorSetLayoutCreateInfo vkDescriptorSetLayoutInfo = {
            .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext        = nullptr,
            .flags        = 0,
            .bindingCount = ARRAYSIZE(vkDescriptorSetLayoutBindings),
            .pBindings    = vkDescriptorSetLayoutBindings,
        };
        vkCreateDescriptorSetLayout(m_device, &vkDescriptorSetLayoutInfo, nullptr, &m_descriptorSetLayoutCamera);
    }
    //- Set 1
    {
        VkDescriptorSetLayoutBinding vkDescriptorSetLayoutBinding = {
            .binding            = ShaderBinding::tBaseColorMap,
            .descriptorType     = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .descriptorCount    = 1,
            .stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr,
        };
        VkDescriptorSetLayoutCreateInfo vkDescriptorSetLayoutInfo = {
            .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext        = nullptr,
            .flags        = 0,
            .bindingCount = 1,
            .pBindings    = &vkDescriptorSetLayoutBinding,
        };
        vkCreateDescriptorSetLayout(m_device, &vkDescriptorSetLayoutInfo, nullptr, &m_descriptorSetLayourMaterial);
    }

}

void VulkanBackend::CreatePipeline()
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
    // TODO(v.matushkin): Hardcoded
    VkVertexInputBindingDescription vkVertexBindingDescriptions[] = {
        {
            .binding   = 0,
            .stride    = sizeof(f32) * 3,
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        },
        {
            .binding   = 1,
            .stride    = sizeof(f32) * 3,
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        },
        {
            .binding   = 2,
            .stride    = sizeof(f32) * 3,
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        },
    };
    VkVertexInputAttributeDescription vkVertexAttributeDescriptions[] = {
        {
            .location = 0,
            .binding  = 0,
            .format   = VK_FORMAT_R32G32B32_SFLOAT,
            .offset   = 0,
        },
        {
            .location = 1,
            .binding  = 1,
            .format   = VK_FORMAT_R32G32B32_SFLOAT,
            .offset   = 0,
        },
        {
            .location = 2,
            .binding  = 2,
            .format   = VK_FORMAT_R32G32B32_SFLOAT,
            .offset   = 0,
        },
    };
    VkPipelineVertexInputStateCreateInfo vkVertexInputState = {
        .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext                           = nullptr,
        .flags                           = 0, // SPEC: reserved for future use
        .vertexBindingDescriptionCount   = ARRAYSIZE(vkVertexBindingDescriptions),
        .pVertexBindingDescriptions      = vkVertexBindingDescriptions,
        .vertexAttributeDescriptionCount = ARRAYSIZE(vkVertexAttributeDescriptions),
        .pVertexAttributeDescriptions    = vkVertexAttributeDescriptions,
    };

    //- InputAssembly
    VkPipelineInputAssemblyStateCreateInfo vkInputAssemblyState = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext                  = nullptr,
        .flags                  = 0, // SPEC: reserved for future use
        .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = false,
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
        .depthClampEnable        = false,
        .rasterizerDiscardEnable = false,
        .polygonMode             = VK_POLYGON_MODE_FILL,
        .cullMode                = VK_CULL_MODE_BACK_BIT,
        .frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable         = false,
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
        .sampleShadingEnable   = false,
        .minSampleShading      = 1.0f,
        .pSampleMask           = nullptr,
        .alphaToCoverageEnable = false,
        .alphaToOneEnable      = false,
    };

    //- Depth/Stencil
    VkPipelineDepthStencilStateCreateInfo vkDepthStencilState = {
        .sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext                 = nullptr,
        .flags                 = 0, // SPEC: reserved for future use
        .depthTestEnable       = true,
        .depthWriteEnable      = true,
        .depthCompareOp        = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = false,
        .stencilTestEnable     = false,
        // .front                 = ,     // for stencilTestEnable=true
        // .back                  = ,     // for stencilTestEnable=true
        // .minDepthBounds        = 0.0f, // for depthBoundsTestEnable=true
        // .maxDepthBounds        = 1.0f, // for depthBoundsTestEnable=true
    };

    //- ColorBlend
    // NOTE(v.matushkin): VkPipelineColorBlendAttachmentState - per framebuffer, VkPipelineColorBlendStateCreateInfo - global
    VkPipelineColorBlendAttachmentState vkColorBlendAttachment = {
        .blendEnable         = false,
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
        .logicOpEnable   = false,
        .logicOp         = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments    = &vkColorBlendAttachment,
        .blendConstants  = {0.0f, 0.0f, 0.0f, 0.0f},
    };

    //- DynamicState
    // NOTE(v.matushkin): Not needed right now
    // VkPipelineDynamicStateCreateInfo vkDynamicStateInfo;

    //- Create VkPipelineLayout
    VkDescriptorSetLayout      vkDescriptorSetLayouts[] = {m_descriptorSetLayoutCamera, m_descriptorSetLayourMaterial};
    VkPipelineLayoutCreateInfo vkPipelineLayoutInfo = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext                  = nullptr,
        .flags                  = 0, // SPEC: reserved for future use
        .setLayoutCount         = ARRAYSIZE(vkDescriptorSetLayouts),
        .pSetLayouts            = vkDescriptorSetLayouts,
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
        .pDepthStencilState  = &vkDepthStencilState,
        .pColorBlendState    = &vkColorBlendState,
        .pDynamicState       = nullptr,
        .layout              = m_pipelineLayout,
        .renderPass          = m_renderPass,
        .subpass             = 0, // NOTE(v.matushkin): What's this?
        .basePipelineHandle  = nullptr, // NOTE(v.matushkin): create a new graphics pipeline by deriving from an existing pipeline
        .basePipelineIndex   = -1,
    };
    // NOTE(v.matushkin): VkPipelineCache ?
    vkCreateGraphicsPipelines(m_device, nullptr, 1, &vkGraphicsPipelineInfo, nullptr, &m_graphicsPipeline);
}

void VulkanBackend::CreateTextureSampler()
{
    VkSamplerCreateInfo vkSamplerInfo = {
        .sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .pNext                   = nullptr,
        .flags                   = 0,
        .magFilter               = VK_FILTER_NEAREST,
        .minFilter               = VK_FILTER_NEAREST,
        .mipmapMode              = VK_SAMPLER_MIPMAP_MODE_NEAREST,
        .addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .mipLodBias              = 0.0f,
        // NOTE(v.matushkin): To use anisotropy, set VkPhysicalDeviceFeatures::samplerAnisotropy = true
        //  and query its support with vkGetPhysicalDeviceFeatures
        .anisotropyEnable        = false,
        .maxAnisotropy           = 1.0f, // NOTE(v.matushkin): Query VkPhysicalDeviceProperties for limits.maxSamplerAnisotropy
        .compareEnable           = false, // ???
        .compareOp               = VK_COMPARE_OP_ALWAYS,
        .minLod                  = 0.0f,
        .maxLod                  = 0.0f,
        .borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK, // NOTE(v.matushkin): Only used for ADDRESS_MODE_CLAMP_TO_BORDER
        .unnormalizedCoordinates = false, // false - uv[0, 1) | true - uv[0, width) [0, height)
    };
    vkCreateSampler(m_device, &vkSamplerInfo, nullptr, &m_sampler);
}

void VulkanBackend::CreateUniformBuffers()
{
    VkBufferCreateInfo vkStagingBufferInfo = {
        .sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext                 = nullptr,
        .flags                 = 0,
        .size                  = sizeof(PerFrame),
        .usage                 = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices   = nullptr,
    };
    VkMemoryAllocateInfo vkAllocateInfo = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext           = nullptr,
        .memoryTypeIndex = m_bufferMemoryTypeIndex.CPU,
    };

    //- PerFrame
    for (ui32 i = 0; i < k_BackBufferFrames; ++i)
    {
        auto perFrameBuffer       = &m_ubPerFrame[i];
        auto perFrameBufferMemory = &m_ubPerFrameMemory[i];

        vkCreateBuffer(m_device, &vkStagingBufferInfo, nullptr, perFrameBuffer);

        VkMemoryRequirements vkMemoryRequirements;
        vkGetBufferMemoryRequirements(m_device, *perFrameBuffer, &vkMemoryRequirements);

        vkAllocateInfo.allocationSize = vkMemoryRequirements.size;
        vkAllocateMemory(m_device, &vkAllocateInfo, nullptr, perFrameBufferMemory);

        vkBindBufferMemory(m_device, *perFrameBuffer, *perFrameBufferMemory, 0);
    }
    //- PerDraw
    vkStagingBufferInfo.size = sizeof(PerDraw);

    for (ui32 i = 0; i < k_BackBufferFrames; ++i)
    {
        auto perDrawBuffer       = &m_ubPerDraw[i];
        auto perDrawBufferMemory = &m_ubPerDrawMemory[i];

        vkCreateBuffer(m_device, &vkStagingBufferInfo, nullptr, perDrawBuffer);

        VkMemoryRequirements vkMemoryRequirements;
        vkGetBufferMemoryRequirements(m_device, *perDrawBuffer, &vkMemoryRequirements);

        vkAllocateInfo.allocationSize = vkMemoryRequirements.size;
        vkAllocateMemory(m_device, &vkAllocateInfo, nullptr, perDrawBufferMemory);

        vkBindBufferMemory(m_device, *perDrawBuffer, *perDrawBufferMemory, 0);
    }
}

void VulkanBackend::CreateDescriptorPool()
{
    // TODO(v.matushkin): <DescriptorSet>
    VkDescriptorPoolSize vkDescriptorPoolSizes[] = {
        {
            .type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = k_BackBufferFrames * 2, // (PerFrame and PerDraw) * k_BackBufferFrames
        },
        {
            .type            = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .descriptorCount = k_MaxTextureDescriptors,
        }
    };

    // NOTE(v.matushkin): Flags?
    VkDescriptorPoolCreateInfo vkDescriptorPoolInfo = {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext         = nullptr,
        .flags         = 0,
        .maxSets       = k_BackBufferFrames + k_MaxTextureDescriptors,
        .poolSizeCount = ARRAYSIZE(vkDescriptorPoolSizes),
        .pPoolSizes    = vkDescriptorPoolSizes,
    };
    vkCreateDescriptorPool(m_device, &vkDescriptorPoolInfo, nullptr, &m_descriptorPool);
}

void VulkanBackend::CreateDescriptorSets()
{
    // TODO(v.matushkin): <DescriptorSet>
    //- Allocate DescriptorSets
    const VkDescriptorSetLayout descriptorSetLayouts[] = {
        m_descriptorSetLayoutCamera,
        m_descriptorSetLayoutCamera,
        m_descriptorSetLayoutCamera
    };

    VkDescriptorSetAllocateInfo vkDescriptorSetInfo = {
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext              = nullptr,
        .descriptorPool     = m_descriptorPool,
        .descriptorSetCount = ARRAYSIZE(descriptorSetLayouts),
        .pSetLayouts        = descriptorSetLayouts,
    };
    vkAllocateDescriptorSets(m_device, &vkDescriptorSetInfo, m_descriptorSets);

    //- Configure descriptors
    VkDescriptorBufferInfo vkDescriptorBufferInfos[k_BackBufferFrames * 2];
    VkWriteDescriptorSet   vkWriteDescriptorSets[k_BackBufferFrames * 2];

    for (ui32 i = 0; i < k_BackBufferFrames; ++i)
    {
        //- PerFrame
        vkDescriptorBufferInfos[i] = {
            .buffer = m_ubPerFrame[i],
            .offset = 0,
            .range  = sizeof(PerFrame),
        };
        vkWriteDescriptorSets[i] = {
            .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext            = nullptr,
            .dstSet           = m_descriptorSets[i],
            .dstBinding       = ShaderBinding::ubPerFrame,
            .dstArrayElement  = 0,
            .descriptorCount  = 1,
            .descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pImageInfo       = nullptr,
            .pBufferInfo      = &vkDescriptorBufferInfos[i],
            .pTexelBufferView = nullptr,
        };
        //- PerDraw
        const auto perDrawIndex = i + k_BackBufferFrames;

        vkDescriptorBufferInfos[perDrawIndex] = {
            .buffer = m_ubPerDraw[i],
            .offset = 0,
            .range  = sizeof(PerDraw),
        };
        vkWriteDescriptorSets[perDrawIndex] = {
            .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext            = nullptr,
            .dstSet           = m_descriptorSets[i],
            .dstBinding       = ShaderBinding::ubPerDraw,
            .dstArrayElement  = 0,
            .descriptorCount  = 1,
            .descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pImageInfo       = nullptr,
            .pBufferInfo      = &vkDescriptorBufferInfos[perDrawIndex],
            .pTexelBufferView = nullptr,
        };
    }

    // NOTE(v.matushkin): What is vkUpdateDescriptorSetWithTemplate?
    // NOTE(v.matushkin): Can I use VkCopyDescriptorSet somehow?
    vkUpdateDescriptorSets(m_device, ARRAYSIZE(vkWriteDescriptorSets), vkWriteDescriptorSets, 0, nullptr);
}

void VulkanBackend::CreateCommandPool()
{
    // NOTE(v.matushkin): VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT ?
    //  what do they use un Vulkan-Samples ?
    VkCommandPoolCreateInfo vkCommandPoolInfo = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext            = nullptr,
        .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = m_graphicsQueueFamily,
    };
    vkCreateCommandPool(m_device, &vkCommandPoolInfo, nullptr, &m_commandPool);
}

void VulkanBackend::FindMemoryTypeIndices()
{
    // NOTE(v.matushkin): VkPhysicalDeviceMemoryProperties2 ?
    VkPhysicalDeviceMemoryProperties vkMemoryProperties;
    vkGetPhysicalDeviceMemoryProperties(m_physiacalDevice, &vkMemoryProperties);

    //- CPU
    m_bufferMemoryTypeIndex.CPU = FindBufferMemoryTypeIndex(
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        vkMemoryProperties
    );
    //- CPUtoGPU
    m_bufferMemoryTypeIndex.CPUtoGPU = FindBufferMemoryTypeIndex(
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        vkMemoryProperties
    );
    //- GPU
    //-- Index Buffer
    m_bufferMemoryTypeIndex.GPUIndex = FindBufferMemoryTypeIndex(
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        vkMemoryProperties);
    //-- Vertex Buffer
    m_bufferMemoryTypeIndex.GPUVertex = FindBufferMemoryTypeIndex(
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        vkMemoryProperties
    );
    //-- Texture
    m_bufferMemoryTypeIndex.GPUTexture = FindImageMemoryTypeIndex(
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        vkMemoryProperties
    );

    LOG_INFO(
        "Memory Type Indices:\n\tCPUtoGPU: {}\n\tGPUIndex: {}\n\tGPUVertex: {}\n\tGPUTexture: {}",
        m_bufferMemoryTypeIndex.CPUtoGPU,
        m_bufferMemoryTypeIndex.GPUIndex,
        m_bufferMemoryTypeIndex.GPUVertex,
        m_bufferMemoryTypeIndex.GPUTexture
    );
}

void VulkanBackend::CreateCommandBuffers()
{
    VkCommandBufferAllocateInfo vkCommandBufferInfo = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext              = nullptr,
        .commandPool        = m_commandPool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = k_BackBufferFrames,
    };
    vkAllocateCommandBuffers(m_device, &vkCommandBufferInfo, m_commandBuffers);
}

void VulkanBackend::CreateSyncronizationObjects()
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
VkDebugUtilsMessengerCreateInfoEXT VulkanBackend::CreateDebugUtilsMessengerInfo()
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

void VulkanBackend::CreateDebugUtilsMessenger()
{
    const auto vkDebugUtilsMessengerInfo = CreateDebugUtilsMessengerInfo();
    vkCreateDebugUtilsMessengerEXT(m_instance, &vkDebugUtilsMessengerInfo, nullptr, &m_debugMessenger);
}
#endif // SNV_GPU_API_DEBUG_ENABLED


// NOTE(v.matushkin): Check for device extensions support?
bool VulkanBackend::IsPhysicalDeviceSuitable(VkPhysicalDevice physicalDevice, ui32& graphicsQueueFamily)
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

//void VulkanBackend::EnumerateInstanceLayerProperties()
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

//void VulkanBackend::EnumerateInstanceExtensionProperties()
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

//void VulkanBackend::CheckDeviceExtensionsSupport(VkPhysicalDevice physicalDevice)
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

ui32 VulkanBackend::FindBufferMemoryTypeIndex(
    VkBufferUsageFlags                      vkUsageFlags,
    VkMemoryPropertyFlags                   vkPropertyFlags,
    const VkPhysicalDeviceMemoryProperties& vkMemoryProperties
)
{
    //- Create dummy VkBuffer
    VkBufferCreateInfo vkBufferInfo = {
        .sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext                 = nullptr,
        .flags                 = 0,
        .size                  = 20,
        .usage                 = vkUsageFlags,
        .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0, // TODO(v.matushkin): This is for VK_SHARING_MODE_CONCURRENT ?
        .pQueueFamilyIndices   = nullptr,
    };
    VkBuffer vkBuffer;
    vkCreateBuffer(m_device, &vkBufferInfo, nullptr, &vkBuffer);

    // NOTE(v.matushkin): VkMemoryRequirements2, VkMemoryDedicatedRequirements ?
    VkMemoryRequirements vkMemoryRequirements;
    vkGetBufferMemoryRequirements(m_device, vkBuffer, &vkMemoryRequirements);
    vkDestroyBuffer(m_device, vkBuffer, nullptr);

    const auto memoryTypeBits = vkMemoryRequirements.memoryTypeBits;
    //- Find suitable memory type index
    for (ui32 i = 0; i < vkMemoryProperties.memoryTypeCount; ++i)
    {
        const auto propertyFlags         = vkMemoryProperties.memoryTypes[i].propertyFlags;
        const auto isRequiredMemoryType  = memoryTypeBits & (1 << i);
        const auto hasRequiredProperties = (propertyFlags & vkPropertyFlags) == vkPropertyFlags;
        if (isRequiredMemoryType && hasRequiredProperties)
        {
            return i;
        }
    }

    SNV_ASSERT(false, "Couldn't find required VkBuffer memory type");
}

ui32 VulkanBackend::FindImageMemoryTypeIndex(
    VkImageUsageFlags                       vkUsageFlags,
    VkMemoryPropertyFlags                   vkPropertyFlags,
    const VkPhysicalDeviceMemoryProperties& vkMemoryProperties
)
{
    //- Create dummy VkImage
    VkImageCreateInfo vkImageInfo = {
        .sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext                 = nullptr,
        .flags                 = 0,
        .imageType             = VK_IMAGE_TYPE_2D,
        .format                = VK_FORMAT_R8G8B8A8_UNORM,
        .extent                = {
            .width  = 4,
            .height = 4,
            .depth  = 1,
        },
        .mipLevels             = 1,
        .arrayLayers           = 1,
        .samples               = VK_SAMPLE_COUNT_1_BIT,
        .tiling                = VK_IMAGE_TILING_OPTIMAL,
        .usage                 = vkUsageFlags,
        .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,       // TODO(v.matushkin): The fuck is this?
        .pQueueFamilyIndices   = nullptr, // TODO(v.matushkin): The fuck is this?
        .initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    VkImage vkImage;
    vkCreateImage(m_device, &vkImageInfo, nullptr, &vkImage);

    // NOTE(v.matushkin): VkMemoryRequirements2, VkMemoryDedicatedRequirements ?
    VkMemoryRequirements vkMemoryRequirements;
    vkGetImageMemoryRequirements(m_device, vkImage, &vkMemoryRequirements);
    vkDestroyImage(m_device, vkImage, nullptr);

    const auto memoryTypeBits = vkMemoryRequirements.memoryTypeBits;
    //- Find suitable memory type index
    for (ui32 i = 0; i < vkMemoryProperties.memoryTypeCount; ++i)
    {
        const auto propertyFlags         = vkMemoryProperties.memoryTypes[i].propertyFlags;
        const auto isRequiredMemoryType  = memoryTypeBits & (1 << i);
        const auto hasRequiredProperties = (propertyFlags & vkPropertyFlags) == vkPropertyFlags;
        if (isRequiredMemoryType && hasRequiredProperties)
        {
            return i;
        }
    }

    SNV_ASSERT(false, "Couldn't find required VkImage memory type");
}

} // namespace snv
