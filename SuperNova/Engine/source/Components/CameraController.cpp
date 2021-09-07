#include <Engine/Components/CameraController.hpp>
#include <Engine/Components/Transform.hpp>
#include <Engine/Entity/GameObject.hpp>
#include <Engine/Input/Cursor.hpp>
#include <Engine/Input/Keyboard.hpp>
#include <Engine/Input/Mouse.hpp>
#include <Engine/Utils/Time.hpp>

#include <glm/ext/vector_float2.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>


namespace snv
{

glm::vec3 GetInputTranslation(f32 movementSpeed, f32 movementBoost);


CameraController::CameraController(GameObject* gameObject, f32 movementSpeed, f32 movementBoost)
    : BaseComponent(gameObject)
    , m_cameraTransform(&(m_gameObject->GetComponent<Transform>()))
    , m_movementSpeed(movementSpeed)
    , m_movementBoost(movementBoost)
{
    auto rotation = m_cameraTransform->GetRotationEuler();
    m_pitch = rotation.x;
    m_yaw = rotation.y;
}


void CameraController::SetMovementSpeed(f32 movementSpeed)
{
    m_movementSpeed = movementSpeed;
}

void CameraController::SetMovementBoost(f32 movementBoost)
{
    m_movementBoost = movementBoost;
}


void CameraController::OnUpdate()
{
    ProcessMouseInput();

    auto rotation = glm::angleAxis(m_pitch, glm::vec3(1, 0, 0));
    rotation *= glm::angleAxis(m_yaw, glm::vec3(0, 1, 0));
    //rotation = glm::normalize(rotation);

    auto tranlation = GetInputTranslation(m_movementSpeed, m_movementBoost) * rotation;

    m_cameraTransform->SetRotation(rotation);
    m_cameraTransform->Translate(tranlation);
}


void CameraController::ProcessMouseInput()
{
    const auto currentMousePosition = Input::Mouse::GetMousePosition();
    const auto currentMouseX = currentMousePosition.x;
    const auto currentMouseY = currentMousePosition.y;

    if (Input::Mouse::IsButtonPressed(Input::MouseButton::Right))
    {
        Input::Cursor::SetCursorMode(Input::CursorMode::Locked);

        if (currentMouseX != m_mousePositionX || currentMouseY != m_mousePositionY)
        {
            const auto mouseDeltaX = currentMouseX - m_mousePositionX;
            const auto mouseDeltaY = currentMouseY - m_mousePositionY;

            const auto deltaTime = Time::GetDelta();

            m_pitch += mouseDeltaY * deltaTime;
            m_yaw   += mouseDeltaX * deltaTime;

            /*if (m_pitch > glm::radians(360.0f)) m_pitch -= glm::radians(360.0f);
            else if (m_pitch < 0) m_pitch += glm::radians(360.0f);
            if (m_yaw > glm::radians(360.0f)) m_yaw -= glm::radians(360.0f);
            else if (m_yaw < 0) m_yaw += glm::radians(360.0f);*/
        }
    }
    else
    {
        Input::Cursor::SetCursorMode(Input::CursorMode::Normal);
    }

    m_mousePositionX = currentMouseX;
    m_mousePositionY = currentMouseY;
}


glm::vec3 GetInputTranslation(f32 movementSpeed, f32 movementBoost)
{
    glm::vec3 direction(0.0f);

    if (Input::Keyboard::IsKeyPressed(Input::KeyboardKey::W))
    {
        direction.z += movementSpeed;
    }
    if (Input::Keyboard::IsKeyPressed(Input::KeyboardKey::S))
    {
        direction.z -= movementSpeed;
    }
    if (Input::Keyboard::IsKeyPressed(Input::KeyboardKey::A))
    {
        direction.x += movementSpeed;
    }
    if (Input::Keyboard::IsKeyPressed(Input::KeyboardKey::D))
    {
        direction.x -= movementSpeed;
    }
    if (Input::Keyboard::IsKeyPressed(Input::KeyboardKey::Q))
    {
        direction.y += movementSpeed;
    }
    if (Input::Keyboard::IsKeyPressed(Input::KeyboardKey::E))
    {
        direction.y -= movementSpeed;
    }

    if (Input::Keyboard::IsKeyPressed(Input::KeyboardKey::LeftShift))
    {
        direction *= movementBoost;
    }

    direction *= static_cast<f32>(Time::GetDelta());

    return direction;
}

} // namespace snv
