#include "stdafx.h"
#include "BaseEmitter.h"

CBaseEmitter::CBaseEmitter(uint32_t tech_id, uint32_t buff_size)
    : IEmitter(tech_id)
    , m_BuffSize(buff_size)
{
    m_Particles.resize(buff_size);
}

CBaseEmitter::~CBaseEmitter()
{
}

void CBaseEmitter::Emit(uint32_t count)
{
    for (uint32_t i = 0; i < count && m_UsedParticles <= m_BuffSize; i++)
    {
        ParticleVertex particle;
        particle.life = 1.0f;
        particle.pos = glm::vec3(rand() % 3, rand() % 3, rand() % 3);
        m_Particles[m_UsedParticles++] = particle;
    }
}

void CBaseEmitter::Simulate()
{
    for (uint32_t i = 0; i < m_UsedParticles; i++)
    {
        //m_Particles[i].life -= 0.0001f;
        //m_Particles[i].life = m_Particles[i].life < 0.0f ? 0.0f : m_Particles[i].life;
    }
}

uint32_t CBaseEmitter::ParticlesCount() const
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
