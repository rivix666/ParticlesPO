#include "stdafx.h"
#include "emitterSpark.h"
using namespace std;
using namespace physx;

emitterSpark::emitterSpark(ParticleBufferData* PartData, UINT MaxParticles) :
emitterInterface(-0.0f, PartData)
{
    m_uiMaxParticles = MaxParticles;
    init();
}

emitterSpark::~emitterSpark()
{
}

void emitterSpark::init()
{
    initVariables();
    initParticleSystem();
    initBuffers();
    initPhysXBehavior();
}

void emitterSpark::initVariables()
{
    particleTexManager *texMan = particleTexManager::getInstance();
    m_uiNumber = 2;
    m_dLife = 0.01f;
    m_dMaxSize = 5.0f;
    m_uiTechId = 0;
    m_fBurnMod = texMan->m_cParticleSpark.fBurnMod;
    m_fTexParam = XMFLOAT3(texMan->m_cParticleSpark.dTexWidth, 
        texMan->m_cParticleSpark.dTexHeight, 
        texMan->m_cParticleSpark.fParticleDepthNum);
    m_dFrame = m_fTexParam.x * m_fTexParam.y;
    setEmitterPosition(XMFLOAT3(0.0f, 4.5f, 0.f));
}

void emitterSpark::initPhysXBehavior()
{
    pPxParticles->setDamping(2.0f);
    pPxParticles->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, true);
}