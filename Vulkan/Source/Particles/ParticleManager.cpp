#include "stdafx.h"
#include "ParticleManager.h"
#include "Emitters/IEmitter.h"

CParticleManager::CParticleManager()
{
}


CParticleManager::~CParticleManager()
{
}

bool CParticleManager::Init()
{
    return true;
}

bool CParticleManager::Shutdown()
{
    return false;
}

void CParticleManager::Simulate()
{

}

int CParticleManager::RegisterEmitter(IEmitter* emitter, const char* name, int id /*= -1*/)
{
    return -1;
}

void CParticleManager::UnregisterEmitter(int id)
{

}

int CParticleManager::FindEmitterId(IEmitter* emitter) const
{
    return -1;
}

int CParticleManager::FindEmitterId(const char* name) const
{
    return -1;
}

IEmitter* CParticleManager::GetEmitter(int id) const
{
    return nullptr;
}
