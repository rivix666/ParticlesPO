#pragma once
#include "explosionInterface.h" 
class explosion1 :
    public explosionInterface
{
public:
    explosion1(ParticleBufferData* ParticlesDatas,
        XMFLOAT3 Pos = XMFLOAT3(0.0f, 4.5f, 0.0f));
    ~explosion1();

    virtual void Emit();
    virtual void EmitByCycle();
    virtual void ResetEmitters();

    std::vector <UINT> SmokeIdx;
    std::vector <UINT> FlameIdx;
    std::vector <UINT> FlashIdx;
    std::vector <UINT> ShockWaveIdx;
    std::vector <UINT> FragsIdx;
    std::vector <UINT> SparkIdx;
    std::vector <UINT> FragsSmokeIdx;
    std::vector <UINT> FlameFlameIdx;
    std::vector <UINT> SmokeFlameIdx;
    std::vector <ParticleIdx> FragsSmokeData;
    std::vector <ParticleIdx> FlameSmokeData;

    void Init();
    void InitSmoke();
    void InitFlame();
    void InitFrags();
    void InitFlash();
    void InitSpark();
    void InitShockWave();
    void InitFragsSmoke();
    void InitFlameFlame();
    void InitFlameSmoke();

protected:
    virtual void DeleteEmitters();

    UINT m_uiNumOfFrags;
    UINT m_uiNumOfFlames;
};

