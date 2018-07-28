#include "stdafx.h"
#include "fireWorks.h"

using namespace physx;

fireWorks::fireWorks(ParticleBufferData* ParticlesData, XMFLOAT3 Pos) :
explosionInterface(ParticlesData, Pos)
{
    init();
}

fireWorks::~fireWorks()
{
    DeleteEmitters();
}

void fireWorks::init()
{
    initFlameTail();
    initFlameExplosion();
    initSmokeTail();
    initFlashExplosion();
}

void fireWorks::initFlameTail()
{
    FlameTail = new emitterFlame(g_pParticlesData);
    FlameTail->setDeltaNumber(1);
    FlameTail->setMaxSize(1.0f);
    FlameTail->setDeltaLife(0.2f);
    FlameTail->disableGravity(true);
    FlameTail->setParticlesTechId(3);
    EmittersVec.push_back(FlameTail);
}

void fireWorks::initFlameExplosion()
{
    FlameExplosion = new emitterFlame(g_pParticlesData);
    FlameExplosion->setDeltaLife(MathHelper::dRand(0.05f, 0.1f));
    FlameExplosion->setDeltaNumber(20);
    FlameExplosion->setMaxSize(0.5f);
    FlameExplosion->disableGravity(true);
    FlameExplosion->setExternalAcceleration(PxVec3(0.0f, 0.2f, 0.0f));
    FlameExplosion->setParticlesTechId(3);
    EmittersVec.push_back(FlameExplosion);
}

void fireWorks::initSmokeTail()
{
    SmokeTail = new emitterSmoke(g_pParticlesData);
    SmokeTail->setDeltaNumber(1);
    SmokeTail->setBurnMod(0.0f);
    SmokeTail->setMaxSize(1.0f);
    SmokeTail->setDeltaLife(0.01f);
    SmokeTail->disableGravity(true);
    SmokeTail->setExternalAcceleration(PxVec3(0.0f, 0.5f, 0.0f));
    SmokeTail->setDamping(10.0f);
    EmittersVec.push_back(SmokeTail);
}

void fireWorks::initFlashExplosion()
{
    FlashExplosion = new emitterFlash(g_pParticlesData, 200);
    FlashExplosion->setDeltaNumber(100);
    FlashExplosion->setBurnMod(0.8f);
    FlashExplosion->setDeltaLife(0.01f);
    FlashExplosion->setMaxSize(1.2f);
    FlashExplosion->disableGravity(true);
    FlashExplosion->setDamping(2.0f);
    FlashExplosion->setExternalAcceleration(PxVec3(0.0f, 0.0f, 0.0f));
    EmittersVec.push_back(FlashExplosion);
}

void fireWorks::Emit()
{
    m_uiExplosionCycle = 0;
    m_bIncreaseExplosionCycle = true;
    FlameTail->setEmitterPosition(m_ExplosionPos);
}

void fireWorks::EmitByCycle()
{
    static UINT loopOffset = 0;
    if (m_uiExplosionCycle < 100)
    {
        if ((loopOffset & 1) == 0)
        {
            XMFLOAT3 pos = FlameTail->getEmitterPosition();
            double x = pos.x + MathHelper::dRand(-0.3f, 0.3f);
            double y = pos.y + 0.2f;
            double z = pos.z + MathHelper::dRand(-0.3f, 0.3f);
            FlameTail->setEmitterPosition(XMFLOAT3(x, y, z));
            SmokeTail->setEmitterPosition(XMFLOAT3(x, y, z));
            FlameTail->Emit(false);
            SmokeTail->Emit(true);
        }
    }
    else
    {
        FlashExplosion->setEmitterPosition(FlameTail->getEmitterPosition());
        FlameExplosion->setEmitterPosition(FlameTail->getEmitterPosition());
        FlashExplosion->Emit(true);
        FlameExplosion->Emit(true);
        m_bIncreaseExplosionCycle = false;
        return;
    }
    loopOffset++;
}