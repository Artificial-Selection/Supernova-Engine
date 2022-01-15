#pragma once

#include <Engine/Core/Core.hpp>
#include <Engine/Renderer/RenderTypes.hpp>

#include <glm/ext/matrix_float4x4.hpp>

#include <string>


namespace snv
{

class GLShader
{
    struct GLRasterizerState
    {
        ui32 PolygonMode;
        bool FacetCullingEnable;
        ui32 CullFace;
        ui32 FrontFace;
    };

    struct GLDepthStencilState
    {
        bool DepthTestEnable;
        bool DepthWriteEnable;
        ui32 DepthCompareFunction;
    };

    struct GLBlendState
    {
        bool BlendEnable;
        bool LogicOpEnable;
        ui32 ColorBlendOp;
        ui32 AlphaBlendOp;
        ui32 ColorSrcBlendFactor;
        ui32 ColorDstBlendFactor;
        ui32 AlphaSrcBlendFactor;
        ui32 AlphaDstBlendFactor;
        ui32 LogicOp;
    };

public:
    // NOTE(v.matushkin): Can I make this move only without default constructor?
    // TODO(v.matushkin): Define destructor
    GLShader() noexcept;
    GLShader(const ShaderDesc& shaderDesc);

    GLShader(GLShader&& other) noexcept;
    GLShader& operator=(GLShader&& other) noexcept;

    GLShader(const GLShader& other) = delete;
    GLShader& operator=(const GLShader& other) = delete;

    void Bind() const;

    void SetInt1(const std::string& name, i32 value) const;
    void SetMatrix4(const std::string& name, const glm::mat4& value) const;

private:
    static ui32 CreateShader(const char* shaderSource, ui32 shaderType);
    static ui32 CreateShaderProgram(i32 vertexShaderID, i32 fragmentShaderID);

    static void CheckShaderCompilationStatus(ui32 shaderID);
    static void CheckShaderProgramLinkStatus(ui32 shaderProgramID);

private:
    ui32                m_shaderProgramID;

    GLRasterizerState   m_rasterizerState;
    GLDepthStencilState m_depthStencilState;
    GLBlendState        m_blendState;
};

}  // namespace snv
