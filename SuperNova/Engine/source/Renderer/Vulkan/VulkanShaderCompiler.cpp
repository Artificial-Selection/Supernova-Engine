#include <Engine/Renderer/Vulkan/VulkanShaderCompiler.hpp>
#include <Engine/Core/Log.hpp>

#include <glslang/SPIRV/GlslangToSpv.h>


// NOTE(v.matushkin): Just a quick/simple implementation
//  everything in this class could be done better, but I just wanna bootstrap Vulkan as fast as I can


const TBuiltInResource k_Resource = {
    .maxLights                                 = 32,
    .maxClipPlanes                             = 6,
    .maxTextureUnits                           = 32,
    .maxTextureCoords                          = 32,
    .maxVertexAttribs                          = 64,
    .maxVertexUniformComponents                = 4096,
    .maxVaryingFloats                          = 64,
    .maxVertexTextureImageUnits                = 32,
    .maxCombinedTextureImageUnits              = 80,
    .maxTextureImageUnits                      = 32,
    .maxFragmentUniformComponents              = 4096,
    .maxDrawBuffers                            = 32,
    .maxVertexUniformVectors                   = 128,
    .maxVaryingVectors                         = 8,
    .maxFragmentUniformVectors                 = 16,
    .maxVertexOutputVectors                    = 16,
    .maxFragmentInputVectors                   = 15,
    .minProgramTexelOffset                     = -8,
    .maxProgramTexelOffset                     = 7,
    .maxClipDistances                          = 8,
    .maxComputeWorkGroupCountX                 = 65535,
    .maxComputeWorkGroupCountY                 = 65535,
    .maxComputeWorkGroupCountZ                 = 65535,
    .maxComputeWorkGroupSizeX                  = 1024,
    .maxComputeWorkGroupSizeY                  = 1024,
    .maxComputeWorkGroupSizeZ                  = 64,
    .maxComputeUniformComponents               = 1024,
    .maxComputeTextureImageUnits               = 16,
    .maxComputeImageUniforms                   = 8,
    .maxComputeAtomicCounters                  = 8,
    .maxComputeAtomicCounterBuffers            = 1,
    .maxVaryingComponents                      = 60,
    .maxVertexOutputComponents                 = 64,
    .maxGeometryInputComponents                = 64,
    .maxGeometryOutputComponents               = 128,
    .maxFragmentInputComponents                = 128,
    .maxImageUnits                             = 8,
    .maxCombinedImageUnitsAndFragmentOutputs   = 8,
    .maxCombinedShaderOutputResources          = 8,
    .maxImageSamples                           = 0,
    .maxVertexImageUniforms                    = 0,
    .maxTessControlImageUniforms               = 0,
    .maxTessEvaluationImageUniforms            = 0,
    .maxGeometryImageUniforms                  = 0,
    .maxFragmentImageUniforms                  = 8,
    .maxCombinedImageUniforms                  = 8,
    .maxGeometryTextureImageUnits              = 16,
    .maxGeometryOutputVertices                 = 256,
    .maxGeometryTotalOutputComponents          = 1024,
    .maxGeometryUniformComponents              = 1024,
    .maxGeometryVaryingComponents              = 64,
    .maxTessControlInputComponents             = 128,
    .maxTessControlOutputComponents            = 128,
    .maxTessControlTextureImageUnits           = 16,
    .maxTessControlUniformComponents           = 1024,
    .maxTessControlTotalOutputComponents       = 4096,
    .maxTessEvaluationInputComponents          = 128,
    .maxTessEvaluationOutputComponents         = 128,
    .maxTessEvaluationTextureImageUnits        = 16,
    .maxTessEvaluationUniformComponents        = 1024,
    .maxTessPatchComponents                    = 120,
    .maxPatchVertices                          = 32,
    .maxTessGenLevel                           = 64,
    .maxViewports                              = 16,
    .maxVertexAtomicCounters                   = 0,
    .maxTessControlAtomicCounters              = 0,
    .maxTessEvaluationAtomicCounters           = 0,
    .maxGeometryAtomicCounters                 = 0,
    .maxFragmentAtomicCounters                 = 8,
    .maxCombinedAtomicCounters                 = 8,
    .maxAtomicCounterBindings                  = 1,
    .maxVertexAtomicCounterBuffers             = 0,
    .maxTessControlAtomicCounterBuffers        = 0,
    .maxTessEvaluationAtomicCounterBuffers     = 0,
    .maxGeometryAtomicCounterBuffers           = 0,
    .maxFragmentAtomicCounterBuffers           = 1,
    .maxCombinedAtomicCounterBuffers           = 1,
    .maxAtomicCounterBufferSize                = 16384,
    .maxTransformFeedbackBuffers               = 4,
    .maxTransformFeedbackInterleavedComponents = 64,
    .maxCullDistances                          = 8,
    .maxCombinedClipAndCullDistances           = 8,
    .maxSamples                                = 4,
    .limits = {
        .nonInductiveForLoops                 = true,
        .whileLoops                           = true,
        .doWhileLoops                         = true,
        .generalUniformIndexing               = true,
        .generalAttributeMatrixVectorIndexing = true,
        .generalVaryingIndexing               = true,
        .generalSamplerIndexing               = true,
        .generalVariableIndexing              = true,
        .generalConstantMatrixVectorIndexing  = true,
    }
};


namespace snv::VulkanShaderCompiler
{

void Init()
{
    if (glslang::InitializeProcess() == false)
    {
        LOG_ERROR("Failed to initialize glslang");
    }
}

void Shutdown()
{
    glslang::FinalizeProcess();
}


std::vector<ui32> CompileShader(ShaderType shaderType, std::span<const char> shaderSource)
{
    std::vector<ui32> spirvBytecode;

    const auto eshLanguage = static_cast<EShLanguage>(shaderType);
    auto fucking_const_char_const = shaderSource.data();
    const auto messages = static_cast<EShMessages>(EShMsgVulkanRules | EShMsgSpvRules | EShMsgDefault);
    const auto glslVersion = 460;

    glslang::TShader shader(eshLanguage);
    shader.setStrings(&fucking_const_char_const, 1);
    shader.setEnvInput(glslang::EShSourceGlsl, eshLanguage, glslang::EShClientVulkan, glslVersion);
    shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_2);
    shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_5);

    if (shader.parse(&k_Resource, glslVersion, false, messages) == false)
    {
        LOG_ERROR(
            "glslang::TShader preprocess failed\n"
            "\tInfo log:\n{}"
            "\tDebug Info log:\n{}",
            shader.getInfoLog(), shader.getInfoDebugLog()
        );
        return spirvBytecode;
    }

    glslang::TProgram program;
    program.addShader(&shader);

    if (program.link(messages) == false || program.mapIO() == false)
    {
        LOG_ERROR(
            "glslang::TProgram linking failed\n{}"
            "\tInfo log:\n{}"
            "\tDebug Info log:\n{}",
            program.getInfoLog(), program.getInfoDebugLog()
        );
        return spirvBytecode;
    }

    glslang::SpvOptions spvOptions;
#ifdef SNV_GPU_API_DEBUG_ENABLED
    spvOptions.generateDebugInfo = true;
    spvOptions.disableOptimizer  = true;
    spvOptions.optimizeSize      = false;
#endif
    auto intermediate = program.getIntermediate(eshLanguage);

    spv::SpvBuildLogger spvLogger;
    glslang::GlslangToSpv(*intermediate, spirvBytecode, &spvLogger, &spvOptions);

    const auto spvLogMessage = spvLogger.getAllMessages();
    if (spvLogMessage.empty() == false)
    {
        LOG_WARN("GlslangToSpv log:\n{}", spvLogMessage);
    }

    return spirvBytecode;
}

} // namespace snv::VulkanShaderCompiler
