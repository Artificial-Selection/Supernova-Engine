#pragma once

#include <Engine/Core/Core.hpp>
#include <Engine/Renderer/IRendererBackend.hpp>

#include <glm/ext/matrix_float4x4.hpp>
#include <vulkan/vulkan.h>

#include <unordered_map>


namespace snv
{

class VKBackend final : public IRendererBackend
{
    static const ui32 k_BackBufferFrames = 3;


    struct VKBuffer
    {
        VkBuffer Index;
        VkBuffer Position;
        VkBuffer Normal;
        VkBuffer TexCoord0;

        VkDeviceMemory IndexMemory;
        VkDeviceMemory PositionMemory;
        VkDeviceMemory NormalMemory;
        VkDeviceMemory TexCoord0Memory;
    };

    struct VKShader
    {
        VkShaderModule Vertex;
        VkShaderModule Fragment;
    };


    // TODO(v.matushkin): PerFrame/PerDraw should be declared in some common header
    struct alignas(256) PerFrame
    {
        glm::mat4x4 _CameraView;
        glm::mat4x4 _CameraProjection;
    };
    struct alignas(256) PerDraw
    {
        glm::mat4x4 _ObjectToWorld;
    };


    struct VKBufferMemoryTypeIndex
    {
        ui32 CPU;

        ui32 CPUtoGPU;

        ui32 VertexGPU;
        ui32 IndexGPU; // NOTE(v.matushkin): This can't be different from VertexGPU, right?
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
    void CreateDescriptorSetLayout();
    void CreatePipeline();

    void CreateUniformBuffers();
    void CreateDescriptorPool();
    void CreateDescriptorSets();

    void CreateCommandPool();
    void FindMemoryTypeIndices();
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
    ui32 FindMemoryTypeIndex(
        VkBufferUsageFlags                      vkUsageFlags,
        VkMemoryPropertyFlags                   vkPropertyFlags,
        const VkPhysicalDeviceMemoryProperties& vkMemoryProperties
    );

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

    // TODO(v.matushkin): Useless VkDescriptorSetLayout, VkPipelineLayout members? Why have them?
    //  They're only used to create VkPipeline. To reuse them?
    VkDescriptorSetLayout    m_descriptorSetLayout;
    VkPipelineLayout         m_pipelineLayout;
    VkRenderPass             m_renderPass;
    VkPipeline               m_graphicsPipeline;

    VkCommandPool            m_commandPool;
    VkCommandBuffer          m_commandBuffers[k_BackBufferFrames];

    VkSemaphore              m_semaphoreImageAvailable[k_BackBufferFrames];
    VkSemaphore              m_semaphoreRenderFinished[k_BackBufferFrames];
    VkFence                  m_fences[k_BackBufferFrames];
    ui32                     m_currentFrame;

    VkDescriptorPool         m_descriptorPool;
    VkDescriptorSet          m_descriptorSets[k_BackBufferFrames];

#ifdef SNV_GPU_API_DEBUG_ENABLED
    VkDebugUtilsMessengerEXT m_debugMessenger;
#endif

    VKBufferMemoryTypeIndex  m_bufferMemoryTypeIndex;

    VkBuffer                 m_ubPerFrame[k_BackBufferFrames];
    VkBuffer                 m_ubPerDraw[k_BackBufferFrames];
    VkDeviceMemory           m_ubPerFrameMemory[k_BackBufferFrames];
    VkDeviceMemory           m_ubPerDrawMemory[k_BackBufferFrames];


    VkClearValue             m_clearValue; // NOTE(v.matushkin): Different from other backends, default initialized

    std::unordered_map<BufferHandle, VKBuffer> m_buffers;
    std::unordered_map<ShaderHandle, VKShader> m_shaders;
};

} // namespace snv
