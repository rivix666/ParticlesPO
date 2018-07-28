#include "stdafx.h"
#include "emitterInterface.h"

#ifdef _DEBUG
#include <string>
#include <sstream>
#endif;

using namespace std;
using namespace physx;

std::vector <emitterInterface*> emitterInterface::g_pAllEmitters;

emitterInterface::emitterInterface(const double ResetLife, ParticleBufferData* PartData) :
RESET_LIFE(ResetLife), MAX_LIFE(1.0), g_pPartData(PartData),
m_pEmitterPosition(new XMFLOAT3(0.0f, 1.0f, 0.0f)),
m_pEmitDirection(new XMFLOAT3(0.0f, 0.0f, 0.0f)),
m_uiTechId(0), m_uiEmitterID(g_pAllEmitters.size())
{
    g_pAllEmitters.push_back(this);
};
emitterInterface::~emitterInterface()
{
    if (m_uiEmitterID == g_pAllEmitters.size() - 1)
        g_pAllEmitters.pop_back();
    else
        g_pAllEmitters[m_uiEmitterID] = nullptr;
};

//  INIT METHODS
//-------------------------------------------------------------------
void emitterInterface::initParticleSystem(){
    pxManager* pPxMan;
    pPxMan = pxManager::getInstance();
    pPxScene = pPxMan->getPxScene();
    pPxParticles = pPxMan->getPxSDK()->createParticleSystem(m_uiMaxParticles);
    pPxParticles->setRestOffset(0.0f);
    pPxScene->addActor(*pPxParticles);
}

void emitterInterface::initBuffers()
{
    Px2CPU.resize(m_uiMaxParticles);
    for (int i = 0; i < m_uiMaxParticles; i++)
        PxPool.push_back(m_uiMaxParticles - 1 - i);
}

//  MAIN METHODS
//-------------------------------------------------------------------
std::vector<UINT> emitterInterface::Emit(bool randomiseVelocities)
{
    vector <UINT> particleIdx; //temporary index vector
    vector <PxVec3> particlePos; //temporary velocity vector
    if (randomiseVelocities)
        createParticlesWithRandomVelocities(particleIdx, particlePos);
    else
        createParticles(particleIdx, particlePos);

    if (particleIdx.size() > 0) 
        sendParticlesToPhysX(particleIdx, particlePos);

    return particleIdx;
}

void emitterInterface::Simulate()
{
    std::vector<PxU32> indexBuff;
    PxParticleReadData* rd = pPxParticles->lockParticleReadData();
    if (rd->validParticleRange > 0)
    {
        PxStrideIterator<const PxVec3> positions(rd->positionBuffer);
        PxStrideIterator<const PxParticleFlags> particleFlags(rd->flagsBuffer);
        // iterate over valid particle bitmap
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

                if (particleFlags[index] & PxParticleFlag::eCOLLISION_WITH_DRAIN 
                    || g_pPartData->CPU_Particle_Data[CPUIdx].Life > MAX_LIFE)
                {
                    g_pPartData->CPU_Particle_Data[CPUIdx].Life = RESET_LIFE;
                    g_pPartData->CPU_Particle_Data[CPUIdx].Frame = 0.0f;
                    indexBuff.push_back(index); //mark particle for delete / replace
                   // debug::Out(L"Particle to release - idx: %u\n", index);
                }
            }
        }
    }
    rd->unlock();
    releaseParticles(indexBuff); // we need to release after unlock, otherwise assert
}

//  OTHER METHODS
//-------------------------------------------------------------------
void emitterInterface::createParticles(vector <UINT> &particleIdx, vector <PxVec3> &particlePos)
{
    for (UINT i = 0; i < m_uiNumber; i++)
    {
        if (PxPool.size() < 1 ||
            g_pPartData->m_iCPUFree > g_pPartData->MAX_PARTICLES) break;
        int PxIdx = addNewParticleToBuffer();
        particleIdx.push_back(PxIdx);
        particlePos.push_back(PxVec3(m_pEmitDirection->x,
            m_pEmitDirection->y, m_pEmitDirection->z));
    }
}

void emitterInterface::createParticlesWithRandomVelocities(vector <UINT> &particleIdx, vector <PxVec3> &particlePos)
{
    double dx, dy, dz;
    for (UINT i = 0; i < m_uiNumber; i++)
    {
        if (PxPool.size() < 1 ||
            g_pPartData->m_iCPUFree > g_pPartData->MAX_PARTICLES) break;
        int PxIdx = addNewParticleToBuffer();
        randomGenerateXYZ(dx, dy, dz);
        particleIdx.push_back(PxIdx);
        particlePos.push_back(PxVec3(dx, dy, dz));
    }
}

void emitterInterface::sendParticlesToPhysX(std::vector<unsigned int>particleIdx, std::vector<PxVec3>particlePos)
{
    PxParticleCreationData particleCreationData;
    PxVec3 tempEmitPos(m_pEmitterPosition->x, m_pEmitterPosition->y, m_pEmitterPosition->z);

    particleCreationData.numParticles = particleIdx.size();
    particleCreationData.indexBuffer = PxStrideIterator<const PxU32>(&particleIdx[0]);
    particleCreationData.positionBuffer = PxStrideIterator<const PxVec3>(&tempEmitPos, 0);
    particleCreationData.velocityBuffer = PxStrideIterator<const PxVec3>(&particlePos[0]);
    pPxParticles->createParticles(particleCreationData);
}

int emitterInterface::addNewParticleToBuffer()
{
    UINT CPUIdx = g_pPartData->m_iCPUFree++;
    UINT PxIdx = PxPool.back();
    PxPool.pop_back();

    Px2CPU[PxIdx] = CPUIdx;
    g_pPartData->CPU2Px[CPUIdx].PartId = PxIdx; 
    g_pPartData->CPU2Px[CPUIdx].EmitId = m_uiEmitterID;

    g_pPartData->CPU_Particle_Data[CPUIdx].Life = RESET_LIFE;
    g_pPartData->CPU_Particle_Data[CPUIdx].Frame = MathHelper::dRand(0.0, m_dFrame);
    g_pPartData->CPU_Particle_Data[CPUIdx].Burn = m_fBurnMod;
    g_pPartData->CPU_Particle_Data[CPUIdx].TexWHD = m_fTexParam;
    g_pPartData->CPU_Particle_Data[CPUIdx].Tech = m_uiTechId;
    g_pPartData->CPU_Particle_Data[CPUIdx].MaxSize = m_dMaxSize;

   // g_pPartData->CPU_Particle_Pos[CPUIdx] = XMFLOAT3(0.0f, 0.0f, 0.0f);

    return PxIdx;
}

void emitterInterface::makeStepInParticleLifeFrame(int CPUIdx) //#REDESIGN too slow, allokacje
{
    if (CPUIdx < 0) return;
    float Frame = m_dFrame * g_pPartData->CPU_Particle_Data.at(CPUIdx).Life;
    if (Frame > m_dFrame)
        Frame = fmod(Frame, m_dFrame + 1.0f);

    g_pPartData->CPU_Particle_Data[CPUIdx].Frame = Frame;
    g_pPartData->CPU_Particle_Data[CPUIdx].Life += m_dLife;
}

void emitterInterface::makeStepInParticlePosition(int CPUIdx, const PxVec3& position) //#REDESIGN too slow, allokacje
{
    if (CPUIdx < 0) return;

    g_pPartData->CPU_Particle_Pos[CPUIdx].x = position.x;
    g_pPartData->CPU_Particle_Pos[CPUIdx].y = position.y;
    g_pPartData->CPU_Particle_Pos[CPUIdx].z = position.z;
}

void emitterInterface::randomGenerateXYZ(double& x, double& y, double& z){
    x = (m_pEmitDirection->x + double(rand() % 5000 - 2500) / double(-200)) * (double)(rand() % 100); //#REDESIGN ostatni modyfikator trzeba ustawic by particl sam sobie wybeiral jakie rzedu ma byc
    y = (m_pEmitDirection->y + double(rand() % 5000 - 2500) / double(-200)) * (double)(rand() % 100); //#REDESIGN ostatni modyfikator trzeba ustawic by particl sam sobie wybeiral jakie rzedu ma byc
    z = (m_pEmitDirection->z + double(rand() % 5000 - 2500) / double(-200)) * (double)(rand() % 100); //#REDESIGN ostatni modyfikator trzeba ustawic by particl sam sobie wybeiral jakie rzedu ma byc
}

void emitterInterface::releaseParticles(const std::vector<PxU32>& indexBuff)
{
    if (indexBuff.size() > 0)
    {
        for (int i = 0; i < indexBuff.size(); i++)
        {
            PxPool.push_back(indexBuff[i]); //return deleted particles to PxPool
           
            UINT dziura = Px2CPU[indexBuff[i]];
            UINT last = --g_pPartData->m_iCPUFree;

            if (dziura == last)
            {
                g_pPartData->CPU_Particle_Data[last].Life = -1.f;
                g_pPartData->CPU_Particle_Pos[last] = XMFLOAT3(0.f, -1.f, 0.f);
                continue;
            }

            g_pPartData->CPU_Particle_Data[dziura] = g_pPartData->CPU_Particle_Data[last];
            g_pPartData->CPU_Particle_Pos[dziura] = g_pPartData->CPU_Particle_Pos[last];

            g_pPartData->CPU_Particle_Data[last].Life = -1;
            g_pPartData->CPU_Particle_Pos[last] = XMFLOAT3(0.f, -1.f, 0.f);
            g_pPartData->CPU2Px[dziura] = g_pPartData->CPU2Px[last];
            emitterInterface::g_pAllEmitters[g_pPartData->CPU2Px[dziura].EmitId]->Px2CPU[g_pPartData->CPU2Px[dziura].PartId] = dziura;
        }
        PxStrideIterator<const PxU32> indexBuffer(&indexBuff[0], sizeof(PxU32));
        pPxParticles->releaseParticles(indexBuff.size(), indexBuffer);
    }
}

UINT emitterInterface::lowestBitSet(UINT x)
{
    if (x == 0)
        return 0;
    unsigned int result = 0;
    while ((x & 1) == 0)
    {
        x >>= 1;
        result++;
    }
    return result;
}

//  PHYSX METHODS
//--------------------------------------------------------------------`
void emitterInterface::disableGravity(bool val)
{
    pPxParticles->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, val);
}

void emitterInterface::setExternalAcceleration(PxVec3 val)
{
    pPxParticles->setExternalAcceleration(val);
}

void emitterInterface::setDamping(double val)
{
    pPxParticles->setDamping(val);
}

//  GETTERS & SETTERS
//--------------------------------------------------------------------
PxParticleSystem* emitterInterface::getPxParticleSystemPtr() { return pPxParticles; }
double emitterInterface::getMaxSize() { return m_dMaxSize; }
UINT emitterInterface::getMaxParticlesNum() { return m_uiMaxParticles; }
XMFLOAT3 emitterInterface::getEmitterPosition() { return *m_pEmitterPosition; }
XMFLOAT3 emitterInterface::getEmitDirection() { return *m_pEmitDirection; }
UINT emitterInterface::getParticlesTechId() { return m_uiTechId; }
UINT emitterInterface::getDeltaNumber() { return m_uiNumber; }
double emitterInterface::getDeltaLife() { return m_dLife; }
double emitterInterface::getDeltaFrame() { return m_dFrame; }
float emitterInterface::getBurnMod() { return m_fBurnMod; }
XMFLOAT3 emitterInterface::getTexParam() { return m_fTexParam; }
UINT emitterInterface::getEmitterId() { return m_uiEmitterID;  }

void emitterInterface::setMaxSize(double ms) { m_dMaxSize = ms; }
void emitterInterface::setEmitterPosition(XMFLOAT3 p) { *m_pEmitterPosition = p; }
void emitterInterface::setEmitDirection(XMFLOAT3 d) { *m_pEmitDirection = d; }
void emitterInterface::setParticlesTechId(UINT val) { m_uiTechId = val; }
void emitterInterface::setDeltaNumber(UINT val) { m_uiNumber = val; }
void emitterInterface::setDeltaLife(double val) { m_dLife = val; }
void emitterInterface::setDeltaFrame(double val) { m_dFrame = val; }
void emitterInterface::setBurnMod(float val) { m_fBurnMod = val; }
void emitterInterface::setTexParam(XMFLOAT3 val) { m_fTexParam = val; }
