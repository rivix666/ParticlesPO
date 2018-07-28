#pragma once
#include "emitterInterface.h"
#include <list>
class emitterShockWave :
    public emitterInterface
{
public:
    emitterShockWave(ParticleBufferData* PartData, UINT MaxParticles = 2);
    ~emitterShockWave();

    virtual void Simulate();

private:
    virtual void init();
    virtual void initVariables();
    virtual void initPhysXBehavior();
};

