#pragma once

class ICamera
{
public:
    // Projection methods
    virtual void SetPerspectiveProjection(float FOV, float aspectRatio, float zNear, float zFar) = 0;

    // View / Position
    virtual void Update() = 0;
    virtual void AddToView(float x, float y, float z) = 0;
    virtual void AddToEye(float x, float y, float z) = 0;
    virtual void MoveCamSpherical(float psi, float fi) = 0;
    virtual void ChangeViewSphereRadius(float r) = 0;
};