#pragma once
#include "emitterInterface.h"

class emitterSpark :
    public emitterInterface
{
public:
    emitterSpark(ParticleBufferData* PartData, UINT MaxParticles = 400);
    ~emitterSpark();

private:
    virtual void init();
    virtual void initVariables();
    virtual void initPhysXBehavior();
};