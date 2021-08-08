#pragma once

#include <Core/Core.hpp>
#include <Renderer/IRendererBackend.hpp>
#include <Renderer/OpenGL/GLBuffer.hpp>
#include <Renderer/OpenGL/GLShader.hpp>
#include <Renderer/OpenGL/GLTexture.hpp>

#include <unordered_map>


namespace snv
{

struct VertexAttributeDescriptor;


class GLBackend final : public IRendererBackend
{
public:
    GLBackend();
    ~GLBackend() override = default;

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
    std::unordered_map<BufferHandle,  GLBuffer>  m_buffers;
    std::unordered_map<TextureHandle, GLTexture> m_textures;
    std::unordered_map<ShaderHandle,  GLShader>  m_shaders;
};

} // namespace snv
