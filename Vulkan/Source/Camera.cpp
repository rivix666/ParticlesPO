#include "stdafx.h"
#include "Camera.h"

const float CCamera::DEFAULT_MOVE_SPEED = 0.02f;
const glm::vec3 CCamera::WORLD_UP = glm::vec3(0.0f, 1.0f, 0.0f);

SCamMemento::SCamMemento(CCamera* cam)
{
    eye = cam->CameraPosition();
    view = cam->CameraView();
    yaw = cam->Yaw();
    pitch = cam->Pitch();
    sphereCamPSI = cam->SphereCamPSI();
    sphereCamFI = cam->SphereCamFI();
    sphereCamRadius = cam->SphereCamRadius();
}

CCamera::CCamera()
{
    // Init move keys states
    for (uint32_t i = 0; i < (uint32_t)ECamMoveDir::_COUNT_; i++)
        m_MoveDirKeyState[i] = false;

    // Init cam params
    RestoreDefaultCamParams();

    // Set up projection matrix
    SetPerspectiveProjection(FOV, ASPECT, Z_NEAR, Z_FAR);
}

void CCamera::Update()
{
    UpdateCameraVectors();

    if (m_UseFreeCam)
        m_ViewMtx = glm::lookAt(m_Eye, m_Eye + m_View, m_Up);
    else
        m_ViewMtx = glm::lookAt(m_Eye, m_View, m_Up);

    PassDataToUniBuffer();
}

void CCamera::SetPerspectiveProjection(float fov, float aspectRatio, float zNear, float zFar)
{
    fov = fov * (float)DEG_TO_RAD;
    m_ProjectionMtx = glm::perspective(fov, aspectRatio, zNear, zFar);
    m_InvProjectionMtx = glm::inverse(m_ProjectionMtx);
}

void CCamera::SetUseFreeCam(bool use /*= true*/)
{
    static SCamMemento memento;
    if (use)
    {
        RestoreCamParams(memento);
    }
    else
    {
        memento = SCamMemento(this);
        RestoreDefaultCamParams();
    }
    m_UseFreeCam = use;
}

void CCamera::ProcessMouseMoveInput(float xoffset, float yoffset)
{
    if (m_UseFreeCam)
        MoveYawPitchFreeCam(xoffset, yoffset);
    else
        MoveCamSpherical(0.0025f * yoffset, 0.0025f * xoffset);
}

void CCamera::MoveFreeCam(ECamMoveDir dir, bool state)
{
    m_MoveDirKeyState[(uint32_t)dir] = state;
}

void CCamera::MoveYawPitchFreeCam(float xoffset, float yoffset)
{
    xoffset *= 0.1f;
    yoffset *= 0.1f;

    m_Yaw += xoffset;
    m_Pitch += yoffset;

    if (m_Pitch > 89.0f)
        m_Pitch = 89.0f;
    if (m_Pitch < -89.0f)
        m_Pitch = -89.0f;
}

void CCamera::MoveCamSpherical(float psi, float fi)
{
    // Vertical move
    m_SphereCamPSI -= psi;
    if (m_SphereCamPSI < -1.5f)
        m_SphereCamPSI = -1.5f;
    else if (m_SphereCamPSI > -0.1f)
        m_SphereCamPSI = -0.1f;

    // Horizonthal move
    m_SphereCamFI -= fi;
}

void CCamera::ChangeViewSphereRadius(float r)
{
    m_SphereCamRadius += r;
    if (m_SphereCamRadius > 0)
        m_SphereCamRadius = 0;
}

void CCamera::RestoreCamParams(const SCamMemento& memento)
{
    m_Eye = memento.eye;
    m_View = memento.view;
    m_Yaw = memento.yaw;
    m_Pitch = memento.pitch;
    m_SphereCamPSI = memento.sphereCamPSI;
    m_SphereCamFI = memento.sphereCamFI;
    m_SphereCamRadius = memento.sphereCamRadius;
}

void CCamera::RestoreDefaultCamParams()
{
    RestoreCamParams(SCamMemento());
    Update();
}

void CCamera::UpdateCameraVectors()
{
    if (m_UseFreeCam)
        UpdateFreeCamVectors();
    else
        UpdateCamSphericalVectors();
}

void CCamera::UpdateCamSphericalVectors()
{
    m_View = glm::vec3(0.0f, 0.0f, 0.0f);
    m_Eye.x = m_View.x + m_SphereCamRadius * cos(m_SphereCamPSI) * cos(m_SphereCamFI);
    m_Eye.y = m_View.y + m_SphereCamRadius * sin(m_SphereCamPSI);
    m_Eye.z = m_View.z + m_SphereCamRadius * cos(m_SphereCamPSI) * sin(m_SphereCamFI);
}

void CCamera::UpdateFreeCamVectors()
{
    UpdateFreeCamPos();
    UpdateFreeCamView();

    // Update right and up
    m_Right = glm::normalize(glm::cross(m_View, WORLD_UP));
    m_Up = glm::normalize(glm::cross(m_Right, m_View));
}

void CCamera::UpdateFreeCamPos()
{
    float velocity = m_MoveSpeed * g_Engine->LastFrameTime();
    if (m_MoveDirKeyState[(uint32_t)ECamMoveDir::FORWARD])
        m_Eye += m_View * velocity;
    if (m_MoveDirKeyState[(uint32_t)ECamMoveDir::BACKWARD])
        m_Eye -= m_View * velocity;
    if (m_MoveDirKeyState[(uint32_t)ECamMoveDir::RIGHT])
        m_Eye += m_Right * velocity;
    if (m_MoveDirKeyState[(uint32_t)ECamMoveDir::LEFT])
        m_Eye -= m_Right * velocity;
    if (m_MoveDirKeyState[(uint32_t)ECamMoveDir::UP])
        m_Eye += m_Up * velocity;
    if (m_MoveDirKeyState[(uint32_t)ECamMoveDir::DOWN])
        m_Eye -= m_Up * velocity;
}

void CCamera::UpdateFreeCamView()
{
    glm::vec3 front;
    front.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
    front.y = sin(glm::radians(m_Pitch));
    front.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
    m_View = glm::normalize(front);
}

void CCamera::PassDataToUniBuffer()
{
    SCamUniBuffer ub = {};
    ub.view = ViewMatrix();
    ub.proj = ProjectionMatrix();
    ub.proj[1][1] *= -1;

    void* data;
    vkMapMemory(g_Engine->Device(), g_Engine->Renderer()->CamUniBufferMemory(), 0, sizeof(ub), 0, &data);
    memcpy(data, &ub, sizeof(ub));
    vkUnmapMemory(g_Engine->Device(), g_Engine->Renderer()->CamUniBufferMemory());
}
