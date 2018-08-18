#pragma once
#include "IEmitter.h"

class CBaseEmitter : public IEmitter 
{
public:
    CBaseEmitter(uint tech_id, uint buff_size);
    ~CBaseEmitter();

    // Emit given number of particles
    void Emit(uint count) override;

    // Advance particles simulation
    void Simulate() override;

    // Getters/Setters
    uint ParticlesCount() const override;
    size_t ParticlesSize() const override;
    void* ParticlesData() override;

protected:
    uint m_BuffSize = 0;
    uint m_UsedParticles = 0;
    std::vector<ParticleVertex> m_Particles;
};