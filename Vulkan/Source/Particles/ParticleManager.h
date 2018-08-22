#pragma once
#include "../Techs/ParticleBaseTechnique.h"

#define REGISTER_EMITTER(emitter, id) g_Engine->ParticleMgr()->RegisterEmitter(emitter, id)

class IEmitter;

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

    // Emitter Register
    int  RegisterEmitter(IEmitter* emitter, int id = -1);
    void UnregisterEmitter(int id);

    // Getters/Setters
    int FindEmitterId(IEmitter* emitter) const;
    IEmitter* GetEmitter(int id) const; 

    // Uni Buffers
    bool CreateUniBuffers();

    // Typedefs
    typedef std::vector<uint32_t>                   TUintVec;
    typedef std::vector<IEmitter*>                  TEmiVec;
    typedef std::vector<TEmiVec>                    TTechEmiVec;
    typedef std::vector<VkBuffer>                   TBufferVec;
    typedef std::vector<VkDeviceMemory>             TBufferMemVec;

protected:
    // Uni Buffers
    void PrepareVectors();
    bool CreateTechUniBuffer(const uint32_t& tech_id);
    void FetchTechUboData(const uint32_t& tech_id, SParticleTechUniBuffer& out);
    void ReleaseUniBuffers();

    // Descriptor Sets
    bool RegisterUniBuffs();

private:
    // Emitters
    TEmiVec             m_Emitters;
    TUintVec            m_Tech2PCount;
    TTechEmiVec         m_Tech2Emi;
    uint32_t            m_ParticlesNum = 0;

    // Buffers
    VkBuffer            m_VertexBuffer = nullptr;
    VkDeviceMemory      m_VertexBufferMemory = nullptr;

    // Uni Buffers
    TBufferVec          m_UniBuffs; // #DESC_MGR REWRITE
    TBufferMemVec       m_UniBuffsMem; // #DESC_MGR REWRITE
};