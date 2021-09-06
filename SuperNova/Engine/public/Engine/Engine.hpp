#pragma once

#include <Engine/Application/IApplicationLayer.hpp>

#include <Engine/Assets/Model.hpp>
#include <Engine/Entity/GameObject.hpp>

#include <memory>


namespace snv
{

class Engine final : public IApplicationLayer
{
public:
    Engine() = default;

private:
    void OnCreate()  override;
    void OnDestroy() override;
    void OnUpdate()  override;

private:
    std::shared_ptr<Model> m_sponzaModel;
    GameObject             m_sponzaGO;
    GameObject             m_camera;
};

} // namespace snv
