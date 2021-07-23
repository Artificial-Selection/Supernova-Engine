#include <Components/CameraController.hpp>
#include <Components/Transform.hpp>
#include <Entity/GameObject.hpp>
#include <Input/Cursor.hpp>
#include <Input/Keyboard.hpp>
#include <Input/Mouse.hpp>
#include <Utils/Time.hpp>

#include <glm/ext/vector_float2.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>


constexpr f32 k_PositionStep  = 1.0f;
constexpr f32 k_PositionBoost = 10.0f;


namespace snv
{

glm::vec3 GetInputTranslation();



CameraController::CameraController()
{
    //Input::Keyboard::SetKeyEventListener(
    //    [this](Input::KeyEvent keyEvent)
    //    {
    //        //LOG_INFO("CameraController::OnKeyEvent key: {} | action: {}", keyEvent.Key, keyEvent.Action);
    //        OnKeyEvent(keyEvent);
    //    }
    //);
}


void CameraController::OnUpdate()
{
    auto& cameraTransform = m_gameObject->GetComponent<Transform>();

    if (m_init == false)
    {
        auto rotation = cameraTransform.GetRotationEuler();
        m_pitch = rotation.x;
        m_yaw = rotation.y;

        m_init = true;
    }


    ProcessMouseInput();

    auto rotation = glm::angleAxis(m_pitch, glm::vec3(1, 0, 0));
    rotation *= glm::angleAxis(m_yaw, glm::vec3(0, 1, 0));
    //rotation = glm::normalize(rotation);

    auto tranlation = GetInputTranslation() * rotation;

    cameraTransform.SetRotation(rotation);
    cameraTransform.Translate(tranlation);
}


void CameraController::ProcessMouseInput()
{
    const auto currentMousePosition = Input::Mouse::GetMousePosition();
    const auto currentMouseX = currentMousePosition.x;
    const auto currentMouseY = currentMousePosition.y;

    if (Input::Mouse::IsButtonPressed(Input::MouseButton::Right))
    {
        snv::Input::Cursor::SetCursorMode(snv::Input::CursorMode::Locked);

        if (currentMouseX != m_mousePositionX || currentMouseY != m_mousePositionY)
        {
            const auto mouseDeltaX = currentMouseX - m_mousePositionX;
            const auto mouseDeltaY = currentMouseY - m_mousePositionY;

            const auto deltaTime = Time::GetDelta();

            m_pitch += mouseDeltaY * deltaTime;
            m_yaw += mouseDeltaX * deltaTime;

            /*if (m_pitch > glm::radians(360.0f)) m_pitch -= glm::radians(360.0f);
            else if (m_pitch < 0) m_pitch += glm::radians(360.0f);
            if (m_yaw > glm::radians(360.0f)) m_yaw -= glm::radians(360.0f);
            else if (m_yaw < 0) m_yaw += glm::radians(360.0f);*/
        }
    }
    else
    {
        snv::Input::Cursor::SetCursorMode(snv::Input::CursorMode::Normal);
    }

    m_mousePositionX = currentMouseX;
    m_mousePositionY = currentMouseY;
}


glm::vec3 GetInputTranslation()
{
    glm::vec3 direction(0.0f);

    if (Input::Keyboard::IsKeyPressed(Input::KeyboardKey::W))
    {
        direction.z += k_PositionStep;
    }
    if (Input::Keyboard::IsKeyPressed(Input::KeyboardKey::S))
    {
        direction.z -= k_PositionStep;
    }
    if (Input::Keyboard::IsKeyPressed(Input::KeyboardKey::A))
    {
        direction.x += k_PositionStep;
    }
    if (Input::Keyboard::IsKeyPressed(Input::KeyboardKey::D))
    {
        direction.x -= k_PositionStep;
    }
    if (Input::Keyboard::IsKeyPressed(Input::KeyboardKey::Q))
    {
        direction.y += k_PositionStep;
    }
    if (Input::Keyboard::IsKeyPressed(Input::KeyboardKey::E))
    {
        direction.y -= k_PositionStep;
    }

    if (Input::Keyboard::IsKeyPressed(Input::KeyboardKey::LeftShift))
    {
        direction *= k_PositionBoost;
    }

    direction *= static_cast<f32>(Time::GetDelta());

    return direction;
}

} // namespace snv
