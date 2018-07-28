#pragma once

class IGObject;
class ITechnique;

#define REGISTER_OBJ(obj) { g_Engine->ObjectControl()->RegisterObject(obj); }
#define REGISTER_OBJ_TECH(tech, obj) { g_Engine->ObjectControl()->RegisterObject(tech, obj); }
#define UNREGISTER_OBJ(obj) { g_Engine->ObjectControl()->UnregisterObject(obj); }
#define UNREGISTER_OBJ_TECH(tech, obj) { g_Engine->ObjectControl()->UnregisterObject(tech, obj); }
/*#define REGISTER_OBJ_LAY(#v_layout, obj) {  }*/ // #TECH tutaj chcialem zrobic rejestracje obiektu rpzez jego vertex layout ale z jevnego vertex layoutu moze korzystac kilka technik...

class CGObjectControl
{
public:
    CGObjectControl(VkDevice device);
    ~CGObjectControl();

    // Typedefs
    typedef unsigned int uint; // #TYPEDEF_UINT czemu nei bierze z stdafx?? cos z dodaniuem includow do headera engine?
    typedef std::vector<size_t> TCacheVec;
    typedef std::vector<IGObject*> TObjectsVec;
    typedef std::vector<TObjectsVec> TTech2ObjVec;

    void Shutdown();

    void RegisterObject(IGObject* obj);
    void RegisterObject(const uint& tech, IGObject* obj);
    void UnregisterObject(IGObject* obj);
    void UnregisterObject(const uint& tech, IGObject* obj);

    void RecordCommandBuffer(VkCommandBuffer& cmd_buff);

    void UpdateUniBuffers();

    const TTech2ObjVec& GetTech2ObjVec() const { return m_TechToObjVec; }

private:
    void EnsureTechIdWillFit(const uint& tech_id);

    // Device
    VkDevice m_Device = nullptr;

    // Objects
    TCacheVec m_SizeCacheVec;
    TTech2ObjVec m_TechToObjVec;
};

