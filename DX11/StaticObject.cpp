#include "stdafx.h"
#include "StaticObject.h"
#include "inputLayouts.h"
#include "DxRenderer.h"

//	RENDER METHODS
//-----------------------------------------------------------
void CGStaticObject::Render()
{
    prepareBuffersAndInputLayout();
    setEffectVariables();
    getTechAndDraw();
}

void CGStaticObject::prepareBuffersAndInputLayout()
{
    UINT stride = sizeof(SimpleVertex);
    UINT offset = 0;
    g_Engine->Renderer()->GetDeviceCtx()->IASetVertexBuffers(0, 1, &pVertexBuffer, &stride, &offset);
    g_Engine->Renderer()->GetDeviceCtx()->IASetIndexBuffer(pIndexBuffer, DXGI_FORMAT_R32_UINT, offset);
    g_Engine->Renderer()->GetDeviceCtx()->IASetInputLayout(g_pLayout);
}

void CGStaticObject::getTechAndDraw()
{
    D3DX11_TECHNIQUE_DESC techDesc;
    g_pTechnique->GetDesc(&techDesc);

    //send vertices down pipeline
    for (UINT p = 0; p < techDesc.Passes; p++)
    {
        g_pTechnique->GetPassByIndex(p)->Apply(0, g_Engine->Renderer()->GetDeviceCtx());
        g_Engine->Renderer()->GetDeviceCtx()->DrawIndexed(uiTotalIndices, 0, 0);
    }
}

//	INIT METHODS
//-----------------------------------------------------------
bool CGStaticObject::Init(StaticObjType obj)
{
    if (obj == StaticObjType::Floor)
    {
        if (!initFloorBuffers()) return releaseAll();
        initFloorVariables();
    }
    else
    {
        if (!initBoxBuffers()) return releaseAll();
        initBoxVariables();
    }
    if (!initInputLayoutAndTechnique(g_Engine->Renderer()->GetEffects())) return releaseAll();
    if (!initTextureSRV()) return releaseAll();
    initShaderVariables(g_Engine->Renderer()->GetEffects());
    setShaderVariables();
    initEffectVariables(g_Engine->Renderer()->GetEffects());

    return true;
}

bool CGStaticObject::initFloorBuffers()
{
    bool bSuccess = true;
    std::vector<SimpleVertex> vertices;
    std::vector<UINT> indices;

    vertices = initVerticesFloor();
    indices = initIndicesFloor();
    bSuccess &= initVertexBuffer(vertices);
    bSuccess &= initIndexBuffer(indices);

    return bSuccess;
}

bool CGStaticObject::initBoxBuffers()
{
    bool bSuccess = true;

    std::vector<SimpleVertex> vertices;
    std::vector<UINT> indices;

    vertices = initVerticesBox();
    indices = initIndicesBox();
    bSuccess &= initVertexBuffer(vertices);
    bSuccess &= initIndexBuffer(indices);

    return bSuccess;
}

void CGStaticObject::initBoxVariables()
{
    World = XMMatrixIdentity();
    World._43 = -6.0f;
    fTexMultiplier = 1.0f;
}

void CGStaticObject::initFloorVariables()
{
    World = XMMatrixIdentity();
    fTexMultiplier = 800.0f;
}

void CGStaticObject::initEffectVariables(ID3DX11Effect* pEffect)
{
    pObjectWorldMatrixEffectVariable = pEffect->GetVariableByName("StaticObjWorld")->AsMatrix();
    pObjectTexMulEffectVariable = pEffect->GetVariableByName("StaticObjectTexMul")->AsScalar();
}

void CGStaticObject::setEffectVariables()
{
    pObjectWorldMatrixEffectVariable->SetMatrix(reinterpret_cast<float*>(&World));
    pObjectTexMulEffectVariable->SetFloat(fTexMultiplier);
}

bool CGStaticObject::initInputLayoutAndTechnique(ID3DX11Effect* pEffect)
{
    inputLayouts* pInputLayouts = inputLayouts::getInstance(g_Engine->Renderer()->GetDevice(), pEffect);
    if (pInputLayouts == nullptr) return false;

    g_pTechnique = pInputLayouts->getTechnique(eInputLayouts::BASE);
    g_pLayout = pInputLayouts->getLayout(eInputLayouts::BASE);

    return true;
}

bool CGStaticObject::initTextureSRV()
{
    if (FAILED(D3DX11CreateShaderResourceViewFromFile(g_Engine->Renderer()->GetDevice(), L"Images/Others/ground.png", NULL, NULL, &pGroundTextureSRV, NULL))) 
        return utils::FatalError(L"Ground texture init FAIL");
    return true;
}

void CGStaticObject::initShaderVariables(ID3DX11Effect* pEffect)
{
    pGroundTextureSR = pEffect->GetVariableByName("groundTex")->AsShaderResource();
}

void CGStaticObject::setShaderVariables()
{
    pGroundTextureSR->SetResource(pGroundTextureSRV);
}

bool CGStaticObject::initVertexBuffer(std::vector<SimpleVertex> vertices)
{
    D3D11_BUFFER_DESC bd;
    bd.Usage = D3D11_USAGE_IMMUTABLE;
    bd.ByteWidth = sizeof(SimpleVertex) * vertices.size();
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = NULL;
    bd.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA vertexBufferData;
    vertexBufferData.pSysMem = &vertices[0];

    if (FAILED(g_Engine->Renderer()->GetDevice()->CreateBuffer(&bd, &vertexBufferData, &pVertexBuffer))) 
        return utils::FatalError(L"Could not create vertex buffer!");
    
    return true;
}

bool CGStaticObject::initIndexBuffer(std::vector<UINT> indices)
{
    D3D11_BUFFER_DESC bd;
    bd.Usage = D3D11_USAGE_IMMUTABLE;
    bd.ByteWidth = sizeof(UINT) * indices.size();
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.CPUAccessFlags = NULL;
    bd.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA indexBufferData;
    indexBufferData.pSysMem = &indices[0];

    if (FAILED(g_Engine->Renderer()->GetDevice()->CreateBuffer(&bd, &indexBufferData, &pIndexBuffer))) 
        return utils::FatalError(L"Could not create index buffer!");
    
    return true;
}

std::vector<SimpleVertex> CGStaticObject::initVerticesFloor()
{
    std::vector<SimpleVertex> vertices;
    vertices.push_back(SimpleVertex(XMFLOAT3(-1000, 0, -1000), XMFLOAT2(0, 0))); //front left 0
    vertices.push_back(SimpleVertex(XMFLOAT3(1000, 0, -1000), XMFLOAT2(1, 0))); //front right 1
    vertices.push_back(SimpleVertex(XMFLOAT3(-1000, 0, 1000), XMFLOAT2(0, 1))); //back left 2
    vertices.push_back(SimpleVertex(XMFLOAT3(1000, 0, 1000), XMFLOAT2(1, 1))); //back right 3
        
    return vertices;
}

std::vector<UINT> CGStaticObject::initIndicesFloor()
{

    std::vector<UINT> indices;
    indices.push_back(0);
    indices.push_back(1);
    indices.push_back(3);
    indices.push_back(0);
    indices.push_back(2);
    indices.push_back(3);

    uiTotalIndices = indices.size();

    return indices;
}

std::vector<SimpleVertex> CGStaticObject::initVerticesBox()
{

    std::vector<SimpleVertex> vertices;
    //DOWN
    vertices.push_back(SimpleVertex(XMFLOAT3(-1.0, 0, -1.0), XMFLOAT2(0, 0))); //front left 0
    vertices.push_back(SimpleVertex(XMFLOAT3(1.0, 0, -1.0), XMFLOAT2(1, 0))); //front right 1
    vertices.push_back(SimpleVertex(XMFLOAT3(-1.0, 0, 1.0), XMFLOAT2(0, 1))); //back left 2
    vertices.push_back(SimpleVertex(XMFLOAT3(1.0, 0, 1.0), XMFLOAT2(1, 1))); //back right 3

    //RIGHT
    vertices.push_back(SimpleVertex(XMFLOAT3(-1.0, 0, -1.0), XMFLOAT2(0, 0))); //front left 4
    vertices.push_back(SimpleVertex(XMFLOAT3(1.0, 0, -1.0), XMFLOAT2(1, 0))); //front right 5
    vertices.push_back(SimpleVertex(XMFLOAT3(-1.0, 2.0, -1.0), XMFLOAT2(0, 1))); //front  up left 6
    vertices.push_back(SimpleVertex(XMFLOAT3(1.0, 2.0, -1.0), XMFLOAT2(1, 1))); //front up right 7

    //UP
    vertices.push_back(SimpleVertex(XMFLOAT3(-1.0, 2.0, -1.0), XMFLOAT2(0, 0))); //front left 8
    vertices.push_back(SimpleVertex(XMFLOAT3(1.0, 2.0, -1.0), XMFLOAT2(1, 0))); //front right 9
    vertices.push_back(SimpleVertex(XMFLOAT3(-1.0, 2.0, 1.0), XMFLOAT2(0, 1))); //back left 10
    vertices.push_back(SimpleVertex(XMFLOAT3(1.0, 2.0, 1.0), XMFLOAT2(1, 1))); //back right 11

    //LEFT
    vertices.push_back(SimpleVertex(XMFLOAT3(-1.0, 0, 1.0), XMFLOAT2(0, 0))); //front left 12
    vertices.push_back(SimpleVertex(XMFLOAT3(1.0, 0, 1.0), XMFLOAT2(1, 0))); //front right 13
    vertices.push_back(SimpleVertex(XMFLOAT3(-1.0, 2.0, 1.0), XMFLOAT2(0, 1))); //front  up left 14
    vertices.push_back(SimpleVertex(XMFLOAT3(1.0, 2.0, 1.0), XMFLOAT2(1, 1))); //front up right 15

    //FRONT
    vertices.push_back(SimpleVertex(XMFLOAT3(-1.0, 0, -1.0), XMFLOAT2(0, 0))); //front left 16
    vertices.push_back(SimpleVertex(XMFLOAT3(-1.0, 0, 1.0), XMFLOAT2(1, 0))); //front right 17
    vertices.push_back(SimpleVertex(XMFLOAT3(-1.0, 2.0, -1.0), XMFLOAT2(0, 1))); //front  up left 18
    vertices.push_back(SimpleVertex(XMFLOAT3(-1.0, 2.0, 1.0), XMFLOAT2(1, 1))); //front up right 19

    //BACK
    vertices.push_back(SimpleVertex(XMFLOAT3(1.0, 0, -1.0), XMFLOAT2(0, 0))); //front left 20
    vertices.push_back(SimpleVertex(XMFLOAT3(1.0, 0, 1.0), XMFLOAT2(1, 0))); //front right 21
    vertices.push_back(SimpleVertex(XMFLOAT3(1.0, 2.0, -1.0), XMFLOAT2(0, 1))); //front  up left 22
    vertices.push_back(SimpleVertex(XMFLOAT3(1.0, 2.0, 1.0), XMFLOAT2(1, 1))); //front up right 23

    return vertices;
}

std::vector<UINT> CGStaticObject::initIndicesBox()
{

    std::vector<UINT> indices;

    //DOWN
    indices.push_back(0);
    indices.push_back(1);
    indices.push_back(3);
    indices.push_back(0);
    indices.push_back(2);
    indices.push_back(3);

    //RIGHT
    indices.push_back(4);
    indices.push_back(5);
    indices.push_back(7);
    indices.push_back(4);
    indices.push_back(6);
    indices.push_back(7);

    //UP
    indices.push_back(8);
    indices.push_back(9);
    indices.push_back(11);
    indices.push_back(8);
    indices.push_back(10);
    indices.push_back(11);

    //LEFT
    indices.push_back(12);
    indices.push_back(13);
    indices.push_back(15);
    indices.push_back(12);
    indices.push_back(14);
    indices.push_back(15);

    //FRONT
    indices.push_back(16);
    indices.push_back(17);
    indices.push_back(19);
    indices.push_back(16);
    indices.push_back(18);
    indices.push_back(19);

    //BACK
    indices.push_back(20);
    indices.push_back(21);
    indices.push_back(23);
    indices.push_back(20);
    indices.push_back(22);
    indices.push_back(23);

    uiTotalIndices = indices.size();

    return indices;
}

//	OTHER METHODS
//-----------------------------------------------------------
bool CGStaticObject::releaseAll()
{
    if (pVertexBuffer) pVertexBuffer->Release();
    if (pIndexBuffer) pIndexBuffer->Release();
    if (pGroundTextureSRV) pGroundTextureSRV->Release();

    return false;
}