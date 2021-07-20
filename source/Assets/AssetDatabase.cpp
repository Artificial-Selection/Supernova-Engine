#include <Assets/AssetDatabase.hpp>
#include <Core/Assert.hpp>
#include <Assets/Model.hpp>
#include <Assets/Texture.hpp>

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

AssetDatabase::Models   AssetDatabase::m_models;
AssetDatabase::Textures AssetDatabase::m_textures;


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
        assetIt = m_textures.emplace(assetPath, std::make_shared<Texture>(LoadTexture(assetPath.c_str()))).first;
    }

    return assetIt->second;
}

// TODO(v.matushkin): There should be a default material with default textures
//   Because rn I don't load meshes with materials without diffuse textures

Model AssetDatabase::LoadModel(const char* assetPath)
{
    Assimp::Importer assimpImporter;
    // TODO(v.matushkin): Learn more about aiPostProcessSteps
    const aiScene* scene = assimpImporter.ReadFile(
        assetPath,
        aiPostProcessSteps::aiProcess_Triangulate
        | aiPostProcessSteps::aiProcess_GenNormals
        // | aiPostProcessSteps::aiProcess_FlipUVs Instead of stbi_set_flip_vertically_on_load(true); ?
    );

    if (scene == nullptr)
    {
        LOG_ERROR(
            "Got an error while loading Mesh"
            "\t\nPath: {0}"
            "\t\nAssimp error message: {1}",
            assetPath, assimpImporter.GetErrorString()
        );
        SNV_ASSERT(false, "REMOVE THIS SOMEHOW");
    }

    std::vector<std::pair<Mesh, Material>> meshes;

    for (ui32 i = 0; i < scene->mNumMeshes; ++i)
    {
        const auto mesh = scene->mMeshes[i];

        SNV_ASSERT(mesh->HasFaces(), "LOL");
        SNV_ASSERT(mesh->HasPositions(), "LOL");
        SNV_ASSERT(mesh->HasNormals(), "LOL");

        const auto numVertices = mesh->mNumVertices;
        const auto numFaces    = mesh->mNumFaces;
        // TODO(v.matushkin): Makes assumption that we have 3 indices per face, which should be fine with aiProcess_Triangulate
        //  but seems like there is some shit with lines and points
        const auto indexBufferSize = numFaces * 3;
        auto indexData = std::make_unique<ui32[]>(indexBufferSize);
        auto indexDataPtr = indexData.get();

        i32 indexCount = 0;
        for (ui32 i = 0; i < numFaces; ++i)
        {
            const auto& face = mesh->mFaces[i];
            for (ui32 j = 0; j < face.mNumIndices; ++j)
            {
                indexDataPtr[indexCount++] = face.mIndices[j];
            }
        }

        std::vector<VertexAttributeDescriptor> vertexLayout;
        ui32 vertexBufferSize = 0;

        // Vertex Positions Layout
        {
            VertexAttributeDescriptor positionAttribute{
                .Attribute = VertexAttribute::Position,
                .Format    = VertexAttributeFormat::Float32,
                .Dimension = AssimpConstants::PositionDimension,
                .Offset    = vertexBufferSize
            };
            vertexLayout.push_back(positionAttribute);

            vertexBufferSize += numVertices * AssimpConstants::PositionSize;
        }
        // Vertex Normals Layout
        {
            VertexAttributeDescriptor normalAttribute{
                .Attribute = VertexAttribute::Normal,
                .Format    = VertexAttributeFormat::Float32,
                .Dimension = AssimpConstants::NormalDimension,
                .Offset    = vertexBufferSize
            };
            vertexLayout.push_back(normalAttribute);

            vertexBufferSize += numVertices * AssimpConstants::NormalSize;
        }
        // Vertex TexCoord0 Layout
        // TODO(v.matushkin): Texture coords copying should be reworked
        //   There is no need for Float32 uv(as far as I know)
        if (mesh->HasTextureCoords(0))
        {
            VertexAttributeDescriptor texCoord0Attribute{
                .Attribute = VertexAttribute::TexCoord0,
                .Format    = VertexAttributeFormat::Float32,
                .Dimension = AssimpConstants::TexCoord0Dimension,
                .Offset    = vertexBufferSize
            };
            vertexLayout.push_back(texCoord0Attribute);

            vertexBufferSize += numVertices * AssimpConstants::TexCoord0Size;
        }

        // TODO: What type should I use?
        auto vertexData = std::make_unique<i8[]>(vertexBufferSize);
        auto vertexDataPtr = vertexData.get();

        // Get Vertex Positions
        {
            const auto bytesToCopy = numVertices * AssimpConstants::PositionSize;
            std::memcpy(vertexDataPtr, mesh->mVertices, bytesToCopy);
            vertexDataPtr += bytesToCopy;
        }
        // Get Vertex Normals
        {
            const auto bytesToCopy = numVertices * AssimpConstants::NormalSize;
            std::memcpy(vertexDataPtr, mesh->mNormals, bytesToCopy);
            vertexDataPtr += bytesToCopy;
        }
        // Get Vertex TexCoord0
        if (mesh->HasTextureCoords(0))
        {
            const auto bytesToCopy = numVertices * AssimpConstants::TexCoord0Size;
            std::memcpy(vertexDataPtr, mesh->mTextureCoords[0], bytesToCopy);
            vertexDataPtr += bytesToCopy;
        }

        const auto assimpMaterial = scene->mMaterials[mesh->mMaterialIndex];
        const auto diffuseTexturesCount = assimpMaterial->GetTextureCount(aiTextureType::aiTextureType_DIFFUSE);
        if (diffuseTexturesCount != 1)
        {
            LOG_WARN("Skipping loading of mesh: {}, with diffuseTexturesCount={}", mesh->mName.C_Str(), diffuseTexturesCount);
            continue;
        }

        Material material(assimpMaterial->GetName().C_Str());
        // Get Material BaseColorMap
        {
            const auto texturePath = GetAssimpMaterialTexturePath(assimpMaterial, aiTextureType::aiTextureType_DIFFUSE);
            material.SetBaseColorMap(AssetDatabase::LoadAsset<Texture>(texturePath.C_Str()));
        }
        // Get Material NormalMap
        if(assimpMaterial->GetTextureCount(aiTextureType::aiTextureType_NORMALS) > 0)
        {
            const auto texturePath = GetAssimpMaterialTexturePath(assimpMaterial, aiTextureType::aiTextureType_NORMALS);
            material.SetNormalMap(AssetDatabase::LoadAsset<Texture>(texturePath.C_Str()));
        }

        meshes.emplace_back(
            std::piecewise_construct,
            std::forward_as_tuple(indexCount, std::move(indexData), numVertices, std::move(vertexData), vertexLayout),
            std::forward_as_tuple(std::move(material))
        );
    }

    return Model(std::move(meshes));
}

Texture AssetDatabase::LoadTexture(const char* texturePath)
{
    stbi_set_flip_vertically_on_load(true); // TODO(v.matushkin): Set only once

    // TODO(v.matushkin): Asset class shouldn't handle path adjusting
    std::string fullPath = "../../assets/models/Sponza/" + std::string(texturePath);

    i32 width, height, numComponents;
    ui8* stbImageData = stbi_load(fullPath.c_str(), &width, &height, &numComponents, 0);
    if (stbImageData == nullptr)
    {
        SNV_ASSERT(false, stbi_failure_reason());
    }
    // TODO(v.matushkin): TextureGraphicsFormat selection need to be more robust
    if (numComponents != 3 && numComponents != 4)
    {
        LOG_WARN("Texture path: {}, numComponents: {}", texturePath, numComponents);
    }

    // NOTE(v.matushkin): Not sure about this dances with memory
    const auto textureSize = width * height * numComponents;
    auto textureData = std::make_unique<ui8[]>(textureSize);
    std::memcpy(textureData.get(), stbImageData, textureSize);
    stbi_image_free(stbImageData);
   
    // TODO(v.matushkin): Load Sponza textures as R8G8B8A8_SRGB ?
    // NOTE(v.matushkin): TextureWrapMode::Repeat by default?
    TextureDescriptor textureDescriptor{
        .Width          = width,
        .Height         = height,
        .GraphicsFormat = numComponents == 3 ? TextureGraphicsFormat::RGB8 : TextureGraphicsFormat::RGBA8,
        .WrapMode       = TextureWrapMode::Repeat
    };

    return Texture(textureDescriptor, std::move(textureData));
}

} // namespace snv
