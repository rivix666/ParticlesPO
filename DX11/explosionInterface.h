#pragma once
#include "Structs.h"
#include "particleTexManager.h"
#include "emitterSmoke.h"
#include "emitterFlame.h"
#include "emitterShockWave.h"
#include "emitterFlash.h"
#include "emitterFrags.h"
#include "emitterSpark.h"
#include "emitterLightning.h"

class explosionInterface
{
public:
    explosionInterface(ParticleBufferData* ParticlesData, 
        XMFLOAT3 Pos = XMFLOAT3(0.0f, 4.5f, 0.0f));
    ~explosionInterface();

    virtual void Emit();
    virtual void Simulate();
    virtual void EmitByCycle();
    virtual void ResetEmitters();
   // virtual void DeleteEmitters() = NULL;

    virtual XMFLOAT3 GetExplosionPos();
    virtual void SetExplosionPos(XMFLOAT3 pos);

    ParticleBufferData* g_pParticlesData;
    std::vector <emitterInterface*> EmittersVec;

protected:
    XMFLOAT3 m_ExplosionPos;
    UINT m_uiExplosionCycle;
    bool m_bIncreaseExplosionCycle;

    virtual void GenRandomPosBySphere(emitterInterface* emitter, double radius);
    virtual void GenRandomPosBySphere(emitterInterface* emitter, XMFLOAT3 Pos0, double radius);
    virtual void DeleteEmitters();
};

