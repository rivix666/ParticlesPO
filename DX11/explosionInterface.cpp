#include "stdafx.h"
#include "explosionInterface.h"


explosionInterface::explosionInterface(ParticleBufferData* ParticlesData, XMFLOAT3 Pos) :
g_pParticlesData(ParticlesData), m_ExplosionPos(Pos),
m_bIncreaseExplosionCycle(false), m_uiExplosionCycle(0)
{
}

explosionInterface::~explosionInterface()
{
    //DeleteEmitters();
}

void explosionInterface::Emit()
{
    for (int i = 0; i < EmittersVec.size(); i++)
    {
        EmittersVec[i]->Emit();
    }
}

void explosionInterface::EmitByCycle()
{

}

void explosionInterface::ResetEmitters()
{

}

void explosionInterface::DeleteEmitters()
{
    for each(emitterInterface* e in EmittersVec)
        delete e;  
    EmittersVec.clear();
}

void explosionInterface::Simulate()
{
    for (int i = 0; i < EmittersVec.size(); i++)
    {
        EmittersVec[i]->Simulate();
    }
    if (m_bIncreaseExplosionCycle)
    {
        m_uiExplosionCycle++;
        EmitByCycle();
    }
}

void explosionInterface::GenRandomPosBySphere(emitterInterface* emitter, double radius)
{
    XMFLOAT3 Pos = m_ExplosionPos;
    double psi = MathHelper::dRand(-(HALF_PI), (HALF_PI));
    double fi = MathHelper::dRand(0.0f, 2.0f); 
    Pos.x = Pos.x + radius * cos(psi) * cos(fi);
    Pos.y = Pos.y + radius * cos(psi) * sin(fi);
    Pos.z = Pos.z + radius * sin(psi);
    emitter->setEmitterPosition(Pos);
}

void explosionInterface::GenRandomPosBySphere(emitterInterface* emitter, XMFLOAT3 Pos0, double radius)
{
    double psi = MathHelper::dRand(-HALF_PI, HALF_PI);
    double fi = MathHelper::dRand(0.0f, 2.0f);
    Pos0.x = Pos0.x + radius * cos(psi) * cos(fi);
    Pos0.y = Pos0.y + radius * cos(psi) * sin(fi);
    Pos0.z = Pos0.z + radius * sin(psi);
    emitter->setEmitterPosition(Pos0);
}

XMFLOAT3 explosionInterface::GetExplosionPos() { return m_ExplosionPos; }
void explosionInterface::SetExplosionPos(XMFLOAT3 pos) { m_ExplosionPos = pos; }
