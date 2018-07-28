#pragma once

#include <d3d11.h> //#REDESIGN move to cpp
#include <d3dx11.h>

struct SimpleVertex
{
    XMFLOAT3 pos;
    XMFLOAT2 tex;

    SimpleVertex(XMFLOAT3 p, XMFLOAT2 t) : pos(p), tex(t) {}
};

struct ParticleData
{
    float Life;
    float Frame;
    XMFLOAT3 TexWHD; //Depth == Color
    float Burn;
    UINT Tech;
    float MaxSize;

    ParticleData() :
        Life(-1.0f), Frame(0.0f),
        Burn(1.0f), Tech(0), MaxSize(1.0f) {};
    ParticleData(const ParticleData &part) = default;

    ParticleData& operator=(const ParticleData& p1)
    {
        Life = p1.Life;
        Frame = p1.Frame;
        TexWHD = p1.TexWHD;
        Burn = p1.Burn;
        Tech = p1.Tech;
        MaxSize = p1.MaxSize;
        return *this;
    }
};

struct ParticleIdx
{
    UINT EmitId;
    UINT PartId;

    ParticleIdx() = default;
    ParticleIdx(UINT eID, UINT pID) :
        EmitId(eID), PartId(pID) {}
};

struct ParticleBufferData
{
    std::vector <XMFLOAT3> CPU_Particle_Pos;
    std::vector <ParticleData> CPU_Particle_Data;
    std::vector <UINT> PxPool;
    std::vector <ParticleIdx> CPU2Px;
    UINT m_iCPUFree;
    const UINT MAX_PARTICLES;

    ParticleBufferData(UINT MaxParticles) :
        MAX_PARTICLES(MaxParticles),
        m_iCPUFree(0)
    {
        CPU_Particle_Pos.resize(MaxParticles);
        CPU_Particle_Data.resize(MaxParticles);
        CPU2Px.resize(MaxParticles);
        for (int i = 0; i < MaxParticles; i++)
        {
            PxPool.push_back(MaxParticles - i - 1);
            CPU_Particle_Pos.push_back(XMFLOAT3(0.f, -1.f, 0.f));
        }
    }
};

struct ParticleTex
{
    float fBurnMod;
    float fParticleDepthNum;
    UINT dTexWidth;
    UINT dTexHeight;

    ParticleTex(float burn, float depth, UINT width, UINT height) :
    fBurnMod(burn), fParticleDepthNum(depth), dTexWidth(width), dTexHeight(height) {};
    ParticleTex(){};
    ~ParticleTex(){};
};