#include <Components/CameraController.hpp>
#include <Components/Transform.hpp>
#include <Entity/GameObject.hpp>

#include <Core/Log.hpp>


namespace snv
{

CameraController::CameraController()
{
    Input::Keyboard::SetKeyEventListener(
        [this](Input::KeyEvent keyEvent)
        {
            //LOG_INFO("CameraController::OnKeyEvent key: {} | action: {}", keyEvent.Key, keyEvent.Action);
            OnKeyEvent(keyEvent);
        }
    );
}


void CameraController::OnKeyEvent(Input::KeyEvent keyEvent)
{
    const auto [key, action] = keyEvent;
    /*LOG_INFO("CameraController::OnKeyEvent key: {} | action: {}", key, action);*/

    constexpr f32 step = 0.05f;

    if (action != Input::InputAction::Release)
    {
        auto& transform = m_gameObject->GetComponent<Transform>();

        switch (key)
        {
        case Input::KeyboardKey::W:
            transform.Translate(0.0f, 0.0f, step);
            break;
        }
    }
}

}
