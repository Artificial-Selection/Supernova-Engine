#include <Engine/Core/Log.hpp>
#include <Engine/Renderer/OpenGL/GLShader.hpp>

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

#include <memory>
#include <utility>


// NOTE(v.matushkin): Shader compilation errors checks shouldn't be under 'SNV_GPU_API_DEBUG_ENABLED' ?


namespace snv
{

//- RasterizerState
static GLenum gl_PolygonMode(PolygonMode polygonMode)
{
    static const GLenum openglPolygonMode[] = {
        GL_FILL,
        GL_LINE,
        // GL_POINT,
    };

    return openglPolygonMode[static_cast<ui8>(polygonMode)];
}

static GLenum gl_TriangleFrontFace(TriangleFrontFace triangleFrontFace)
{
    return triangleFrontFace == TriangleFrontFace::Clockwise ? GL_CW : GL_CCW;
}

//- DepthStencilState
static GLenum gl_CompareFunction(CompareFunction compareFunction)
{
    static const GLenum openglCompareFunction[] = {
        GL_NEVER,
        GL_LESS,
        GL_EQUAL,
        GL_LEQUAL,
        GL_GREATER,
        GL_NOTEQUAL,
        GL_GEQUAL,
        GL_ALWAYS,
    };

    return openglCompareFunction[static_cast<ui8>(compareFunction)];
}

//- BlendState
static GLenum gl_BlendOp(BlendOp blendOp)
{
    static const GLenum openglBlendOp[] = {
        GL_FUNC_ADD,
        GL_FUNC_SUBTRACT,
        GL_FUNC_REVERSE_SUBTRACT,
        GL_MIN,
        GL_MAX,
    };

    return openglBlendOp[static_cast<ui8>(blendOp)];
}

static GLenum gl_BlendFactor(BlendFactor blendFactor)
{
    static const GLenum openglBlendFactor[] = {
        GL_ZERO,
        GL_ONE,
        GL_SRC_COLOR,
        GL_ONE_MINUS_SRC_COLOR,
        GL_DST_COLOR,
        GL_ONE_MINUS_DST_COLOR,
        GL_SRC_ALPHA,
        GL_ONE_MINUS_SRC_ALPHA,
        GL_DST_ALPHA,
        GL_ONE_MINUS_DST_ALPHA,
        GL_SRC_ALPHA_SATURATE,
        GL_SRC1_COLOR,
        GL_ONE_MINUS_SRC1_COLOR,
        GL_SRC1_ALPHA,
        GL_ONE_MINUS_SRC1_ALPHA,
    };

    return openglBlendFactor[static_cast<ui8>(blendFactor)];
}

static GLenum gl_BlendLogicOp(BlendLogicOp blendLogicOp)
{
    static const GLenum openglBlendLogicOp[] = {
        GL_CLEAR,
        GL_SET,
        GL_COPY,
        GL_COPY_INVERTED,
        GL_NOOP,
        GL_INVERT,
        GL_AND,
        GL_NAND,
        GL_OR,
        GL_NOR,
        GL_XOR,
        GL_EQUIV,
        GL_AND_REVERSE,
        GL_AND_INVERTED,
        GL_OR_REVERSE,
        GL_OR_INVERTED,
    };

    return openglBlendLogicOp[static_cast<ui8>(blendLogicOp)];
}


GLShader::GLShader() noexcept
    : m_shaderProgramID(k_InvalidHandle)
{}

GLShader::GLShader(const ShaderDesc& shaderDesc)
{
    const auto vertexShaderID   = CreateShader(shaderDesc.VertexSource.data(), GL_VERTEX_SHADER);
    const auto fragmentShaderID = CreateShader(shaderDesc.FragmentSource.data(), GL_FRAGMENT_SHADER);

    CheckShaderCompilationStatus(vertexShaderID);
    CheckShaderCompilationStatus(fragmentShaderID);

    m_shaderProgramID = CreateShaderProgram(vertexShaderID, fragmentShaderID);

    CheckShaderProgramLinkStatus(m_shaderProgramID);

    glDeleteShader(vertexShaderID);
    glDeleteShader(fragmentShaderID);

    //- RasterizerState
    {
        const auto rasterizerStateDesc = shaderDesc.RasterizerStateDesc;

        // NOTE(v.matushkin): No point in setting FrontFace per shader in OpenGL? All engine shaders are GL_CCW and ImGui is GL_CW
        m_rasterizerState = {
            .PolygonMode = gl_PolygonMode(rasterizerStateDesc.PolygonMode),
            .FrontFace   = gl_TriangleFrontFace(rasterizerStateDesc.FrontFace),
        };

        if (rasterizerStateDesc.CullMode != CullMode::Off)
        {
            m_rasterizerState.FacetCullingEnable = true;
            m_rasterizerState.CullFace           = rasterizerStateDesc.CullMode == CullMode::Front ? GL_FRONT : GL_BACK;
        }
    }
    //- DepthStencilState
    {
        const auto depthStencilStateDesc = shaderDesc.DepthStencilStateDesc;

        m_depthStencilState = {
            .DepthTestEnable      = depthStencilStateDesc.DepthTestEnable,
            .DepthWriteEnable     = depthStencilStateDesc.DepthWriteEnable,
            .DepthCompareFunction = gl_CompareFunction(depthStencilStateDesc.DepthCompareFunction),
        };
    }
    //- BlendState
    {
        const auto blendStateDesc = shaderDesc.BlendStateDesc;

        if (blendStateDesc.BlendMode == BlendMode::Off)
        {
            m_blendState = {};
        }
        else if (blendStateDesc.BlendMode == BlendMode::BlendOp)
        {
            m_blendState = {
                .BlendEnable         = true,
                .ColorBlendOp        = gl_BlendOp(blendStateDesc.ColorBlendOp),
                .AlphaBlendOp        = gl_BlendOp(blendStateDesc.AlphaBlendOp),
                .ColorSrcBlendFactor = gl_BlendFactor(blendStateDesc.ColorSrcBlendFactor),
                .ColorDstBlendFactor = gl_BlendFactor(blendStateDesc.ColorDstBlendFactor),
                .AlphaSrcBlendFactor = gl_BlendFactor(blendStateDesc.AlphaSrcBlendFactor),
                .AlphaDstBlendFactor = gl_BlendFactor(blendStateDesc.AlphaDstBlendFactor),
            };
        }
        else // blendStateDesc.BlendMode == BlendMode::LogicOp
        {
            m_blendState = {
                .LogicOpEnable = true,
                .LogicOp       = gl_BlendLogicOp(blendStateDesc.LogicOp),
            };
        }
    }
}

GLShader::GLShader(GLShader&& other) noexcept
    : m_shaderProgramID(std::exchange(other.m_shaderProgramID, k_InvalidHandle))
    , m_rasterizerState(std::exchange(other.m_rasterizerState, {}))
    , m_depthStencilState(std::exchange(other.m_depthStencilState, {}))
    , m_blendState(std::exchange(other.m_blendState, {}))
{}

GLShader& GLShader::operator=(GLShader&& other) noexcept
{
    m_shaderProgramID   = std::exchange(other.m_shaderProgramID, k_InvalidHandle);
    m_rasterizerState   = std::exchange(other.m_rasterizerState, {});
    m_depthStencilState = std::exchange(other.m_depthStencilState, {});
    m_blendState        = std::exchange(other.m_blendState, {});

    return *this;
}


void GLShader::Bind() const
{
    glUseProgram(m_shaderProgramID);

    //- RasterizerState
    glPolygonMode(GL_FRONT_AND_BACK, m_rasterizerState.PolygonMode);
    if (m_rasterizerState.FacetCullingEnable)
    {
        glEnable(GL_CULL_FACE);
        glCullFace(m_rasterizerState.CullFace);
    }
    else
    {
        glDisable(GL_CULL_FACE);
    }
    glFrontFace(m_rasterizerState.FrontFace);

    //- DepthStencilState
    if (m_depthStencilState.DepthTestEnable)
    {
        glEnable(GL_DEPTH_TEST);
        glDepthMask(m_depthStencilState.DepthWriteEnable);
        glDepthFunc(m_depthStencilState.DepthCompareFunction);
    }
    else
    {
        glDisable(GL_DEPTH_TEST);
    }

    //- BlendState
    if (m_blendState.BlendEnable)
    {
        glEnable(GL_BLEND);
        glBlendEquationSeparate(m_blendState.ColorBlendOp, m_blendState.AlphaBlendOp);
        glBlendFuncSeparate(
            m_blendState.ColorSrcBlendFactor,
            m_blendState.ColorDstBlendFactor,
            m_blendState.AlphaSrcBlendFactor,
            m_blendState.AlphaDstBlendFactor
        );
    }
    else if (m_blendState.LogicOpEnable)
    {
        glEnable(GL_COLOR_LOGIC_OP);
        glLogicOp(m_blendState.LogicOp);
    }
    else
    {
        glDisable(GL_BLEND);
        glDisable(GL_COLOR_LOGIC_OP);
    }
}


void GLShader::SetInt1(const std::string& name, i32 value) const
{
    auto location = glGetUniformLocation(m_shaderProgramID, name.c_str());
    glProgramUniform1i(m_shaderProgramID, location, value);
}

void GLShader::SetMatrix4(const std::string& name, const glm::mat4& value) const
{
    auto location = glGetUniformLocation(m_shaderProgramID, name.c_str());
    glProgramUniformMatrix4fv(m_shaderProgramID, location, 1, GL_FALSE, glm::value_ptr(value));
}


ui32 GLShader::CreateShader(const char* shaderSource, ui32 shaderType)
{
    const auto shaderID = glCreateShader(shaderType);
    glShaderSource(shaderID, 1, &shaderSource, nullptr);
    glCompileShader(shaderID);

    return shaderID;
}

ui32 GLShader::CreateShaderProgram(i32 vertexShaderID, i32 fragmentShaderID)
{
    ui32 shaderProgramID = glCreateProgram();

    glAttachShader(shaderProgramID, vertexShaderID);
    glAttachShader(shaderProgramID, fragmentShaderID);
    glLinkProgram(shaderProgramID);

    return shaderProgramID;
}


#ifdef SNV_GPU_API_DEBUG_ENABLED

void GLShader::CheckShaderCompilationStatus(ui32 shaderID)
{
    i32 isCompiled;
    glGetShaderiv(shaderID, GL_COMPILE_STATUS, &isCompiled);

    if (isCompiled == GL_FALSE)
    {
        i32 logLength;
        glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &logLength);

        auto errorLog = std::make_unique<char[]>(logLength);
        glGetShaderInfoLog(shaderID, logLength, nullptr, errorLog.get());
        LOG_ERROR("OpenGL Shader compilation error, Log:\n{)", errorLog.get());
    }
}

void GLShader::CheckShaderProgramLinkStatus(ui32 shaderProgramID)
{
    i32 isCompiled;
    glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &isCompiled);

    if (isCompiled == GL_FALSE)
    {
        i32 logLength;
        glGetProgramiv(shaderProgramID, GL_INFO_LOG_LENGTH, &logLength);

        auto errorLog = std::make_unique<char[]>(logLength);
        glGetProgramInfoLog(shaderProgramID, logLength, NULL, errorLog.get());
        LOG_ERROR("OpenGL ShaderProgram link error, Log:\n{)", errorLog.get());
    }
}

#endif // SNV_GPU_API_DEBUG_ENABLED

} // namespace snv
