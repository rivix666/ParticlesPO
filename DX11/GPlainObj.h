#pragma once
#include <d3d11.h> //#REDESIGN move to cpp
#include <d3dx11.h>
#include "IGObject.h"

//#REDESIGN jak bedzie czas
class CGPlainObj : public IGObject
{
public:
    CGPlainObj();
    ~CGPlainObj();

//     void Render() override;
//     bool Init(ID3D11Device* device, ID3DX11Effect* pEffect);
// 
//     XMMATRIX World;

private:
//     bool InitInputLayoutAndTechnique(ID3DX11Effect* pEffect);
//     bool InitBuffers();
//     bool InitVertexBuffer(std::vector<SimpleVertex> vertices);
//     bool InitIndexBuffer(std::vector<UINT> indices);
//     bool InitTextureSRV();
//     void InitShaderVariables(ID3DX11Effect* pEffect);
//     void SetShaderVariables();
//     void InitVariables();
//     void InitEffectVariables(ID3DX11Effect* pEffect);
//     virtual std::vector<UINT> InitIndices();
//     virtual std::vector<SimpleVertex> InitVertices();
// 
//     void PrepareBuffersAndInputLayout(); //like set buffers and input layout
//     void GetTechAndDraw();
//     void SetEffectVariables();
// 
//     bool ReleaseAll();
// 
//     ID3DX11EffectTechnique* g_pTechnique;
//     ID3D10InputLayout* g_pLayout;
//     ID3D10Buffer* pVertexBuffer;
//     ID3D10Buffer* pIndexBuffer;
// 
//     UINT uiTotalIndices;
//     float fTexMultiplier;
// 
//     //textures
//     ID3D10ShaderResourceView* pGroundTextureSRV;
//     ID3DX11EffectShaderResourceVariable* pGroundTextureSR;
// 
//     ID3DX11EffectMatrixVariable* pObjectWorldMatrixEffectVariable;
//     ID3DX11EffectScalarVariable* pObjectTexMulEffectVariable;
};

