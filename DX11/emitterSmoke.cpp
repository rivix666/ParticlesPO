#include "stdafx.h"
#include "emitterSmoke.h"
using namespace std;
using namespace physx;

emitterSmoke::emitterSmoke(ParticleBufferData* PartData, UINT MaxParticles) :
emitterInterface(-0.0f, PartData)
{
    m_uiMaxParticles = MaxParticles;
    init();
}

emitterSmoke::~emitterSmoke()
{
}

void emitterSmoke::init()
{
    initVariables();
    initParticleSystem();
    initBuffers();
    initPhysXBehavior();
}

void emitterSmoke::initVariables()
{
    particleTexManager *texMan = particleTexManager::getInstance();
    m_uiTechId = 0;
    m_uiNumber = 2;
    m_dLife = 0.006f;
    m_dMaxSize = 3.0f;
    m_fBurnMod = texMan->m_cParticleSmoke.fBurnMod;
    m_fTexParam = XMFLOAT3(texMan->m_cParticleSmoke.dTexWidth,
        texMan->m_cParticleSmoke.dTexHeight,
        texMan->m_cParticleSmoke.fParticleDepthNum);
    m_dFrame = m_fTexParam.x * m_fTexParam.y;
    setEmitterPosition(XMFLOAT3(0.0f, 4.5f, 8.0f));
}

void emitterSmoke::initPhysXBehavior()
{
    pPxParticles->setDamping(1.5f);
    pPxParticles->setExternalAcceleration(PxVec3(0.f, 0.5f, 0.f));
    pPxParticles->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, true);
}