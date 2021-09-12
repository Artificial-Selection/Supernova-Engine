#pragma once

#include <Engine/Core/Core.hpp>
#include <Engine/Renderer/IRendererBackend.hpp>

#include <vulkan/vulkan.h>


namespace snv
{

class VKBackend final : public IRendererBackend
{
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

#ifdef SNV_GPU_API_DEBUG_ENABLED
    VkDebugUtilsMessengerCreateInfoEXT CreateDebugUtilsMessengerInfo();
    void CreateDebugUtilsMessenger();
#endif

    // void EnumerateInstanceLayerProperties();
    // void EnumerateInstanceExtensionProperties();

private:
    VkInstance               m_instance;

#ifdef SNV_GPU_API_DEBUG_ENABLED
    VkDebugUtilsMessengerEXT m_debugMessenger;
#endif
};

} // namespace snv
