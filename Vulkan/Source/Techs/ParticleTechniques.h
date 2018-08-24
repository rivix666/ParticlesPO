#pragma once
#include "ParticleBaseTechnique.h"

class CParticleFlameTechnique : public CParticleBaseTechnique
{
public:
    CParticleFlameTechnique(ITechnique* parent);
};

//////////////////////////////////////////////////////////////////////////
class CParticleSmokeTechnique : public CParticleBaseTechnique
{
public:
    CParticleSmokeTechnique(ITechnique* parent);
};

//////////////////////////////////////////////////////////////////////////
class CParticleDebrisTechnique : public CParticleBaseTechnique
{
public:
    CParticleDebrisTechnique(ITechnique* parent);
};

//////////////////////////////////////////////////////////////////////////
class CParticleFlareTechnique : public CParticleBaseTechnique
{
public:
    CParticleFlareTechnique(ITechnique* parent);
};

//////////////////////////////////////////////////////////////////////////
class CParticleHaloFlareTechnique : public CParticleBaseTechnique
{
public:
    CParticleHaloFlareTechnique(ITechnique* parent);
};

//////////////////////////////////////////////////////////////////////////
class CParticleRoundSparksTechnique : public CParticleBaseTechnique
{
public:
    CParticleRoundSparksTechnique(ITechnique* parent);
};