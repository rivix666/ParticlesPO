#pragma once
#include "IGObject.h"
#include <d3dx11.h>
#include <d3d11.h> //#REDESIGN mvoe to cpp
#include <dxerr.h>
#include "d3dx/d3dx11effect.h"

enum StaticObjType
{
    Floor,
    Box
};

class CGStaticObject : public IGObject
{
public:
    void Render() override;
    bool Init(StaticObjType obj);

    XMMATRIX World;

private:
    ID3D11Device*			g_pD3DDevice;

    ID3DX11EffectTechnique* g_pTechnique;
    ID3D11InputLayout* g_pLayout;
    ID3D11Buffer* pVertexBuffer;
    ID3D11Buffer* pIndexBuffer;

    UINT uiTotalIndices;
    float fTexMultiplier;

    //textures
    ID3D11ShaderResourceView* pGroundTextureSRV;
    ID3DX11EffectShaderResourceVariable* pGroundTextureSR;

    ID3DX11EffectMatrixVariable* pObjectWorldMatrixEffectVariable;
    ID3DX11EffectScalarVariable* pObjectTexMulEffectVariable;

    bool initInputLayoutAndTechnique(ID3DX11Effect* pEffect);
    bool initFloorBuffers();
    bool initBoxBuffers();
    bool initVertexBuffer(std::vector<SimpleVertex> vertices);
    bool initIndexBuffer(std::vector<UINT> indices);
    bool initTextureSRV();
    void initShaderVariables(ID3DX11Effect* pEffect);
    void setShaderVariables();
    void initBoxVariables();
    void initFloorVariables();
    void initEffectVariables(ID3DX11Effect* pEffect);
    std::vector<UINT> initIndicesFloor();
    std::vector<SimpleVertex> initVerticesFloor();
    std::vector<UINT> initIndicesBox();
    std::vector<SimpleVertex> initVerticesBox();

    void prepareBuffersAndInputLayout(); //like set buffers and input layout
    void getTechAndDraw();
    void setEffectVariables();

    bool releaseAll();
};

