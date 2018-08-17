#include "stdafx.h"
#include "BaseEmitter.h"

CBaseEmitter::CBaseEmitter(uint tech_id, uint buff_size)
    : IEmitter(tech_id)
    , m_BuffSize(buff_size)
{
    m_Particles.resize(buff_size, nullptr);
}

CBaseEmitter::~CBaseEmitter()
{
    utils::DeletePtrVec(m_Particles);
}

void CBaseEmitter::Emit(uint count)
{
    for (uint i = 0; i < count && m_UsedParticles <= m_BuffSize; i++)
    {
        auto particle = new ParticleVertex();
        particle->life = 1.0f;
        particle->pos = glm::vec3(rand() % 3, rand() % 3, rand() % 3);
        m_Particles[m_UsedParticles++] = particle;
    }
}

void CBaseEmitter::Simulate()
{

}

uint CBaseEmitter::ParticlesCount() const
{
    return m_UsedParticles;
}