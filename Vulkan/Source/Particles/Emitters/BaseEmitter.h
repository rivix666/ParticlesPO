#pragma once
#include "IEmitter.h"

class CBaseEmitter : public IEmitter 
{
public:
    CBaseEmitter(uint32_t tech_id, uint32_t buff_size);
    ~CBaseEmitter();

    // Emit given number of particles
    void Emit(uint32_t count) override;

    // Advance particles simulation
    void Simulate() override; // #PARTICLES jeden bazowy emitter a symulacje oddelegowac do technik??

    // Getters/Setters
    uint32_t ParticlesCount() const override;
    size_t ParticlesSize() const override;
    void* ParticlesData() override;

protected:
    uint32_t m_BuffSize = 0;
    uint32_t m_UsedParticles = 0;
    std::vector<ParticleVertex> m_Particles;
};