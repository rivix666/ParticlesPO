#include "stdafx.h"
#include "BaseEmitter.h"

CBaseEmitter::CBaseEmitter(uint tech_id, uint buff_size)
    : IEmitter(tech_id)
    , m_BuffSize(buff_size)
{
    m_Particles.resize(buff_size);
}

CBaseEmitter::~CBaseEmitter()
{
}

void CBaseEmitter::Emit(uint count)
{
    for (uint i = 0; i < count && m_UsedParticles <= m_BuffSize; i++)
    {
        ParticleVertex particle;
        particle.life = 1.0f;
        particle.pos = glm::vec3(rand() % 3, rand() % 3, rand() % 3);
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

void* CBaseEmitter::ParticlesData()
{
    return &m_Particles[0];
}

size_t CBaseEmitter::ParticlesSize() const
{
    return ParticlesCount() * SingleParticleSize();
}
