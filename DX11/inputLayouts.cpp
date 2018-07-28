#include "stdafx.h"
#include "inputLayouts.h"
#include <d3d11.h>
#include <d3dx11.h>
#include "d3dx/d3dx11effect.h"

inputLayouts* inputLayouts::objectInstance;

inputLayouts::inputLayouts()
{
    ZeroMemory(&LayoutsArr, __COUNT__ * sizeof(ID3D11InputLayout*)); // #CHECK czy nie czysci za duzo
    ZeroMemory(&TechArr, __COUNT__ * sizeof(ID3DX11EffectTechnique*));
}

inputLayouts::~inputLayouts()
{
}

inputLayouts* inputLayouts::getInstance(ID3D11Device *pD3DDevice, ID3DX11Effect *pEffect)
{
    if (objectInstance == nullptr){
        objectInstance = new inputLayouts();
        objectInstance->initTechniques(pEffect);
        if(!objectInstance->initLayouts(pD3DDevice, pEffect))return nullptr; 
        //if initLayouts() fail then return nullptr and whole app will close
    }

    return objectInstance;
}

//	INIT METHODS
//-----------------------------------------------------------------------------------
void inputLayouts::initTechniques(ID3DX11Effect *pEffect)
{
    TechArr[BASE] = pEffect->GetTechniqueByName("BaseTech");
    TechArr[PARTICLE] = pEffect->GetTechniqueByName("Particle");
    TechArr[PARTICLE_SORT] = pEffect->GetTechniqueByName("ParticleSort");
}

bool inputLayouts::initLayouts(ID3D11Device *pD3DDevice, ID3DX11Effect *pEffect)
{
    if (!LayoutsArr[BASE])
        if (!initBaseLayout(pD3DDevice, pEffect)) return releaseAll();
    if (!LayoutsArr[PARTICLE])
        if (!initParticleLayout(pD3DDevice, pEffect)) return releaseAll();

    return true;
}


bool inputLayouts::initBaseLayout(ID3D11Device *pD3DDevice, ID3DX11Effect *pEffect)
{
    D3D11_INPUT_ELEMENT_DESC layout[] = 
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    UINT numElements = 2;
    D3DX11_PASS_DESC PassDesc;
    TechArr[BASE]->GetPassByIndex(0)->GetDesc(&PassDesc);

    if (FAILED(pD3DDevice->CreateInputLayout(layout,
        numElements, PassDesc.pIAInputSignature,
        PassDesc.IAInputSignatureSize, &LayoutsArr[BASE]))) return utils::FatalError(L"Could not create input layout BASE");

    return true;
}

bool inputLayouts::initParticleLayout(ID3D11Device *pD3DDevice, ID3DX11Effect *pEffect)
{
    const D3D11_INPUT_ELEMENT_DESC layout[] =
    {
         { "LIFE", 0, DXGI_FORMAT_R32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
         { "FRAME", 0, DXGI_FORMAT_R32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
         { "TEXPARAM", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
         { "BURN", 0, DXGI_FORMAT_R32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
         { "TECH", 0, DXGI_FORMAT_R32_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
         { "MAX_SIZE", 0, DXGI_FORMAT_R32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    UINT numElements = sizeof(layout) / sizeof(layout[0]);
    D3DX11_PASS_DESC PassDesc;
    TechArr[PARTICLE]->GetPassByIndex(0)->GetDesc(&PassDesc);

    if (FAILED(pD3DDevice->CreateInputLayout(layout,
        numElements, PassDesc.pIAInputSignature,
        PassDesc.IAInputSignatureSize, &LayoutsArr[PARTICLE]))) return utils::FatalError(L"Could not create input layout PARTICLE");

    return true;
}


//	GETTERS
//-----------------------------------------------------------------------------------
ID3D11InputLayout* inputLayouts::getLayout(eInputLayouts val)
{
    if (val < __COUNT__)
        return LayoutsArr[val];
    return nullptr;
}

ID3DX11EffectTechnique* inputLayouts::getTechnique(eInputLayouts val)
{
    if (val < __COUNT__)
        return TechArr[val];
    return nullptr;
}

//	OTHER METHODS
//-----------------------------------------------------------------------------------
bool inputLayouts::releaseAll()
{
    for (int i = 0; i < __COUNT__; i++)
    {
        SAFE_RELEASE(LayoutsArr[i]);
        SAFE_RELEASE(TechArr[i]);
    }
    return false;
}