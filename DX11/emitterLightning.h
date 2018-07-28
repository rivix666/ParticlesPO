#pragma once
#include "emitterInterface.h"

class emitterLightning :
    public emitterInterface
{
public:
    emitterLightning(ParticleBufferData* PartData, UINT MaxParticles = 1);
    ~emitterLightning();

private:
    virtual void init();
    virtual void initVariables();
    virtual void initPhysXBehavior();
};

