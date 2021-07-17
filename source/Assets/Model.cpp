#include <Assets/Model.hpp>
#include <Core/Log.hpp>
#include <Renderer/Renderer.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>


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


namespace snv
{

Model::Model(Model&& other) noexcept
    : m_meshes(std::move(other.m_meshes))
{}

Model& Model::operator=(Model&& other) noexcept
{
    m_meshes = std::move(other.m_meshes);

    return *this;
}


Model Model::LoadAsset(const char* assetPath)
{
    Assimp::Importer assimpImporter;
    // TODO(v.matushkin): Learn more about aiPostProcessSteps
    const aiScene* scene = assimpImporter.ReadFile(
        assetPath,
        aiPostProcessSteps::aiProcess_Triangulate
        | aiPostProcessSteps::aiProcess_GenNormals
    );

    Model model;

    if (scene == nullptr)
    {
        LOG_ERROR(
            "Got an error while loading Mesh"
            "\t\nPath: {0}"
            "\t\nAssimp error message: {1}",
            assetPath, assimpImporter.GetErrorString()
        );
        return model;
    }

    auto& meshes = model.m_meshes;

    for (ui32 i = 0; i < scene->mNumMeshes; ++i)
    {
        auto mesh = scene->mMeshes[i];
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

        if (mesh->HasPositions())
        {
            VertexAttributeDescriptor vertexAttributeDescriptor;
            vertexAttributeDescriptor.Attribute = VertexAttribute::Position;
            vertexAttributeDescriptor.Dimension = AssimpConstants::PositionDimension;
            vertexAttributeDescriptor.Format    = VertexAttributeFormat::Float32;
            vertexAttributeDescriptor.Offset    = vertexBufferSize;
            vertexLayout.push_back(vertexAttributeDescriptor);

            vertexBufferSize += numVertices * AssimpConstants::PositionSize;
        }
        if (mesh->HasNormals())
        {
            VertexAttributeDescriptor vertexAttributeDescriptor;
            vertexAttributeDescriptor.Attribute = VertexAttribute::Normal;
            vertexAttributeDescriptor.Dimension = AssimpConstants::NormalDimension;
            vertexAttributeDescriptor.Format    = VertexAttributeFormat::Float32;
            vertexAttributeDescriptor.Offset    = vertexBufferSize;
            vertexLayout.push_back(vertexAttributeDescriptor);

            vertexBufferSize += numVertices * AssimpConstants::NormalSize;
        }

        // TODO: What type should I use?
        auto vertexData = std::make_unique<i8[]>(vertexBufferSize);
        auto vertexDataPtr = vertexData.get();

        if (mesh->HasPositions())
        {
            const auto bytesToCopy = numVertices * AssimpConstants::PositionSize;
            std::memcpy(vertexDataPtr, mesh->mVertices, bytesToCopy);
            vertexDataPtr += bytesToCopy;
        }
        if (mesh->HasNormals())
        {
            const auto bytesToCopy = numVertices * AssimpConstants::NormalSize;
            std::memcpy(vertexDataPtr, mesh->mNormals, bytesToCopy);
            vertexDataPtr += bytesToCopy;
        }

        meshes.emplace_back(indexCount, std::move(indexData), numVertices, std::move(vertexData), vertexLayout);
    }

    return model;
}

} // namespace snv
