#pragma once
#include <d3d11.h> //#REDESIGN move to cpp
#include <d3dx11.h>
#include "Timer.h"
#include "Structs.h"
#include "pxManager.h"
#include "DxCamera.h"
#include "particleTexManager.h"
#include "pxManager.h"

class emitterInterface
{
public:
    emitterInterface(const double ResetLife, ParticleBufferData* PartData);
    virtual ~emitterInterface();

    virtual std::vector<UINT> 
        Emit(bool randomiseVelocities = true);
    virtual void Simulate();

    const float RESET_LIFE;
    const float MAX_LIFE;

    //  CONTAINERS
    //--------------------------------------------------------------------
    ParticleBufferData* g_pPartData;
    std::vector <UINT> PxPool;
    std::vector <UINT> Px2CPU;
    static std::vector <emitterInterface*> g_pAllEmitters;

    //  GETTERS & SETTERS
    //--------------------------------------------------------------------
    physx::PxParticleSystem* getPxParticleSystemPtr();
    double getMaxSize();
    UINT getMaxParticlesNum();
    XMFLOAT3 getEmitterPosition();
    XMFLOAT3 getEmitDirection();
    UINT getParticlesTechId();
    UINT getDeltaNumber();
    double getDeltaLife();
    double getDeltaFrame();
    float getBurnMod();
    XMFLOAT3 getTexParam();
    UINT getEmitterId();

    void setMaxSize(double ms);
    void setEmitterPosition(XMFLOAT3 p);
    void setEmitDirection(XMFLOAT3 d);
    void setParticlesTechId(UINT val);
    void setDeltaNumber(UINT val);
    void setDeltaLife(double val);
    void setDeltaFrame(double val);
    void setBurnMod(float val);
    void setTexParam(XMFLOAT3 val);

    //  PHYSX METHODS
    //--------------------------------------------------------------------
    void disableGravity(bool val);
    void setExternalAcceleration(physx::PxVec3 val);
    void setDamping(double val);

protected:
    physx::PxParticleSystem *pPxParticles;
    physx::PxScene* pPxScene;
    
    XMFLOAT3* m_pEmitterPosition;
    XMFLOAT3* m_pEmitDirection;
    XMFLOAT3 m_fTexParam;
    float m_fBurnMod;
    double m_dFrame;
    double m_dLife;
    UINT m_uiNumber;
    UINT m_uiTechId;
    UINT m_uiMaxParticles;
    UINT m_uiEmitterID;
    double m_dMaxSize;

    //  INIT METHODS
    //-------------------------------------------------------------------
    virtual void init() = NULL;
    void initParticleSystem();
    void initBuffers();

    //  OTHER METHODS
    //-------------------------------------------------------------------
    virtual void createParticles(std::vector <UINT> &particleIdx, std::vector <physx::PxVec3> &particlePos);
    virtual void createParticlesWithRandomVelocities(std::vector <UINT> &particleIdx, std::vector <physx::PxVec3> &particlePos);
    virtual int addNewParticleToBuffer();
    virtual void sendParticlesToPhysX(std::vector<unsigned int>particleIdx, 
        std::vector<physx::PxVec3>particlePos);
    virtual void makeStepInParticleLifeFrame(int CPUIdx);
    virtual void makeStepInParticlePosition(int CPUIdx, const physx::PxVec3& position);
    virtual void randomGenerateXYZ(double& x, double& y, double& z);
    virtual void releaseParticles(const std::vector<physx::PxU32>& indexBuff);
    UINT lowestBitSet(UINT x);
};

