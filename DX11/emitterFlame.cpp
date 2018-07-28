#include "stdafx.h"
#include "emitterFlame.h"
using namespace std;
using namespace physx;

emitterFlame::emitterFlame(ParticleBufferData* PartData, UINT MaxParticles) :
emitterInterface(-0.0f, PartData)
{
    m_uiMaxParticles = MaxParticles;
    init();
}

emitterFlame::~emitterFlame()
{
}

void emitterFlame::init()
{
    initVariables();
    initParticleSystem();
    initBuffers();
    initPhysXBehavior();
}

void emitterFlame::initVariables()
{
    particleTexManager *texMan = particleTexManager::getInstance();
    m_uiNumber = 1;
    m_dLife = /*1.f / (float)(rand() % 30);*/0.008f;
    m_dMaxSize = 2.5f;
    m_fBurnMod = texMan->m_cParticleFlame.fBurnMod;
    m_fTexParam = XMFLOAT3((float)texMan->m_cParticleFlame.dTexWidth,
        (float)texMan->m_cParticleFlame.dTexHeight,
        (float)texMan->m_cParticleFlame.fParticleDepthNum);
    m_dFrame = m_fTexParam.x * m_fTexParam.y;
    setEmitterPosition(XMFLOAT3(0.f, 4.5f, 0.f));
}

void emitterFlame::initPhysXBehavior()
{
    pPxParticles->setDamping(0.0f);
    pPxParticles->setExternalAcceleration(PxVec3(0.f, 8.0f, 0.f));
    pPxParticles->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, true);
}