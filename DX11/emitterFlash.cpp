#include "stdafx.h"
#include "emitterFlash.h"
using namespace std;
using namespace physx;

emitterFlash::emitterFlash(ParticleBufferData* PartData, UINT MaxParticles) :
emitterInterface(-0.0f, PartData)
{
    m_uiMaxParticles = MaxParticles;
    init();
}

emitterFlash::~emitterFlash()
{
}

void emitterFlash::init()
{
    initVariables();
    initParticleSystem();
    initBuffers();
    initPhysXBehavior();
}

void emitterFlash::initVariables()
{
    particleTexManager *texMan = particleTexManager::getInstance();
    m_uiNumber = 1;
    m_dLife = 0.1f;
    m_uiTechId = 0;
    m_dMaxSize = 8.0f;
    m_fBurnMod = texMan->m_cParticleFlash.fBurnMod;
    m_fTexParam = XMFLOAT3(texMan->m_cParticleFlash.dTexWidth,
        texMan->m_cParticleFlash.dTexHeight,
        texMan->m_cParticleFlash.fParticleDepthNum);
    m_dFrame = m_fTexParam.x * m_fTexParam.y;
    setEmitterPosition(XMFLOAT3(0.0f, 0.0f, 0.0f));
}

void emitterFlash::initPhysXBehavior()
{
    pPxParticles->setDamping(0.0f);
    pPxParticles->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, true);
}