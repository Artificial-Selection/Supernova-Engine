#pragma once

#include <Components/Component.hpp>
#include <Input/Keyboard.hpp>


namespace snv
{

//namespace Input
//{
//    struct KeyEvent;
//}


class CameraController final : public BaseComponent
{
public:
    CameraController();

    void OnKeyEvent(Input::KeyEvent keyEvent);
};

} // namespace snv
