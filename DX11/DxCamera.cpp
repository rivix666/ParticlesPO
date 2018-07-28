#include "stdafx.h"
#include "DxCamera.h"

CDxCamera::CDxCamera() 
    : m_Eye(XMFLOAT3(0.f, 2.f, -4.f))
    , m_Up(XMFLOAT3(0.f, 1.f, 0.f))
    , m_View(XMFLOAT3(0.f,0.f,0.f))
    , sphereCamPSI(-0.5f)
    , sphereCamFI(0.f)
    , sphereCamRadius(-10.f)
{
    XMStoreFloat4x4(&m_ViewMtx, XMMatrixIdentity());
    XMStoreFloat4x4(&m_ProjectionMtx, XMMatrixIdentity());
    XMStoreFloat4x4(&m_InvProjectionMtx, XMMatrixIdentity());
}

CDxCamera::~CDxCamera()
{
}

void CDxCamera::SetPerspectiveProjection(float FOV, float aspectRatio, float zNear, float zFar)
{
    FOV = FOV * (float)DEG_TO_RAD;
    XMStoreFloat4x4(&m_ProjectionMtx, XMMatrixPerspectiveFovLH(FOV, aspectRatio, zNear, zFar));
    XMStoreFloat4x4(&m_InvProjectionMtx, XMMatrixInverse(&XMVECTOR(), XMLoadFloat4x4(&ProjectionMatrix()))); //#DX11 wywalic statica (patrz cast na dole) 
}

void CDxCamera::Update()
{
    MoveCamSpherical();
    XMStoreFloat4x4(&m_ViewMtx, XMMatrixLookAtLH(XMLoadFloat3(&m_Eye), XMLoadFloat3(&m_View), XMLoadFloat3(&m_Up))); //#DX11 przerobic na xmvector z xmfloat
}


void CDxCamera::MoveCamSpherical(float psi, float fi)
{
    // vertical move
    sphereCamPSI -= psi;
    if (sphereCamPSI < -1.5f) 
        sphereCamPSI = -1.5f;
    else if (sphereCamPSI > -0.1f) 
        sphereCamPSI = -0.1f;

    // horizonthal move
    sphereCamFI -= fi;
}

void CDxCamera::ChangeViewSphereRadius(float r)
{
    sphereCamRadius += r;
    if (sphereCamRadius > 0) 
        sphereCamRadius = 0;
}

void CDxCamera::MoveCamSpherical()
{
    XMFLOAT3 temp = XMFLOAT3(0., 0., 0.); // view
    m_Eye.x = temp.x + sphereCamRadius * cos(sphereCamPSI) * cos(sphereCamFI);
    m_Eye.y = temp.y + sphereCamRadius * sin(sphereCamPSI);
    m_Eye.z = temp.z + sphereCamRadius * cos(sphereCamPSI) * sin(sphereCamFI);
}

void CDxCamera::AddToView(float x, float y, float z){
    m_View.x += x;
    m_View.y += y;
    m_View.z += z;
}

void CDxCamera::AddToEye(float x, float y, float z){
    m_Eye.x += x;
    m_Eye.y += y;
    m_Eye.z += z;
}

XMFLOAT4X4& CDxCamera::ViewMatrix()
{
    return m_ViewMtx;
}

XMFLOAT4X4& CDxCamera::ProjectionMatrix()
{
    return m_ProjectionMtx;
}

XMFLOAT4X4& CDxCamera::InvProjectionMatrix()
{
    return m_InvProjectionMtx;
}

XMFLOAT3& CDxCamera::CameraPosition()
{
    return m_Eye;
}

XMFLOAT3& CDxCamera::CameraView()
{
    return m_View;
}

XMFLOAT3& CDxCamera::CameraUp()
{
    return m_Up;
}
