#include "stdafx.h"
#include "emitterFrags.h"
using namespace std;
using namespace physx;

emitterFrags::emitterFrags(ParticleBufferData* PartData, UINT MaxParticles) :
emitterInterface(-0.0f, PartData)
{
    m_uiMaxParticles = MaxParticles;
    init();
}

emitterFrags::~emitterFrags()
{
}

void emitterFrags::init()
{
    initVariables();
    initParticleSystem();
    initBuffers();
    initPhysXBehavior();
}

void emitterFrags::initVariables()
{
    particleTexManager *texMan = particleTexManager::getInstance();
    m_uiNumber = 1;
    m_dLife = 0.0001f;
    m_dMaxSize = 0.15f;
    m_uiTechId = 2;
    m_fBurnMod = texMan->m_cParticleFrags.fBurnMod;
    m_fTexParam = XMFLOAT3(texMan->m_cParticleFrags.dTexWidth,
        texMan->m_cParticleFrags.dTexHeight,
        texMan->m_cParticleFrags.fParticleDepthNum);
    m_dFrame = m_fTexParam.x * m_fTexParam.y;
    setEmitterPosition(XMFLOAT3(0.0f, 4.5f, -6.f));
}

void emitterFrags::initPhysXBehavior()
{
    pPxParticles->setDamping((physx::PxReal)0.0);
    pPxParticles->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, false);
    pPxParticles->setRestitution((physx::PxReal)0.4);
    pPxParticles->setParticleMass((physx::PxReal)1.0);
}

void emitterFrags::Simulate()
{
    std::vector<PxU32>  indexBuff;
    PxParticleReadData* rd = pPxParticles->lockParticleReadData();
    if (rd->validParticleRange > 0)
    {
        PxStrideIterator<const PxVec3> positions(rd->positionBuffer);
        PxStrideIterator<const PxParticleFlags> particleFlags(rd->flagsBuffer);
        for (PxU32 w = 0; w <= (rd->validParticleRange - 1) >> 5; w++)
        {

            for (PxU32 b = rd->validParticleBitmap[w]; b; b &= b - 1)
            {
                PxU32 index = (w << 5 | lowestBitSet(b));

                // access particle position
                const PxVec3& position = positions[index];
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
    releaseParticles(indexBuff); // we need to release after unlock, otherwise assert
}

void emitterFrags::makeStepInParticleLifeFrame(int CPUIdx){
    if (CPUIdx < 0) return;
    float Frame = g_pPartData->CPU_Particle_Data[CPUIdx].Frame + 2.0f * (float)g_Engine->GetTimer().getLastElapsedTime();
    if (Frame > m_dFrame)
        Frame = fmod(Frame, m_dFrame + 1.0f);

    g_pPartData->CPU_Particle_Data[CPUIdx].Frame = Frame;
    g_pPartData->CPU_Particle_Data[CPUIdx].Life += m_dLife;
}