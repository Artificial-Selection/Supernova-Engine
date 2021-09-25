#pragma once

#include <Engine/Core/Core.hpp>
#include <Engine/Renderer/IRendererBackend.hpp>

#include <vulkan/vulkan.h>

#include <unordered_map>


namespace snv
{

class VKBackend final : public IRendererBackend
{
    static const ui32 k_BackBufferFrames = 3;


    struct VKShader
    {
        VkShaderModule Vertex;
        VkShaderModule Fragment;
    };


public:
    VKBackend();
    ~VKBackend() override;

    void EnableBlend() override;
    void EnableDepthTest() override;

    void SetBlendFunction(BlendFactor source, BlendFactor destination) override;
    void SetClearColor(f32 r, f32 g, f32 b, f32 a) override;
    void SetDepthFunction(DepthFunction depthFunction) override;
    void SetViewport(i32 x, i32 y, i32 width, i32 height) override;

    void Clear(BufferBit bufferBitMask) override;

    void BeginFrame(const glm::mat4x4& localToWorld, const glm::mat4x4& cameraView, const glm::mat4x4& cameraProjection) override;
    void EndFrame() override;
    void DrawBuffer(TextureHandle textureHandle, BufferHandle bufferHandle, i32 indexCount, i32 vertexCount) override;
    void DrawArrays(i32 count) override;
    void DrawElements(i32 count) override;

    BufferHandle CreateBuffer(
        std::span<const std::byte>              indexData,
        std::span<const std::byte>              vertexData,
        const std::vector<VertexAttributeDesc>& vertexLayout
    ) override;
    TextureHandle CreateTexture(const TextureDesc& textureDesc, const ui8* textureData) override;
    ShaderHandle  CreateShader(std::span<const char> vertexSource, std::span<const char> fragmentSource) override;

private:
    void CreateInstance();
    void CreateSurface();
    void CreateDevice();
    void CreateSwapchain();
    void CreateRenderPass();
    void CreateFramebuffers();
    void CreatePipeline();

    void CreateCommandPool();
    // NOTE(v.matushkin): Should be reworked, can't prerecord commandbuffer in the real world
    void CreateCommandBuffers();
    void CreateSyncronizationObjects();

#ifdef SNV_GPU_API_DEBUG_ENABLED
    VkDebugUtilsMessengerCreateInfoEXT CreateDebugUtilsMessengerInfo();
    void CreateDebugUtilsMessenger();
#endif

    // Helpers
    bool IsPhysicalDeviceSuitable(VkPhysicalDevice physicalDevice, ui32& graphicsQueueFamily);
    // void EnumerateInstanceLayerProperties();
    // void EnumerateInstanceExtensionProperties();
    // void CheckDeviceExtensionsSupport(VkPhysicalDevice physicalDevice);

private:
    VkInstance               m_instance;
    VkPhysicalDevice         m_physiacalDevice;
    VkDevice                 m_device;
    VkQueue                  m_graphicsQueue;
    ui32                     m_graphicsQueueFamily;

    VkSurfaceKHR             m_surface;
    VkSwapchainKHR           m_swapchain;
    VkExtent2D               m_swapchainExtent;
    VkImageView              m_backBuffers[k_BackBufferFrames];
    VkFramebuffer            m_framebuffers[k_BackBufferFrames];
    ui32                     m_currentBackBufferIndex;

    VkRenderPass             m_renderPass;
    VkPipelineLayout         m_pipelineLayout;
    VkPipeline               m_graphicsPipeline;

    VkCommandPool            m_commandPool;
    VkCommandBuffer          m_commandBuffers[k_BackBufferFrames];

    VkSemaphore              m_semaphoreImageAvailable[k_BackBufferFrames];
    VkSemaphore              m_semaphoreRenderFinished[k_BackBufferFrames];
    VkFence                  m_fences[k_BackBufferFrames];
    ui32                     m_currentFrame;

#ifdef SNV_GPU_API_DEBUG_ENABLED
    VkDebugUtilsMessengerEXT m_debugMessenger;
#endif


    VkClearValue             m_clearValue; // NOTE(v.matushkin): Different from other backends, default initialized

    std::unordered_map<ShaderHandle, VKShader> m_shaders;
};

} // namespace snv
