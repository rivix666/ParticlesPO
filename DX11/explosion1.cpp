#include "stdafx.h"
#include "explosion1.h"

using namespace physx;

explosion1::explosion1(ParticleBufferData* ParticlesData, XMFLOAT3 Pos) :
explosionInterface(ParticlesData, Pos), m_uiNumOfFrags(0), m_uiNumOfFlames(0)
{
    Init();
}

explosion1::~explosion1()
{
    DeleteEmitters();
}

void explosion1::Emit()
{
    m_uiExplosionCycle = 0;
    m_bIncreaseExplosionCycle = true;

    for each(UINT ui in FlashIdx)
    {
        EmittersVec[ui]->Emit(false);
    }
}

void explosion1::EmitByCycle()
{
    if (m_uiExplosionCycle < 40)
        switch (m_uiExplosionCycle)
    {
        case 5:
        {
            for each(UINT ui in ShockWaveIdx)
            {
                EmittersVec[ui]->Emit(false);
            }
            /////////////////////////////////////////////////////////////
            std::vector <UINT> tempinio;
            UINT emitID = EmittersVec[FlameIdx[FlameIdx.size() - 1]]->getEmitterId();
            tempinio = EmittersVec[FlameIdx[FlameIdx.size() - 1]]->Emit();
            for each(UINT u in tempinio)
            {
                FlameSmokeData.push_back(ParticleIdx(emitID, u));
            }
            /////////////////////////////////////////////////////////////
            if (FlameIdx.size() > 0)
                EmittersVec[FlameIdx[0]]->Emit();
            break;
        }
        case 15:
            for (int i = 1; i < FlameIdx.size() - 1; i++)
                EmittersVec[FlameIdx[i]]->Emit();
            for each(UINT ui in SparkIdx)
            {
                EmittersVec[ui]->Emit();
            }
            for each(UINT ui in SmokeIdx)
            {
                EmittersVec[ui]->Emit();
            }
            /////////////////////////////////////////////////////////////
            std::vector <UINT> tempinio2;
            UINT emitID2;
            for each(UINT ui in FragsIdx)
            {
                emitID2 = EmittersVec[ui]->getEmitterId();
                tempinio2 = EmittersVec[ui]->Emit();
                for each(UINT u in tempinio2)
                {
                    FragsSmokeData.push_back(ParticleIdx(emitID2, u));
                }
            }
            /////////////////////////////////////////////////////////////
            break;
    }
    else
    {
        static UINT loopOffset = 0;

        if (m_uiExplosionCycle > 150)
        {
            m_bIncreaseExplosionCycle = false; //it will end explosion cycle
            FragsSmokeData.clear();
            return;
        }

        //if loopOffset is even then emit fragSmoke
        if ((loopOffset & 1) == 0)
        {
            XMFLOAT3 TempPos;
            UINT CPUIdx;
            UINT fsi;
            for (int i = 0; i < FragsSmokeIdx.size(); i++)
            {
                CPUIdx = emitterInterface::g_pAllEmitters[FragsSmokeData[i].EmitId]->Px2CPU[FragsSmokeData[i].PartId];
                fsi = FragsSmokeIdx[i];
                if (g_pParticlesData->CPU_Particle_Data[CPUIdx].Life < 0.0f)
                    continue;

                TempPos = g_pParticlesData->CPU_Particle_Pos[CPUIdx];
                EmittersVec[fsi]->setEmitterPosition(TempPos);
                EmittersVec[fsi]->Emit();
            }

            if (m_uiExplosionCycle < 120)
            {
                UINT ffi, sfi;
                for (int i = 0; i < FlameFlameIdx.size(); i++)
                {
                    CPUIdx = emitterInterface::g_pAllEmitters[FlameSmokeData[i].EmitId]->Px2CPU[FlameSmokeData[i].PartId];
                    ffi = FlameFlameIdx[i];
                    sfi = SmokeFlameIdx[i];
                    if (g_pParticlesData->CPU_Particle_Data[CPUIdx].Life < 0.0f)
                        continue;

                    TempPos = g_pParticlesData->CPU_Particle_Pos[CPUIdx];
                    EmittersVec[ffi]->setEmitterPosition(TempPos);
                    EmittersVec[ffi]->Emit();
                    EmittersVec[sfi]->setEmitterPosition(TempPos);
                    EmittersVec[sfi]->Emit();
                }
            }
        }
        loopOffset++;
    }
}
std::vector <ParticleIdx> FlameSmokeData;
void explosion1::ResetEmitters()
{
    DeleteEmitters();
    m_uiNumOfFrags = 0;
    Init();
}

void explosion1::DeleteEmitters()
{
    explosionInterface::DeleteEmitters();
    SmokeIdx.clear();
    FlameIdx.clear();
    FlashIdx.clear();
    ShockWaveIdx.clear();
    FragsIdx.clear();
    SparkIdx.clear();
    FragsSmokeIdx.clear();
    FlameFlameIdx.clear();
    SmokeFlameIdx.clear();
    FragsSmokeData.clear();
    FlameSmokeData.clear();
}

void explosion1::Init()
{
    InitFlame();
    InitSmoke();
    InitFlash();
    InitFrags();
    InitSpark();
    InitShockWave();
    InitFragsSmoke();
    InitFlameSmoke();
    InitFlameFlame();
}

void explosion1::InitSmoke()
{
    emitterSmoke* Smoke;
    double burnMod = 0.0f;
    UINT deltaNum = 10;

    for (int j = 0; j < FlameIdx.size(); j++)
    for (int i = 0; i < 3; i++)
    {
        Smoke = new emitterSmoke(g_pParticlesData);
        GenRandomPosBySphere(Smoke, EmittersVec[FlameIdx[j]]->getEmitterPosition(), 2.5f);
        Smoke->setDeltaNumber(deltaNum);
        Smoke->setBurnMod(burnMod);
        Smoke->setDeltaLife(MathHelper::dRand(0.002f, 0.005f));
        Smoke->disableGravity(true);
        Smoke->setDamping(2.5f);
        Smoke->setMaxSize(4.0f);
        Smoke->setExternalAcceleration(PxVec3(0.0f, 0.5f, 0.0f));

        SmokeIdx.push_back(EmittersVec.size());
        EmittersVec.push_back(Smoke);
    }
}

void explosion1::InitFlame()
{
    emitterFlame* Flame = new emitterFlame(g_pParticlesData);

    Flame->setEmitterPosition(m_ExplosionPos);
    Flame->setDeltaLife(0.05f);
    Flame->setDeltaNumber(50);
    Flame->disableGravity(true);
    Flame->setExternalAcceleration(PxVec3(0.0f, 0.2f, 0.0f));

    FlameIdx.push_back(EmittersVec.size());
    EmittersVec.push_back(Flame);

    for (int i = 0; i < 4; i++)
    {
        Flame = new emitterFlame(g_pParticlesData);

        GenRandomPosBySphere(Flame, m_ExplosionPos, 2.0);

        Flame->setDeltaLife(MathHelper::dRand(0.05f, 0.03f));
        Flame->setDeltaNumber(20);
        Flame->disableGravity(true);
        Flame->setExternalAcceleration(PxVec3(0.0f, -0.2f, 0.0f));

        FlameIdx.push_back(EmittersVec.size());
        EmittersVec.push_back(Flame);
    }


    Flame = new emitterFlame(g_pParticlesData);

    Flame->setEmitterPosition(XMFLOAT3(m_ExplosionPos.x, m_ExplosionPos.y, m_ExplosionPos.z));
    Flame->setDeltaLife(0.007f);
    m_uiNumOfFlames = rand() % 6 + 5;
    Flame->setDeltaNumber(m_uiNumOfFlames);
    Flame->setMaxSize(0.5f);
    Flame->disableGravity(true);
    Flame->setDamping(0.5f);
    Flame->setEmitDirection(XMFLOAT3(0.0f, 1.0f, 0.0f));
    Flame->setExternalAcceleration(PxVec3(0.0f, 0.0f, 0.0f));

    FlameIdx.push_back(EmittersVec.size());
    EmittersVec.push_back(Flame);
}

void explosion1::InitFrags()
{
    emitterFrags* Frags;
    UINT deltaNum;

    for (int i = 0; i < FlameIdx.size(); i++)
        for (int j = 0; j < 1; j++)
    {
        Frags = new emitterFrags(g_pParticlesData);

        GenRandomPosBySphere(Frags, EmittersVec[FlameIdx[i]]->getEmitterPosition(), 1.0);
        deltaNum = rand() % 2 + 2;
        m_uiNumOfFrags += deltaNum;
        Frags->setDeltaNumber(deltaNum);
        Frags->setMaxSize(MathHelper::dRand(0.1f, 0.2f));
        Frags->setDeltaLife(MathHelper::dRand(0.0006f, 0.001f));
        Frags->getPxParticleSystemPtr()->setRestitution(0.6);
        Frags->getPxParticleSystemPtr()->setParticleMass(1.0f);

        FragsIdx.push_back(EmittersVec.size());
        EmittersVec.push_back(Frags);
    }
}

void explosion1::InitFlash()
{
    emitterFlash* Flash;
    double burnMod = 1.0f;
    UINT deltaNum = 1;

    for (int i = 0; i < 4; i++)
    {
        Flash = new emitterFlash(g_pParticlesData);
        GenRandomPosBySphere(Flash, 0.5f);
        Flash->setDeltaNumber(deltaNum);
        Flash->setBurnMod(burnMod);
        Flash->disableGravity(true);
        Flash->setExternalAcceleration(PxVec3(0.0f, 0.0f, 0.0f));

        FlashIdx.push_back(EmittersVec.size());
        EmittersVec.push_back(Flash);
    }
}

void explosion1::InitShockWave()
{
    emitterShockWave* ShockWave = new emitterShockWave(g_pParticlesData);

    ShockWave->setEmitterPosition(XMFLOAT3(m_ExplosionPos.x, m_ExplosionPos.y - 4.0, m_ExplosionPos.z));

    ShockWaveIdx.push_back(EmittersVec.size());
    EmittersVec.push_back(ShockWave);
}

void explosion1::InitSpark()
{
    emitterSpark* Spark;
    double burnMod = 1.0f;
    UINT deltaNum = 10;

    for (int j = 0; j < FlameIdx.size(); j++)
        for (int i = 0; i < 4; i++)
        {
            Spark = new emitterSpark(g_pParticlesData);
            GenRandomPosBySphere(Spark, EmittersVec[FlameIdx[j]]->getEmitterPosition(), 1.0);
            Spark->setDeltaNumber(deltaNum);
            Spark->setBurnMod(burnMod);
            Spark->setDeltaLife(0.03f);
            Spark->disableGravity(true);
            Spark->setDamping(4.0f);
            Spark->setExternalAcceleration(PxVec3(0.0f, 0.5f, 0.0f));

            SparkIdx.push_back(EmittersVec.size());
            EmittersVec.push_back(Spark);
        }
}

void explosion1::InitFragsSmoke()
{
    emitterSmoke* Smoke;
    double burnMod = 0.0f;
    UINT deltaNum = 1;
    for (int j = 0; j < m_uiNumOfFrags; j++)
        {
            Smoke = new emitterSmoke(g_pParticlesData);
            Smoke->setDeltaNumber(deltaNum);
            Smoke->setBurnMod(burnMod);
            Smoke->setDeltaLife(0.01f);
            Smoke->disableGravity(true);
            Smoke->setDamping(40.0f);
            Smoke->setMaxSize(0.3f);
            Smoke->setExternalAcceleration(PxVec3(0.0f, 0.5f, 0.0f));

            FragsSmokeIdx.push_back(EmittersVec.size());
            EmittersVec.push_back(Smoke);
        }
}

void explosion1::InitFlameFlame()
{
    emitterFlame* FlameTail;

    for (int i = 0; i < m_uiNumOfFlames; i++)
    {
        FlameTail = new emitterFlame(g_pParticlesData);
        FlameTail->setDeltaNumber(1);
        FlameTail->setMaxSize(1.0f);
        FlameTail->setDeltaLife(0.05f);
        FlameTail->disableGravity(true);
        FlameTail->setExternalAcceleration(PxVec3(0.0f, 0.8f, 0.0f));
        FlameTail->setDamping(20.0f);

        FlameFlameIdx.push_back(EmittersVec.size());
        EmittersVec.push_back(FlameTail);
    }


}

void explosion1::InitFlameSmoke()
{
    emitterSmoke *SmokeTail;

    for (int i = 0; i < m_uiNumOfFlames; i++)
    {
        SmokeTail = new emitterSmoke(g_pParticlesData);
        SmokeTail->setDeltaNumber(2);
        SmokeTail->setBurnMod(0.0f);
        SmokeTail->setMaxSize(1.2f);
        SmokeTail->setDeltaLife(0.004f);
        SmokeTail->disableGravity(true);
        SmokeTail->setExternalAcceleration(PxVec3(0.0f, 0.5f, 0.0f));
        SmokeTail->setDamping(10.0f);

        SmokeFlameIdx.push_back(EmittersVec.size());
        EmittersVec.push_back(SmokeTail);
    }
}