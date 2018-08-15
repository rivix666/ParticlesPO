#pragma once

#define REGISTER_EMITTER(emitter, class_name, id) { }

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
    TEmiVec     m_Emitters;
    TEmiChrMap  m_EmittersIdMap;
};

