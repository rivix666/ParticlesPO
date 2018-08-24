#pragma once
#include "../../Techs/ParticleBaseTechnique.h"

#define MIN_LIFE 0.0f
#define RESET_LIFE 1.0f

class CParticleManager;

class IEmitter //#EMITTERS przerobic to na baseEmitter, tamten usunac i zrobiæ dot ego IEmitter z prawdziego szdarzenie tzn interfejs
{
public:
    IEmitter(const uint32_t& tech_id, const uint32_t& buffer_size);

    // Emit given number of particles
    virtual void Emit(const uint32_t& count);

    // Advance particles simulation
    virtual void Simulate();

    // Getters/Setters
    const uint32_t& ParticlesCount() const { return m_UsedParticles; }
    const uint32_t& TechId() const { return m_TechId; }
    const uint32_t& BufferSize() const { return m_BuffSize; }
    const uint32_t& EmitterId() const { return m_EmitterId; }
    const glm::vec3& Pos() const { return m_Pos; }
    const glm::vec3& Direction() const { return m_Direction; }
    bool UseParticleDrain() const { return m_UseParticleDrain; }

    void SetPos(const glm::vec3& pos) { m_Pos = pos; }
    void SetDirection(const glm::vec3& dir) { m_Direction = dir; }
    void SetEmitterId(const uint32_t& id) { m_EmitterId = id; }
    void UseParticleDrain(bool use = true) { m_UseParticleDrain = use; }

    CParticleManager* ParticleMgr();

protected:
    // PhysX Init
    physx::PxParticleSystem* CreateParticleSystem();

    // Create/Release particles
    virtual void PrepareParticleSystem(physx::PxParticleSystem* ps);
    virtual void CreateParticles(std::vector<uint32_t>& idx_out, std::vector<physx::PxVec3>& dir_out, const uint32_t& emit_count);
    virtual void ReleaseParticles(const std::vector<uint32_t>& index_buff);

    // Simulate particles
    virtual void MakeStepInParticleLife(const uint32_t& cpu_idx);
    virtual void UpdateParticlePos(const uint32_t& cpu_idx, const physx::PxVec3& new_pos);

    // Particle register
    const uint32_t& ReserveSlotInBuffer();

    // PhysX Handle
    virtual void SendParticlesToPhysX(const std::vector<uint32_t>& idx, const std::vector<physx::PxVec3>& dir);

    // Misc
    uint32_t LowestBitSet(uint32_t x);
    void GenerateRandomXYZ(float& x, float& y, float& z);

    // Tech Id
    uint32_t                    m_TechId = UINT32_MAX;

    // Emitter params
    glm::vec3                   m_Pos = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3                   m_Direction = glm::vec3(0.0f, 0.0f, 0.0f);
    uint32_t                    m_EmitterId = UINT32_MAX;
    uint32_t                    m_BuffSize = 0;

    // PhysX
    bool                        m_UseParticleDrain = true;
    uint32_t                    m_UsedParticles = 0;
    physx::PxScene*             m_PxScene = nullptr;
    physx::PxParticleSystem*    m_PxParticles = nullptr;
    std::vector<uint32_t>       m_Px2CPU;
    std::vector<uint32_t>       m_PxPool;

#if PX_SUPPORT_GPU_PHYSX
    bool                        m_RunOnGpu = false;
#endif
};

