#include "stdafx.h"
#include "particleTexManager.h"

ID3DX11EffectShaderResourceVariable* particleTexManager::m_pParticleVolumeTexSRVar;
ID3DX11EffectShaderResourceVariable* particleTexManager::m_pParticleColorSRVar;
particleTexManager*                 particleTexManager::objectInstance;

particleTexManager::particleTexManager() : 
m_NumOfTex(7), m_cParticleSmoke(ParticleTex(0.0f, 0.0f, 8, 8)),
m_cParticleFlame(ParticleTex(1.0f, 0.1875f, 8, 8)),
m_cParticleShockWave(ParticleTex(0.7f, 0.31f, 1, 1)),
m_cParticleFlash(ParticleTex(0.8f, 0.4375f, 1, 1)),
m_cParticleFrags(ParticleTex(0.0f, 0.56f, 3, 3)),
m_cParticleSpark(ParticleTex(1.0f, 0.688f, 1, 1)),
m_cParticleLightning(ParticleTex(0.8f, 0.82f, 1, 1))
{
}

particleTexManager::~particleTexManager()
{
    releaseAll();
}

particleTexManager* particleTexManager::getInstance()
{

    if (objectInstance == nullptr){
        objectInstance = new particleTexManager();
    }
    return objectInstance;
}

bool particleTexManager::init(ID3D11Device *pD3DDevice, ID3DX11Effect *pEffect)
{
    bool bSuccess = true;
    objectInstance->initShadersResourceVariables(pEffect);
    bSuccess &= objectInstance->initParticleTextures(pD3DDevice);
    bSuccess &= objectInstance->initParticleColorTextures(pD3DDevice);
    objectInstance->SetTex();
    return bSuccess;
}

bool particleTexManager::initParticleTextures(ID3D11Device *pD3DDevice)
{
    if (FAILED(D3DX11CreateShaderResourceViewFromFile(pD3DDevice, L"Images/Particles/Tex/volumeTex.dds", NULL, NULL, &m_pParticleVolumeTexSRV, NULL)))
        return utils::FatalError(L"Can't load Tex/volume");
    return true;
}

bool particleTexManager::initParticleColorTextures(ID3D11Device *pD3DDevice)
{
    if (FAILED(D3DX11CreateShaderResourceViewFromFile(pD3DDevice, L"Images/Particles/Gradients/gradients.dds", NULL, NULL, &m_pParticleColorSRV, NULL)))
        return utils::FatalError(L"Can't load Color");
    return true;
}

void particleTexManager::initShadersResourceVariables(ID3DX11Effect* pEffect)
{
    m_pParticleColorSRVar = pEffect->GetVariableByName("txParticleColorGradient")->AsShaderResource();
    m_pParticleVolumeTexSRVar = pEffect->GetVariableByName("txParticleVolumeAtlas")->AsShaderResource(); 
}

void particleTexManager::SetTex()
{
    m_pParticleVolumeTexSRVar->SetResource(m_pParticleVolumeTexSRV);
    m_pParticleColorSRVar->SetResource(m_pParticleColorSRV);
}

//	OTHER METHODS
//-----------------------------------------------------------------------------------

bool particleTexManager::releaseAll()
{
    SAFE_RELEASE(m_pParticleVolumeTexSRV);
    SAFE_RELEASE(m_pParticleColorSRV);
    return false;
}