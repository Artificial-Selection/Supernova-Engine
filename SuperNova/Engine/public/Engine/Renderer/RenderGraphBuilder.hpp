#pragma once

#include <Engine/Core/Core.hpp>
#include <Engine/Renderer/IRenderPass.hpp>

#include <memory>
#include <string>
//#include <string_view>
#include <vector>
//#include <unordered_map>


namespace snv
{

class RenderGraph;


class RenderGraphBuilder
{
    // struct string_hash
    // {
    //     using hash_type = std::hash<std::string_view>;
    //     using is_transparent = void;
    // 
    //     ui64 operator()(const char* str)        const { return hash_type{}(str); }
    //     ui64 operator()(std::string_view str)   const { return hash_type{}(str); }
    //     ui64 operator()(const std::string& str) const { return hash_type{}(str); }
    // };


    // struct RenderPassInfo
    // {
    //     std::unique_ptr<IRenderPass> RenderPass;
    //     std::vector<std::string>     ReadAttachments;
    //     std::vector<std::string>     WriteAttachments;
    // };

public:
    template<CRenderPass T>
    void AddRenderPass()
    {
        m_renderPasses.push_back(std::make_unique<T>());
    }

    RenderGraph* Build();

private:
    std::vector<std::unique_ptr<IRenderPass>> m_renderPasses;

    // std::unordered_map<std::string, RenderTexture, string_hash, std::equal_to<>> m_renderTextures;
};

} // namespace snv
