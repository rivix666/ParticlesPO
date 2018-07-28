#include "stdafx.h"
#include "emitterLightning.h"
using namespace std;
using namespace physx;

emitterLightning::emitterLightning(ParticleBufferData* PartData, UINT MaxParticles) :
emitterInterface(-0.0f, PartData)
{
    m_uiMaxParticles = MaxParticles;
    init();
}

emitterLightning::~emitterLightning()
{
}

void emitterLightning::init()
{
    initVariables();
    initParticleSystem();
    initBuffers();
    initPhysXBehavior();
}

void emitterLightning::initVariables()
{
    particleTexManager *texMan = particleTexManager::getInstance();
    m_uiTechId = 0;
    m_uiNumber = 1;
    m_dLife = 0.05f;
    m_dMaxSize = 8.0f;
    m_fBurnMod = texMan->m_cParticleLightning.fBurnMod;
    m_fTexParam = XMFLOAT3(texMan->m_cParticleLightning.dTexWidth,
        texMan->m_cParticleLightning.dTexHeight,
        texMan->m_cParticleLightning.fParticleDepthNum);
    m_dFrame = m_fTexParam.x * m_fTexParam.y;
    setEmitterPosition(XMFLOAT3(0.0f, 4.5f, 0.f));
}

void emitterLightning::initPhysXBehavior()
{
    pPxParticles->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, true);
}