#pragma once

#define REGISTER_EMITTER(emitter, id) g_Engine->ParticleMgr()->RegisterEmitter(emitter, id)

class IEmitter;

class CParticleManager
{
public:
    CParticleManager();
    ~CParticleManager();

    typedef unsigned int uint; // #TYPEDEF_UINT czemu nei bierze z stdafx??

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
    
    inline IEmitter* GetEmitter(int id) const;

    typedef std::vector<uint> TUintVec;
    typedef std::vector<IEmitter*> TEmiVec;
    typedef std::vector<TEmiVec> TTechEmiVec;

private:
    // Emitters
    TEmiVec         m_Emitters;
    TUintVec        m_Tech2PCount;
    TTechEmiVec     m_Tech2Emi;
    uint            m_ParticlesNum = 0;

    // Buffers
    VkBuffer        m_VertexBuffer = nullptr;
    VkDeviceMemory  m_VertexBufferMemory = nullptr;
};

