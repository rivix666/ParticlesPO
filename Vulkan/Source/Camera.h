#pragma once

class CCamera;

struct SCamUniBuffer
{
    glm::mat4 view;
    glm::mat4 proj;
};

struct SCamMemento
{
    SCamMemento(CCamera* cam);
    SCamMemento() = default;
    ~SCamMemento() = default;

    // Camera Attributes
    glm::vec3 eye = glm::vec3(0.0f, 5.0f, 15.0f);
    glm::vec3 view = glm::vec3(0.0f, 0.0f, 0.0f);

    // Angles
    float yaw = -90.0f;
    float pitch = 0.0f;;

    // Cam sphere move attributes
    float sphereCamPSI = -0.5f;
    float sphereCamFI = 0.0f;
    float sphereCamRadius = -10.0f;;
};

enum class ECamMoveDir
{
    FORWARD = 0,
    BACKWARD,
    RIGHT,
    LEFT,
    UP,
    DOWN,

    _COUNT_
};

class CCamera
{
public:
    CCamera();
    ~CCamera() = default;

    typedef unsigned int uint; // #TYPEDEF_UINT czemu nei bierze z stdafx??

    // View / Position
    void Update();

    // Projection methods
    void SetPerspectiveProjection(float fov, float aspectRatio, float zNear, float zFar);

    // Setters & Getters
    void SetEye(const glm::vec3& eye) { m_Eye = eye; }
    void SetView(const glm::vec3& view) { m_View = view; }

    float MoveSpeed() const { return m_MoveSpeed; }
    bool UseFreeCam() const { return m_UseFreeCam; }
    void SetUseFreeCam(bool use = true);
    void SetMoveSpeed(float speed) { m_MoveSpeed = speed; }

    // Matrices
    const glm::mat4& ViewMatrix() const { return m_ViewMtx; }
    const glm::mat4& ProjectionMatrix() const { return m_ProjectionMtx; }
    const glm::mat4& InvProjectionMatrix() const { return m_InvProjectionMtx; }

    // Vectors
    const glm::vec3& CameraPosition() const { return m_Eye; }
    const glm::vec3& CameraRight() const { return m_Right; }
    const glm::vec3& CameraView() const { return m_View; }
    const glm::vec3& CameraUp() const { return m_Up; }

    // Angles
    float Yaw() const { return m_Yaw; }
    float Pitch() const { return m_Pitch; }

    // Sphere attributes
    float SphereCamPSI() const { return m_SphereCamPSI; }
    float SphereCamFI() const { return m_SphereCamFI; }
    float SphereCamRadius() const { return m_SphereCamRadius; }

    // Input
    void ProcessMouseMoveInput(float xoffset, float yoffset);

    // Free camera move
    void MoveFreeCam(ECamMoveDir dir, bool state);
    void MoveYawPitchFreeCam(float xoffset, float yoffset);

    // Sphere camera move
    void MoveCamSpherical(float psi, float fi);
    void ChangeViewSphereRadius(float r);

    // Misc
    void RestoreCamParams(const SCamMemento& memento);
    void RestoreDefaultCamParams();

    static const float DEFAULT_MOVE_SPEED;
    static const glm::vec3 WORLD_UP;

private:
    // Move
    void UpdateCameraVectors();
    void UpdateCamSphericalVectors();
    void UpdateFreeCamVectors();
    void UpdateFreeCamPos();
    void UpdateFreeCamView();

    // Uniform buffer
    void PassDataToUniBuffer();

    // Matrices
    glm::mat4 m_ViewMtx = glm::mat4(1.0f);
    glm::mat4 m_ProjectionMtx = glm::mat4(1.0f);
    glm::mat4 m_InvProjectionMtx = glm::mat4(1.0f);

    // Camera Attributes
    glm::vec3 m_Eye;
    glm::vec3 m_View;
    glm::vec3 m_Up;
    glm::vec3 m_Right;

    // Move
    float m_MoveSpeed = DEFAULT_MOVE_SPEED;

    // Angles
    float m_Yaw;
    float m_Pitch;

    // Cam sphere move attributes
    float m_SphereCamPSI;
    float m_SphereCamFI;
    float m_SphereCamRadius;

    // Misc
    bool m_UseFreeCam = true;
    bool m_MoveDirKeyState[(uint)ECamMoveDir::_COUNT_];
};