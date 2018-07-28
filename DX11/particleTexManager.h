#pragma once
#include <d3d11.h>
#include <d3dx11.h> //#REDESIGN przeniesc do cpp
#include "Structs.h"
#include "d3dx/d3dx11effect.h"

class particleTexManager
{
public:
    ~particleTexManager();

    static particleTexManager* getInstance();
    bool init(ID3D11Device *pD3DDevice, ID3DX11Effect *pEffect);
    void SetTex();

    const ParticleTex m_cParticleFlame;
    const ParticleTex m_cParticleSmoke;
    const ParticleTex m_cParticleShockWave;
    const ParticleTex m_cParticleFlash;
    const ParticleTex m_cParticleFrags;
    const ParticleTex m_cParticleSpark;
    const ParticleTex m_cParticleLightning;

private:
    particleTexManager();

    UINT m_NumOfTex;
    static particleTexManager* objectInstance;

    bool initParticleTextures(ID3D11Device *pD3DDevice);
    bool initParticleColorTextures(ID3D11Device *pD3DDevice);
    static void initShadersResourceVariables(ID3DX11Effect* pEffect);

    static ID3DX11EffectShaderResourceVariable* m_pParticleColorSRVar;
    static ID3DX11EffectShaderResourceVariable* m_pParticleVolumeTexSRVar;
    ID3D11ShaderResourceView* m_pParticleVolumeTexSRV;
    ID3D11ShaderResourceView* m_pParticleColorSRV;

    //other methods
    bool releaseAll();
};

