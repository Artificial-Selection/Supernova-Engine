#include <Engine/Assets/AssetDatabase.hpp>

#include <Engine/Assets/Model.hpp>
#include <Engine/Assets/Mesh.hpp>
#include <Engine/Assets/Material.hpp>
#include <Engine/Assets/Texture.hpp>
#include <Engine/Assets/Shader.hpp>
#include <Engine/Components/MeshRenderer.hpp>
#include <Engine/Core/Assert.hpp>
#include <Engine/Entity/GameObject.hpp>
#include <Engine/Renderer/Renderer.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// NOTE(v.matushkin): Put stb in a normal conan package? not this fucking trash that I get rn
#ifdef SNV_ENABLE_DEBUG
    #define STBI_FAILURE_USERMSG
#else
    #define STBI_NO_FAILURE_STRINGS
#endif
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <filesystem>
#include <fstream>


namespace AssimpConstants
{
    // Index
    static constexpr i32 IndexSize = sizeof(ui32);
    // Position
    static constexpr i32 PositionDimension = 3;
    static constexpr i32 PositionTypeSize = sizeof(f32);
    static constexpr i32 PositionSize = PositionDimension * PositionTypeSize;
    // Normal
    static constexpr i32 NormalDimension = 3;
    static constexpr i32 NormalTypeSize = sizeof(f32);
    static constexpr i32 NormalSize = NormalDimension * NormalTypeSize;
    // TexCoord0
    static constexpr i32 TexCoord0Dimension = 3;
    static constexpr i32 TexCoord0TypeSize = sizeof(f32);
    static constexpr i32 TexCoord0Size = TexCoord0Dimension * TexCoord0TypeSize;

#define MEMBER_SIZEOF(type, member) sizeof(((type *)0)->member)
    static_assert(MEMBER_SIZEOF(aiVector3D, x) == PositionTypeSize);
    static_assert(MEMBER_SIZEOF(aiVector3D, x) == NormalTypeSize);
#undef MEMBER_SIZEOF

#define ARRAY_MEMBER_SIZEOF(type, member) sizeof(((type *)0)->member[0])
    static_assert(ARRAY_MEMBER_SIZEOF(aiFace, mIndices)  == IndexSize);
    static_assert(ARRAY_MEMBER_SIZEOF(aiMesh, mVertices) == PositionSize);
    static_assert(ARRAY_MEMBER_SIZEOF(aiMesh, mNormals)  == NormalSize);
#undef ARRAY_MEMBER_SIZEOF
};


aiString GetAssimpMaterialTexturePath(const aiMaterial* material, aiTextureType textureType)
{
    aiString texturePath;
    auto error = material->GetTexture(aiTextureType::aiTextureType_DIFFUSE, 0, &texturePath);
    SNV_ASSERT(error == aiReturn::aiReturn_SUCCESS, error == aiReturn_FAILURE ? "aiReturn_FAILURE" : "aiReturn_OUTOFMEMORY");

    return texturePath;
}


namespace snv
{

std::vector<std::shared_ptr<Material>> GetAssimpMaterials(const aiScene* scene, ShaderPtr shader);


void AssetDatabase::Init(std::string assetDirectory)
{
    // NOTE(v.matushkin): Assuming that assetDirectory ends with '/'
    m_assetDir = std::move(assetDirectory);
    m_modelDir = m_assetDir + "models/";

    const std::string shaderDir = m_assetDir + "shaders/";

    m_glShaderDir = shaderDir + "gl/";
    m_vkShaderDir = shaderDir + "vk/";
    m_dxShaderDir = shaderDir + "dx/";
}


// TODO(v.matushkin): Heterogeneous lookup for string_view assetPath?
// TODO(v.matushkin): I think assets shouldn't have *::LoadAsset() method,
//   but coding asset importers is complicated, leave it like this for now
template<>
ModelPtr AssetDatabase::LoadAsset(const std::string& assetPath)
{
    auto assetIt = m_models.find(assetPath);
    if (assetIt == m_models.end())
    {
        assetIt = m_models.emplace(assetPath, std::make_shared<Model>(LoadModel(assetPath.c_str()))).first;
    }

    return assetIt->second;
}

template<>
TexturePtr AssetDatabase::LoadAsset(const std::string& assetPath)
{
    auto assetIt = m_textures.find(assetPath);
    if (assetIt == m_textures.end())
    {
        assetIt = m_textures.emplace(assetPath, std::make_shared<Texture>(LoadTexture(assetPath))).first;
    }

    return assetIt->second;
}

template<>
ShaderPtr AssetDatabase::LoadAsset(const std::string& assetPath)
{
    if (m_theOneAndOnlyForNow == nullptr)
    {
        m_theOneAndOnlyForNow = std::make_shared<Shader>(LoadShader(assetPath));
    }

    return m_theOneAndOnlyForNow;
}


Model AssetDatabase::LoadModel(const std::string& modelName)
{
    const auto modelPath = m_modelDir + modelName;

    Assimp::Importer assimpImporter;
    // TODO(v.matushkin): Learn more about aiPostProcessSteps
    const aiScene* scene = assimpImporter.ReadFile(
        modelPath,
        aiPostProcessSteps::aiProcess_Triangulate
        | aiPostProcessSteps::aiProcess_GenNormals
        // | aiPostProcessSteps::aiProcess_FlipUVs          // Instead of stbi_set_flip_vertically_on_load(true); ?
        // | aiPostProcessSteps::aiProcess_FlipWindingOrder // Default is counter clockwise
    );

    if (scene == nullptr)
    {
        LOG_ERROR(
            "Got an error while loading Mesh"
            "\t\nPath: {0}"
            "\t\nAssimp error message: {1}",
            modelPath, assimpImporter.GetErrorString());
        SNV_ASSERT(false, "REMOVE THIS SOMEHOW");
    }

    const auto materials = GetAssimpMaterials(scene, m_theOneAndOnlyForNow);

    std::vector<GameObject> modelGameObjects;

    for (ui32 i = 0; i < scene->mNumMeshes; ++i)
    {
        const auto assimpMesh = scene->mMeshes[i];

        SNV_ASSERT(assimpMesh->HasFaces(), "LOL");
        SNV_ASSERT(assimpMesh->HasPositions(), "LOL");
        SNV_ASSERT(assimpMesh->HasNormals(), "LOL");

        const auto numVertices     = assimpMesh->mNumVertices;
        const auto numFaces        = assimpMesh->mNumFaces;
        // TODO(v.matushkin): Makes assumption that we have 3 indices per face, which should be fine with aiProcess_Triangulate
        //  but seems like there is some shit with lines and points
        const auto indexBufferSize = numFaces * 3;
        auto       indexData       = std::make_unique<ui32[]>(indexBufferSize);
        auto       indexDataPtr    = indexData.get();

        i32 indexCount = 0;
        for (ui32 i = 0; i < numFaces; ++i)
        {
            const auto& face = assimpMesh->mFaces[i];
            for (ui32 j = 0; j < face.mNumIndices; ++j)
            {
                indexDataPtr[indexCount++] = face.mIndices[j];
            }
        }

        std::vector<VertexAttributeDesc> vertexLayout;
        ui32 vertexBufferSize = 0;

        // Vertex Positions Layout
        {
            VertexAttributeDesc positionAttributeDesc = {
                .Attribute = VertexAttribute::Position,
                .Format    = VertexAttributeFormat::Float32,
                .Dimension = AssimpConstants::PositionDimension,
                .Offset    = vertexBufferSize,
            };
            vertexLayout.push_back(positionAttributeDesc);

            vertexBufferSize += numVertices * AssimpConstants::PositionSize;
        }
        // Vertex Normals Layout
        {
            VertexAttributeDesc normalAttributeDesc = {
                .Attribute = VertexAttribute::Normal,
                .Format    = VertexAttributeFormat::Float32,
                .Dimension = AssimpConstants::NormalDimension,
                .Offset    = vertexBufferSize,
            };
            vertexLayout.push_back(normalAttributeDesc);

            vertexBufferSize += numVertices * AssimpConstants::NormalSize;
        }
        // Vertex TexCoord0 Layout
        // TODO(v.matushkin): Texture coords copying should be reworked
        //   There is no need for Float32 uv(as far as I know)
        if (assimpMesh->HasTextureCoords(0))
        {
            VertexAttributeDesc texCoord0AttributeDesc = {
                .Attribute = VertexAttribute::TexCoord0,
                .Format    = VertexAttributeFormat::Float32,
                .Dimension = AssimpConstants::TexCoord0Dimension,
                .Offset    = vertexBufferSize,
            };
            vertexLayout.push_back(texCoord0AttributeDesc);

            vertexBufferSize += numVertices * AssimpConstants::TexCoord0Size;
        }

        // TODO: What type should I use?
        auto vertexData    = std::make_unique<ui8[]>(vertexBufferSize);
        auto vertexDataPtr = vertexData.get();

        // Get Vertex Positions
        {
            const auto bytesToCopy = numVertices * AssimpConstants::PositionSize;
            std::memcpy(vertexDataPtr, assimpMesh->mVertices, bytesToCopy);
            vertexDataPtr += bytesToCopy;
        }
        // Get Vertex Normals
        {
            const auto bytesToCopy = numVertices * AssimpConstants::NormalSize;
            std::memcpy(vertexDataPtr, assimpMesh->mNormals, bytesToCopy);
            vertexDataPtr += bytesToCopy;
        }
        // Get Vertex TexCoord0
        if (assimpMesh->HasTextureCoords(0))
        {
            const auto bytesToCopy = numVertices * AssimpConstants::TexCoord0Size;
            std::memcpy(vertexDataPtr, assimpMesh->mTextureCoords[0], bytesToCopy);
            vertexDataPtr += bytesToCopy;
        }

        auto mesh = std::make_shared<Mesh>(indexCount, std::move(indexData), numVertices, std::move(vertexData), vertexLayout);

        GameObject gameObject;
        gameObject.AddComponent<MeshRenderer>(materials[assimpMesh->mMaterialIndex], mesh);

        modelGameObjects.emplace_back(gameObject);
    }

    return Model(std::move(modelGameObjects));
}

Texture AssetDatabase::LoadTexture(const std::string& texturePath)
{
    stbi_set_flip_vertically_on_load(true); // TODO(v.matushkin): Set only once

    // TODO(v.matushkin): Asset class shouldn't handle path adjusting
    std::string fullPath = "../../assets/models/Sponza/" + texturePath;

    i32 width, height, numComponents;
    // TODO(v.matushkin): Check for errors
    stbi_info(fullPath.c_str(), &width, &height, &numComponents);
    // TODO(v.matushkin): TextureGraphicsFormat selection need to be more robust
    // TODO(v.matushkin): Make SNV_ASSERT take formatting arguments, so here texturePath can be logged
    SNV_ASSERT(numComponents == 3 || numComponents == 4, "Right now only Textures with 3 or 4 channels are supported");
    const i32 desiredComponents = numComponents == 3 ? 4 : numComponents;

    ui8* stbImageData = stbi_load(fullPath.c_str(), &width, &height, &numComponents, desiredComponents);
    SNV_ASSERT(stbImageData != nullptr, stbi_failure_reason());

    // NOTE(v.matushkin): Not sure about this dances with memory
    const auto textureSize = width * height * desiredComponents;
    auto       textureData = std::make_unique<ui8[]>(textureSize);
    std::memcpy(textureData.get(), stbImageData, textureSize);
    stbi_image_free(stbImageData);

    // TODO(v.matushkin): Load Sponza textures as R8G8B8A8_SRGB ?
    // NOTE(v.matushkin): TextureWrapMode::Repeat by default?
    TextureDesc textureDesc = {
        .Width    = static_cast<ui32>(width),
        .Height   = static_cast<ui32>(height),
        .Format   = TextureFormat::RGBA8,
        .WrapMode = TextureWrapMode::Repeat
    };

    return Texture(textureDesc, std::move(textureData));
}

// TODO(v.matushkin): Improve this shit with passes/loading(don't know what did I mean by that)
//  Thats why there should be only one shader language I guess
//  Or at least there should some static AppSettings class or something, so there is no need to access Renderer
Shader AssetDatabase::LoadShader(const std::string& shaderName)
{
    const auto graphicsApi = Renderer::GetGraphicsApi();

    std::string shaderPath;
    std::string shaderExtension;
    if (graphicsApi == GraphicsApi::DirectX11 || graphicsApi == GraphicsApi::DirectX12)
    {
        shaderExtension = ".hlsl";
        shaderPath      = m_dxShaderDir;
    }
    else
    {
        shaderExtension = ".glsl";
        shaderPath      = graphicsApi == GraphicsApi::OpenGL ? m_glShaderDir : m_vkShaderDir;
    }
    shaderPath += shaderName;

    // Get Vertex Shader
    const std::string vertexPath(shaderPath + "_vs" + shaderExtension);
    const auto        vertexSize   = std::filesystem::file_size(vertexPath);
    auto              vertexSource = std::make_unique<char[]>(vertexSize + 1);
    {
        std::ifstream vertexFile(vertexPath, std::ios::binary | std::ios::in);
        vertexFile.read(vertexSource.get(), vertexSize);
    }
    // Get Fragment Shader
    const std::string fragmentPath(shaderPath + "_fs" + shaderExtension);
    const auto        fragmentSize   = std::filesystem::file_size(fragmentPath);
    auto              fragmentSource = std::make_unique<char[]>(fragmentSize + 1);
    {
        std::ifstream fragmentFile(fragmentPath, std::ios::binary | std::ios::in);
        fragmentFile.read(fragmentSource.get(), fragmentSize);
    }

    return Shader(
        std::span(vertexSource.get(), vertexSize),
        std::span(fragmentSource.get(), fragmentSize)
    );
}


std::vector<std::shared_ptr<Material>> GetAssimpMaterials(const aiScene* scene, ShaderPtr shader)
{
    SNV_ASSERT(scene->HasMaterials(), "LOL");

    std::vector<std::shared_ptr<Material>> materials;
    const auto numMaterials = scene->mNumMaterials;

    for (ui32 i = 0; i < numMaterials; ++i)
    {
        const auto assimpMaterial     = scene->mMaterials[i];
        const auto assimpMaterialName = assimpMaterial->GetName().C_Str();
        auto       material           = std::make_shared<Material>(shader);
        material->SetName(assimpMaterialName);

        // Get Material BaseColorMap
        const auto diffuseTexturesCount = assimpMaterial->GetTextureCount(aiTextureType::aiTextureType_DIFFUSE);
        if (diffuseTexturesCount > 0)
        {
            const auto texturePath = GetAssimpMaterialTexturePath(assimpMaterial, aiTextureType::aiTextureType_DIFFUSE);
            material->SetBaseColorMap(AssetDatabase::LoadAsset<Texture>(texturePath.C_Str()));

            if (diffuseTexturesCount != 1)
            {
                LOG_WARN("Material: {}, has {} BaseColor textures", assimpMaterialName, diffuseTexturesCount);
            }
        }
        else
        {
            LOG_WARN("Material: {}, has 0 baseColor textures, using default Black texture", assimpMaterialName);
            material->SetBaseColorMap(Texture::GetBlackTexture());
        }
        // Get Material NormalMap
        const auto normalTexturesCount = assimpMaterial->GetTextureCount(aiTextureType::aiTextureType_NORMALS);
        if (normalTexturesCount > 0)
        {
            const auto texturePath = GetAssimpMaterialTexturePath(assimpMaterial, aiTextureType::aiTextureType_NORMALS);
            material->SetNormalMap(AssetDatabase::LoadAsset<Texture>(texturePath.C_Str()));

            if (normalTexturesCount != 1)
            {
                LOG_WARN("Material: {}, has {} Normal textures", assimpMaterialName, diffuseTexturesCount);
            }
        }
        else
        {
            LOG_WARN("Material: {}, has 0 normal textures, using default Normal texture", assimpMaterialName);
            material->SetNormalMap(Texture::GetNormalTexture());
        }

        materials.emplace_back(material);
    }

    return materials;
}

} // namespace snv
