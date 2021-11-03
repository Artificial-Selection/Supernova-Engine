#include <Engine/Renderer/RenderGraphBuilder.hpp>
#include <Engine/Renderer/RenderGraph.hpp>


namespace snv
{

RenderGraph* RenderGraphBuilder::Build()
{
    // TODO(v.matushkin): Some complicated render graph building code.
    //  For now just add every pass.
    std::vector<IRenderPass*> graphRenderPasses;

    for (auto& renderPass : m_renderPasses)
    {
        graphRenderPasses.push_back(renderPass.get());
    }

    return new RenderGraph(std::move(graphRenderPasses));
}


// void RenderGraphBuilder::ReadTexture(std::string_view textureName)
// {
//     if (auto it = m_renderTextures.find(textureName); it != m_renderTextures.end())
//     {
//         it->second.ReadCount++;
//     }
//     else
//     {
//         RenderTexture rt = {.ReadCount = 1};
//         m_renderTextures.emplace(textureName, rt);
//     }
// }
// 
// void RenderGraphBuilder::WriteTexture(std::string_view textureName)
// {
//     if (auto it = m_renderTextures.find(textureName); it != m_renderTextures.end())
//     {
//         it->second.WriteCount++;
//     }
//     else
//     {
//         RenderTexture rt = {.WriteCount = 1};
//         m_renderTextures.emplace(textureName, rt);
//     }
// }

} // namespace snv
