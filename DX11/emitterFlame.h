#pragma once
#include "emitterInterface.h"

class emitterFlame : public emitterInterface
{
public:
    emitterFlame(ParticleBufferData* PartData, UINT MaxParticles = 1500); // #FX_REDESIGN 0 jako max powinno zonaczac brak limitu
    ~emitterFlame();

private:
    virtual void init();
    virtual void initVariables();
    virtual void initPhysXBehavior();
};