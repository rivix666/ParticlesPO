#pragma once
#include "../Techs/ParticleBaseTechnique.h"

#define REGISTER_EMITTER(emitter, id) g_Engine->ParticleMgr()->RegisterEmitter(emitter, id)

class CBaseEmitter;

struct SParticleIdx
{
    uint32_t EmitId = UINT_MAX;
    uint32_t PartId = UINT_MAX;
};

struct SParticleBufferData
{
    // PhysX Info
    uint32_t CPUFree = 0;
    std::vector<uint32_t> PxPool;
    std::vector<SParticleIdx> CPU2Px;

    // Particles Data
    std::vector<ParticleVertex> Particles;

    SParticleBufferData();
};

class CParticleManager
{
public:
    ~CParticleManager();

    // Init/Release
    bool Init();
    bool Shutdown();

    // Simulation
    void Simulate();

    // Update
    void UpdateBuffers();

    // Command buffer handle
    void RecordCommandBuffer(VkCommandBuffer& cmd_buff);

    // Emitters
    void EmitParticles(const uint32_t& idx, const uint32_t& count, const double& in_time = 0.0);

    // Emitter Register
    int  RegisterEmitter(CBaseEmitter* emitter, int id = -1);
    void UnregisterEmitter(int id);

    // Getters/Setters
    int FindEmitterId(CBaseEmitter* emitter) const;
    CBaseEmitter* GetEmitter(int id) const; 

    SParticleBufferData& BuffData() { return m_PartData; }

    // Uni Buffers
    bool CreateUniBuffers();

    // Typedefs // #TYPEDEFS zrobiæ porz¹dek w typedefach, przeniesc te czesciej u¿ywane do stdafx
    typedef std::vector<uint32_t>           TUintVec;
    typedef std::vector<CBaseEmitter*>          TEmiVec;
    typedef std::vector<TEmiVec>            TTechEmiVec;
    typedef std::vector<VkBuffer>           TBufferVec;
    typedef std::vector<VkDeviceMemory>     TBufferMemVec;

protected:
    // Uni Buffers
    void PrepareVectors();
    bool CreateTechUniBuffer(const uint32_t& tech_id);
    void FetchTechUboData(const uint32_t& tech_id, SParticleTechUniBuffer& out);
    void ReleaseUniBuffers();

    // Descriptor Sets
    bool RegisterUniBuffs();

private:
    // Internal structs
    struct SLongEmit
    {
        SLongEmit(const double& t, const uint32_t& i, const uint32_t& p)
            : t_left(t), e_idx(i), p_left(p) {}

        // Time left
        double t_left = 0.0;
        // Emitter idx
        uint32_t e_idx = UINT32_MAX;
        // Particle left
        uint32_t p_left = 0;
    };

    // Emitters
    void EmitParticlesInTime(); // #EMITTERS dokonczyc jak bedzie czas
    
    // Emitters
    TEmiVec                 m_Emitters;
    TTechEmiVec             m_Tech2Emi;
    std::set<SLongEmit*>    m_LongEmitted;

    // Buffers
    VkBuffer                m_VertexBuffer = nullptr;
    VkDeviceMemory          m_VertexBufferMemory = nullptr;
    SParticleBufferData     m_PartData;

    // Uni Buffers
    TBufferVec              m_UniBuffs; // #DESC_MGR REWRITE
    TBufferMemVec           m_UniBuffsMem; // #DESC_MGR REWRITE
};