#include "stdafx.h"
#include "ParticleTechniques.h"

CParticleFlameTechnique::CParticleFlameTechnique(ITechnique* parent)
    : CParticleBaseTechnique(parent)
{
    m_UniBuffData.burn = 1.0f;
    m_UniBuffData.max_size = 2.5f;
    m_UniBuffData.texture_id = EParticleTex::Flame;
}

//////////////////////////////////////////////////////////////////////////
CParticleSmokeTechnique::CParticleSmokeTechnique(ITechnique* parent)\
    : CParticleBaseTechnique(parent)
{
    m_UniBuffData.burn = 0.0f;
    m_UniBuffData.max_size = 3.5f;
    m_UniBuffData.texture_id = EParticleTex::Smoke;
}

//////////////////////////////////////////////////////////////////////////
CParticleDebrisTechnique::CParticleDebrisTechnique(ITechnique* parent)
    : CParticleBaseTechnique(parent)
{
    m_UniBuffData.burn = 0.0f;
    m_UniBuffData.max_size = 0.15f;
    m_UniBuffData.texture_id = EParticleTex::Debris;
}

//////////////////////////////////////////////////////////////////////////
CParticleFlareTechnique::CParticleFlareTechnique(ITechnique* parent)
    : CParticleBaseTechnique(parent)
{
    m_UniBuffData.burn = 1.0f;
    m_UniBuffData.max_size = 0.1f;
    m_UniBuffData.texture_id = EParticleTex::Flare;
}

//////////////////////////////////////////////////////////////////////////
CParticleHaloFlareTechnique::CParticleHaloFlareTechnique(ITechnique* parent)
    : CParticleBaseTechnique(parent)
{
    m_UniBuffData.burn = 0.7f;
    m_UniBuffData.max_size = 12.0f;
    m_UniBuffData.texture_id = EParticleTex::HaloFlare;
}

//////////////////////////////////////////////////////////////////////////
CParticleRoundSparksTechnique::CParticleRoundSparksTechnique(ITechnique* parent)
    : CParticleBaseTechnique(parent)
{
    m_UniBuffData.burn = 1.0f;
    m_UniBuffData.max_size = 5.0f;
    m_UniBuffData.texture_id = EParticleTex::RoundSparks;
}