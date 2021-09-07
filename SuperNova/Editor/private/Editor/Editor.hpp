#pragma once

#include <Engine/Application/IApplicationLayer.hpp>


namespace snv
{

class Editor final : public IApplicationLayer
{
public:
    Editor() = default;

private:
    void OnCreate()  override;
    void OnDestroy() override;
    void OnUpdate()  override;
};

} // namespace snv
