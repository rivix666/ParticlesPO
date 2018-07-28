#pragma once
#include "emitterInterface.h"

class emitterSmoke : 
    public emitterInterface
{
public:
    emitterSmoke(ParticleBufferData* PartData, UINT MaxParticles = 4000);
    ~emitterSmoke();

private:
    virtual void init();
    virtual void initVariables();
    virtual void initPhysXBehavior();
};

