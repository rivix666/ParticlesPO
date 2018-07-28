#pragma once
#include "explosionInterface.h" 
class fireWorks :
    public explosionInterface
{
public:
    fireWorks(ParticleBufferData* ParticlesData, 
        XMFLOAT3 Pos = XMFLOAT3(0.0f, 4.5f, 0.0f));
    ~fireWorks();

    virtual void Emit();
    virtual void EmitByCycle();

private:
    void init();
    void initFlameTail();
    void initFlameExplosion();
    void initSmokeTail();
    void initFlashExplosion();
    emitterFlame *FlameTail;
    emitterSmoke *SmokeTail;
    emitterFlash *FlashExplosion;
    emitterFlame *FlameExplosion;
};

