#pragma once
#include "emitterInterface.h"
class emitterFlash :
    public emitterInterface
{
public:
    emitterFlash(ParticleBufferData* PartData, UINT MaxParticles = 200);
    ~emitterFlash();

private:
    virtual void init();
    virtual void initVariables();
    virtual void initPhysXBehavior();
};

