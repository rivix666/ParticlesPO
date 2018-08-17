#pragma once
#include "../../Techs/ParticleBaseTechnique.h"

class IEmitter
{
public:
    IEmitter(uint tech_id);

    // Emit given number of particles
    virtual void Emit(uint count) = 0;

    // Advance particles simulation
    virtual void Simulate() = 0;

    // Getters/Setters
    virtual uint ParticlesCount() const = 0;

    const glm::vec3& Pos() const { return m_Pos; }
    void SetPos(const glm::vec3& pos) { m_Pos = pos; }

    uint TechId() const { return m_TechId; }

    static size_t SingleParticleSize() { return sizeof(ParticleVertex); }

protected:
    // Tech Id
    uint m_TechId = UINT_MAX;

    // Emitter position
    glm::vec3 m_Pos;
};

