#include "stdafx.h"
#include "TechniqueManager.h"
#include "Interface/ITechnique.h"

CTechniqueManager::CTechniqueManager()
{
}

CTechniqueManager::~CTechniqueManager()
{
}

void CTechniqueManager::Shutdown()
{
    for (auto tech : m_Techniques)
    {
        tech->Shutdown();
        DELETE(tech);
    }
}

uint32_t CTechniqueManager::RegisterTechnique(ITechnique* tech)
{
    if (m_Techniques.size() >= TECH_MAX)
        return UINT32_MAX;

    m_Techniques.push_back(tech);
    return m_Techniques.size() - 1;
}

uint32_t CTechniqueManager::RegisterTechnique(const std::string& lay_name, ITechnique* tech)
{
    uint32_t res = RegisterTechnique(tech);
    m_LayToTech[lay_name].push_back(res);
    return res;
}

bool CTechniqueManager::UnregisterTechnique(ITechnique* tech)
{
    if (!tech)
        return false;

    auto found = std::find(m_Techniques.begin(), m_Techniques.end(), tech);
    if (found == m_Techniques.end())
        return false;

    m_Techniques[found - m_Techniques.begin()] = nullptr;

    return true;
}

bool CTechniqueManager::UnregisterTechnique(uint32_t tech_id)
{
    if (tech_id >= m_Techniques.size())
        return false;

    m_Techniques[tech_id] = nullptr;

    return true;
}

bool CTechniqueManager::DeleteTechnique(ITechnique* tech)
{
    if (!tech)
        return false;

    if (!UnregisterTechnique(tech))
        return false;

    tech->Shutdown();
    DELETE(tech);
    
    return true;
}

bool CTechniqueManager::DeleteTechnique(uint32_t tech_id)
{
    if (tech_id >= m_Techniques.size())
        return false;

    auto tech = m_Techniques[tech_id];
    UnregisterTechnique(tech_id);

    if (tech)
    { 
        tech->Shutdown();
        DELETE(tech);
    }

    return true;
}

uint32_t CTechniqueManager::TechniquesCount() const
{
    return m_Techniques.size();
}

// uint32_t CTechniqueManager::GetTechIdByLayoutName(const std::string& lay_name) const
// {
//     auto it = m_LayToTech.find(lay_name);
//     if (it != m_LayToTech.end())
//     {
//         return it->second;
//     }
//     return UINT_MAX;
// }

ITechnique* CTechniqueManager::GetTechnique(const uint32_t& tech_id) const
{
    return m_Techniques[tech_id];
}

void CTechniqueManager::InitTechniques()
{
    for (auto tech : m_Techniques)
    {
        tech->Init();
    }
}

void CTechniqueManager::ShutdownTechniques()
{
    for (auto tech : m_Techniques)
    {
        tech->Shutdown();
    }
}