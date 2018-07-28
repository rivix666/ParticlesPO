#include "stdafx.h"
#include "emitterShockWave.h"

emitterShockWave::emitterShockWave(ParticleBufferData* PartData, UINT MaxParticles) :
emitterInterface(-0.0f, PartData)
{
    m_uiMaxParticles = MaxParticles;
    init();
}

emitterShockWave::~emitterShockWave()
{
}

void emitterShockWave::init()
{
    particleTexManager *texMan = particleTexManager::getInstance();
    initVariables();
    initParticleSystem();
    initBuffers();
    initPhysXBehavior();
}

void emitterShockWave::initVariables()
{
    particleTexManager *texMan = particleTexManager::getInstance();
    m_uiNumber = 1;
    m_dLife = 0.06f;
    m_dMaxSize = 12.0f;
    m_uiTechId = 1;
    m_dFrame = 1.0f;
    m_fBurnMod = texMan->m_cParticleShockWave.fBurnMod;
    m_fTexParam = XMFLOAT3(texMan->m_cParticleShockWave.dTexWidth,
        texMan->m_cParticleShockWave.dTexHeight,
        texMan->m_cParticleShockWave.fParticleDepthNum);
    m_dFrame = m_fTexParam.x * m_fTexParam.y;
    setEmitterPosition(XMFLOAT3(0.0f, 0.0f, 0.0f));
}

void emitterShockWave::initPhysXBehavior()
{
    pPxParticles->setActorFlag(physx::PxActorFlag::eDISABLE_GRAVITY, true);
}

void emitterShockWave::Simulate()
{
    std::vector<physx::PxU32>  indexBuff;
    physx::PxParticleReadData* rd = pPxParticles->lockParticleReadData();
    if (rd->validParticleRange > 0)
    {
        physx::PxStrideIterator<const physx::PxVec3> positions(rd->positionBuffer);
        physx::PxStrideIterator<const physx::PxParticleFlags> particleFlags(rd->flagsBuffer);
        // iterate over valid particle bitmap
        for (physx::PxU32 w = 0; w <= (rd->validParticleRange - 1) >> 5; w++)
        {
            for (physx::PxU32 b = rd->validParticleBitmap[w]; b; b &= b - 1)
            {
                physx::PxU32 index = (w << 5 | lowestBitSet(b));
                // access particle position
                const physx::PxVec3& position = positions[index];
                int CPUIdx = Px2CPU[index];
                makeStepInParticlePosition(CPUIdx, position);
                makeStepInParticleLifeFrame(CPUIdx);

                if (g_pPartData->CPU_Particle_Data[CPUIdx].Life > MAX_LIFE) 
                {
                    g_pPartData->CPU_Particle_Data[CPUIdx].Life = RESET_LIFE;
                    g_pPartData->CPU_Particle_Data[CPUIdx].Frame = 0.0;
                    indexBuff.push_back(index); //mark particle for delete / replace                
                }
            }
        }
    }
    rd->unlock();
    releaseParticles(indexBuff);  // we need to release after unlock, otherwise assert
}
