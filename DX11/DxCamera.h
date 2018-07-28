#pragma once

#include <d3d11.h> //#REDESIGN move to cpp
#include <d3dx11.h>
#include "ICamera.h"

class CDxCamera : public ICamera
{
public:
    CDxCamera();
    ~CDxCamera();

    // Projection methods
    void SetPerspectiveProjection(float FOV, float aspectRatio, float zNear, float zFar) override;

    // View / Position
    void Update() override;
    void MoveCamSpherical(float psi, float fi) override;
    void ChangeViewSphereRadius(float r) override;
    void AddToView(float x, float y, float z) override;
    void AddToEye(float x, float y, float z);

    // Setters & Getters
    XMFLOAT4X4& ViewMatrix();
    XMFLOAT4X4& ProjectionMatrix();
    XMFLOAT4X4& InvProjectionMatrix();
    XMFLOAT3& CameraPosition();
    XMFLOAT3& CameraView();
    XMFLOAT3& CameraUp();

private:
    // Move
    void MoveCamSpherical();

    XMFLOAT4X4 m_ViewMtx;
    XMFLOAT4X4 m_ProjectionMtx;
    XMFLOAT4X4 m_InvProjectionMtx;
    XMFLOAT3 m_Eye;
    XMFLOAT3 m_View;
    XMFLOAT3 m_Up;

    // Cam move attributes
    float sphereCamPSI, sphereCamFI, sphereCamRadius;
};

