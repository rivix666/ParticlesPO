#pragma once

class ITechnique;

#define REGISTER_TECH(lay_struct_name, tech) g_Engine->TechMgr()->RegisterTechnique(#lay_struct_name, tech)

class CTechniqueManager
{
public:
    CTechniqueManager();
    ~CTechniqueManager();

    // It will destroy all registered techniques
    void Shutdown();

    // Adds tech into TechniqueMgr takes ownership of the tech
    uint32_t RegisterTechnique(ITechnique* tech);
    uint32_t RegisterTechnique(const std::string& lay_name, ITechnique* tech);

    // Unregister tech from the manager and pass ownership to caller
    bool UnregisterTechnique(ITechnique* tech);
    bool UnregisterTechnique(uint32_t tech_id);

    // Release technique
    bool DeleteTechnique(ITechnique* tech);
    bool DeleteTechnique(uint32_t tech_id);

    // Getters
    uint32_t TechniquesCount() const;
    //uint32_t GetTechIdByLayoutName(const std::string& lay_name) const; // #TECH_UGH moze rejestrowac pod unikalna nazwa by moc fajnie wyciagac bo po co mi w sumie po typie
    ITechnique* GetTechnique(const uint32_t& tech_id) const;

    // Recreate Techniques
    void InitTechniques();
    void ShutdownTechniques();

private:
    typedef std::vector<uint32_t> TIdVec;
    typedef std::vector<ITechnique*> TTechVec;
    typedef std::map<std::string, TIdVec> TLay2TechIdMap;

    TTechVec m_Techniques;
    TLay2TechIdMap m_LayToTech;
};