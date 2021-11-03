#pragma once

#include <Engine/Core/Core.hpp>
#include <Engine/Renderer/IRendererBackend.hpp>

#include <glm/ext/matrix_float4x4.hpp>
#include <vulkan/vulkan.h>

#include <unordered_map>


namespace snv
{

class VulkanBackend final : public IRendererBackend
{
    static const ui32 k_BackBufferFrames      = 3;
    static const ui32 k_MaxTextureDescriptors = 300;


    struct VulkanBuffer
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

    struct VulkanTexture
    {
        VkImageView    View;
        VkImage        Image;
        VkDeviceMemory Memory;

        ui32 DescriptorSetIndex;
    };

    struct VulkanShader
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


    // TODO(v.matushkin): Is this shit even valid?
    struct VulkanMemoryTypeIndex
    {
        ui32 CPU;
        ui32 CPUtoGPU;
        ui32 GPUVertex;
        ui32 GPUIndex;   // NOTE(v.matushkin): This can't be different from VertexGPU, right?
        ui32 GPUTexture; // NOTE(v.matushkin): Will this be different from VkBuffer ?
    };


public:
    VulkanBackend();
    ~VulkanBackend() override;

    void EnableBlend() override;
    void EnableDepthTest() override;

    [[nodiscard]] void* GetNativeRenderTexture(RenderTextureHandle renderTextureHandle) override { return nullptr; }

    void SetBlendFunction(BlendFactor source, BlendFactor destination) override;
    void SetClearColor(f32 r, f32 g, f32 b, f32 a) override;
    void SetDepthFunction(DepthFunction depthFunction) override;
    void SetViewport(i32 x, i32 y, i32 width, i32 height) override;

    void Clear(BufferBit bufferBitMask) override;

    void BeginFrame(const glm::mat4x4& localToWorld, const glm::mat4x4& cameraView, const glm::mat4x4& cameraProjection) override;
    void BeginRenderPass(FramebufferHandle framebufferHandle) override {}
    void EndFrame() override;

    void DrawBuffer(TextureHandle textureHandle, BufferHandle bufferHandle, i32 indexCount, i32 vertexCount) override;

    [[nodiscard]] IImGuiRenderContext* CreateImGuiRenderContext() override { return nullptr; }

    [[nodiscard]] GraphicsState CreateGraphicsState(const GraphicsStateDesc& graphicsStateDesc) override { return {}; }
    [[nodiscard]] BufferHandle  CreateBuffer(
        std::span<const std::byte>              indexData,
        std::span<const std::byte>              vertexData,
        const std::vector<VertexAttributeDesc>& vertexLayout
    ) override;
    [[nodiscard]] TextureHandle CreateTexture(const TextureDesc& textureDesc, const ui8* textureData) override;
    [[nodiscard]] ShaderHandle  CreateShader(std::span<const char> vertexSource, std::span<const char> fragmentSource) override;

private:
    void CreateInstance();
    void CreateSurface();
    void CreateDevice();
    void CreateSwapchain();
    void CreateDepthBuffer();
    void CreateRenderPass();
    void CreateFramebuffers();

    void CreateUniformBuffers();
    void CreateTextureSampler(); // TODO(v.matushkin): This should be removed. Textures should have individual samplers
    void CreateDescriptorPool();
    void CreateDescriptorSetLayouts();
    void CreateDescriptorSets();

    void CreatePipeline();

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
    ui32 FindBufferMemoryTypeIndex(
        VkBufferUsageFlags                      vkUsageFlags,
        VkMemoryPropertyFlags                   vkPropertyFlags,
        const VkPhysicalDeviceMemoryProperties& vkMemoryProperties
    );
    ui32 FindImageMemoryTypeIndex(
        VkImageUsageFlags                       vkUsageFlags,
        VkMemoryPropertyFlags                   vkPropertyFlags,
        const VkPhysicalDeviceMemoryProperties& vkMemoryProperties
    );

private:
    //- Vulkan
    VkInstance               m_instance;
    VkPhysicalDevice         m_physiacalDevice;
    VkDevice                 m_device;
    VkQueue                  m_graphicsQueue;
    ui32                     m_graphicsQueueFamily;
    //-- Surface
    VkSurfaceKHR             m_surface;
    VkSwapchainKHR           m_swapchain;
    VkExtent2D               m_swapchainExtent;

    VkImageView              m_backBuffers[k_BackBufferFrames];
    VkFramebuffer            m_framebuffers[k_BackBufferFrames];

    VkImage                  m_depthImage;
    VkImageView              m_depthImageView;
    VkDeviceMemory           m_depthImageMemory;

    ui32                     m_currentBackBufferIndex;
    //-- Pipeline
    // TODO(v.matushkin): Useless VkDescriptorSetLayout, VkPipelineLayout members? Why have them?
    //  They're only used to create VkPipeline. To reuse them?
    VkPipelineLayout         m_pipelineLayout;
    VkRenderPass             m_renderPass;
    VkPipeline               m_graphicsPipeline;
    //-- Command Buffers
    VkCommandPool            m_commandPool;
    VkCommandBuffer          m_commandBuffers[k_BackBufferFrames];
    //-- Syncronization Objects
    VkSemaphore              m_semaphoreImageAvailable[k_BackBufferFrames];
    VkSemaphore              m_semaphoreRenderFinished[k_BackBufferFrames];
    VkFence                  m_fences[k_BackBufferFrames];
    ui32                     m_currentFrame;
    //-- Descriptors
    VkDescriptorSetLayout    m_descriptorSetLayoutCamera;
    VkDescriptorSetLayout    m_descriptorSetLayourMaterial;

    VkDescriptorPool         m_descriptorPool;

    VkDescriptorSet          m_descriptorSets[k_BackBufferFrames];
    VkDescriptorSet          m_descriptorSetMaterials[k_MaxTextureDescriptors];

#ifdef SNV_GPU_API_DEBUG_ENABLED
    VkDebugUtilsMessengerEXT m_debugMessenger;
#endif

    VulkanMemoryTypeIndex    m_bufferMemoryTypeIndex;

    VkSampler                m_sampler;

    VkBuffer                 m_ubPerFrame[k_BackBufferFrames];
    VkBuffer                 m_ubPerDraw[k_BackBufferFrames];
    VkDeviceMemory           m_ubPerFrameMemory[k_BackBufferFrames];
    VkDeviceMemory           m_ubPerDrawMemory[k_BackBufferFrames];


    VkClearValue             m_clearValues[2]; // 0 - color, 1 - depth

    std::unordered_map<BufferHandle,  VulkanBuffer>  m_buffers;
    std::unordered_map<TextureHandle, VulkanTexture> m_textures;
    std::unordered_map<ShaderHandle,  VulkanShader>  m_shaders;
};

} // namespace snv
