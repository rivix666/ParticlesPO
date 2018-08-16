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
    if (FindEmitterId(emitter) < 0)
        return -1;

    // Prepare slot for new emitter
    int tmp_id = -1;
    if (id < 0)
    {
        m_Emitters.push_back(nullptr);
        tmp_id = m_Emitters.size() - 1;
    }
    else
    {
        if (id >= m_Emitters.size())
        {
            uint old_size = m_Emitters.size();
            m_Emitters.resize(id + 1);
            for (uint i = old_size; i < m_Emitters.size(); i++)
            {
                m_Emitters[i] = nullptr;
            }
        }
        tmp_id = id;
    }

    // Register emitter
    m_Emitters[tmp_id] = emitter;
    m_EmittersIdMap[emitter] = name;
}

void CParticleManager::UnregisterEmitter(int id)
{
    IEmitter* emit = m_Emitters[id];
    m_Emitters.erase(m_Emitters.begin() + id, m_Emitters.begin() + id + 1);
    m_EmittersIdMap.erase(emit);
}

int CParticleManager::FindEmitterId(IEmitter* emitter) const
{
    auto it = std::find(m_Emitters.begin(), m_Emitters.end(), emitter);
    if (it != m_Emitters.end())
        return (int)(it - m_Emitters.begin());
    return -1;
}

int CParticleManager::FindEmitterId(const char* name) const
{
    auto it = std::find_if(m_EmittersIdMap.begin(), m_EmittersIdMap.end(), [=](std::pair<IEmitter*, const char*> p) { return strcmp(name, p.second) == 0; });
    if (it != m_EmittersIdMap.end())
        return FindEmitterId((*it).first);
    return -1;
}

IEmitter* CParticleManager::GetEmitter(int id) const
{
    return m_Emitters[id];
}
