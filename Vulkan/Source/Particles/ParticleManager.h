#pragma once

#define REGISTER_EMITTER(emitter, class_name, id) g_Engine->ParticleMgr()->RegisterEmitter(emitter, #class_name, id)

class IEmitter;

class CParticleManager
{
public:
    CParticleManager();
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
    int  RegisterEmitter(IEmitter* emitter, const char* name, int id = -1);
    void UnregisterEmitter(int id);

    // Getters/Setters
    int FindEmitterId(IEmitter* emitter) const;
    int FindEmitterId(const char* name) const;
    
    inline IEmitter* GetEmitter(int id) const;

    typedef std::vector<IEmitter*> TEmiVec;
    typedef std::map<IEmitter*, const char*> TEmiChrMap;

private:
    // Emitters
    TEmiVec     m_Emitters;
    TEmiChrMap  m_EmittersIdMap; // #PARTICLES potrzebne?? (napewno wyleci, zamaist tego mapa z emiterami per techniki)

    // Buffers
    VkBuffer m_VertexBuffer = nullptr;
    VkDeviceMemory m_VertexBufferMemory = nullptr;
};

