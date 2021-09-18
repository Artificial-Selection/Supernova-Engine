#pragma once

#include <Engine/Core/Core.hpp>
#include <Engine/Renderer/IRendererBackend.hpp>

#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>


namespace snv
{

class VKBackend final : public IRendererBackend
{
    static const ui32 k_BackBufferFrames = 3;

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
    void CreateGraphicsPipeline();

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

    VkSurfaceKHR             m_surface;
    VkSwapchainKHR           m_swapchain;
    VkImageView              m_backBuffers[k_BackBufferFrames];

#ifdef SNV_GPU_API_DEBUG_ENABLED
    VkDebugUtilsMessengerEXT m_debugMessenger;
#endif

    ui32 m_graphicsQueueFamily;
};

} // namespace snv
