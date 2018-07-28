#pragma once
#include "emitterInterface.h"

class emitterFrags : public emitterInterface
{
public:
    emitterFrags(ParticleBufferData* PartData, UINT MaxParticles = 1000);
    ~emitterFrags();
    virtual void Simulate();

private:
    virtual void init();
    virtual void initVariables();
    virtual void initPhysXBehavior();
    virtual void makeStepInParticleLifeFrame(int CPUIdx);
};

