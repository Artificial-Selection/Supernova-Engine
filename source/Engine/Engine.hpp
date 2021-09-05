#pragma once

#include <Application/IApplicationLayer.hpp>

#include <Assets/Model.hpp>
#include <Entity/GameObject.hpp>

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
