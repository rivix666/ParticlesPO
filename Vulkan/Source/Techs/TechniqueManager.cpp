#include "stdafx.h"
#include "TechniqueManager.h"
#include "ITechnique.h"

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

uint CTechniqueManager::RegisterTechnique(ITechnique* tech)
{
    m_Techniques.push_back(tech);
    return m_Techniques.size() - 1;
}

uint CTechniqueManager::RegisterTechnique(const std::string& lay_name, ITechnique* tech)
{
    uint res = RegisterTechnique(tech);
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

bool CTechniqueManager::UnregisterTechnique(uint tech_id)
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

bool CTechniqueManager::DeleteTechnique(uint tech_id)
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

uint CTechniqueManager::TechniquesCount() const
{
    return m_Techniques.size();
}

ITechnique* CTechniqueManager::GetTechnique(const uint& tech_id) const
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