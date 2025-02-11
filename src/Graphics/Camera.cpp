#include <Camera.hpp>

namespace dev
{

Camera::Camera()
{
    EventManager::Get().AddListener(Event::Type::WindowResize, this);
}

Camera::~Camera()
{
    EventManager::Get().RemoveListener(Event::Type::WindowResize, this);
}

void Camera::SetPerspective()
{
    projectionMatrix = glm::perspective(glm::radians(fov), aspect, near, far);
}

void Camera::SetOrthographic(float left, float right, float bottom, float top)
{
    projectionMatrix = glm::ortho(left, right, bottom, top, near, far);
}

void Camera::SetViewport(const LLGL::Extent2D& resolution)
{
    viewportSize = { resolution.width, resolution.height };
    aspect = (float)resolution.width / resolution.height;

    SetPerspective();
}

void Camera::SetFov(float fov)
{
    this->fov = fov;
    SetPerspective();
}

void Camera::SetNear(float near)
{
    this->near = near;
    SetPerspective();
}

void Camera::SetFar(float far)
{
    this->far = far;
    SetPerspective();
}

void Camera::SetFirstPerson(bool firstPerson)
{
    this->firstPerson = firstPerson;
}

void Camera::SetUp(const glm::vec3& up)
{
    this->up = up;
}

void Camera::SetLookAt(const glm::vec3& lookAtPos)
{
    this->lookAtPos = lookAtPos;
}

void Camera::SetViewMatrix(const glm::mat4& viewMatrix)
{
    this->viewMatrix = viewMatrix;
}

void Camera::OnEvent(Event& event)
{
    if(event.GetType() == Event::Type::WindowResize)
    {
        auto resizeEvent = dynamic_cast<WindowResizeEvent*>(&event);

        SetViewport(resizeEvent->GetSize());
    }
}

glm::vec3 Camera::GetUp() const
{
    return up;
}

glm::vec3 Camera::GetLookAt() const
{
    return lookAtPos;
}

glm::mat4 Camera::GetViewMatrix() const
{
    return viewMatrix;
}

glm::mat4 Camera::GetProjectionMatrix() const
{
    return projectionMatrix;
}

glm::vec2 Camera::GetViewport() const
{
    return viewportSize;
}

float Camera::GetFov() const
{
    return fov;
}

float Camera::GetNear() const
{
    return near;
}

float Camera::GetFar() const
{
    return far;
}

float Camera::GetAspect() const
{
    return aspect;
}

bool Camera::IsFirstPerson() const
{
    return firstPerson;
}

}
