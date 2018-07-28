#ifndef __PARTICLES_SORT_EFFECT__
#define __PARTICLES_SORT_EFFECT__

#include "Particles.fx"

#define PARTICLE_BUFF_SIZE 24576    // Change also in stdafx.h
#define BITONIC_BLOCK_SIZE 32    // Change also in ParticleSort.fx

uint ParticlesNum;
float3 CamNormPos;

uint SortLevel;
uint SortLevelMask;
uint SortWidth;
uint SortHeight;

StructuredBuffer<uint> IdxIn; // dla index bufferu ma tu byc zwykly buffer ale dla input indexow strucured //#TEST
RWStructuredBuffer<uint> IdxOut : register(u0);

groupshared uint shared_data[BITONIC_BLOCK_SIZE];

[numthreads(BITONIC_BLOCK_SIZE, 1, 1)]
void CSSort(uint3 DTid : SV_DispatchThreadID,
            uint3 GTid : SV_GroupThreadID,
            uint  GI : SV_GroupIndex)
{
    for (uint i = 2; i <= ParticlesNum; i <<= 1)
    {
        for (uint j = i >> 1; j > 0; j >>= 1)
        {
            for (uint k = GI; k < ParticlesNum >> 1; k += BITONIC_BLOCK_SIZE)
            {
                uint l = k / j * j + k;

                if (l + j < ParticlesNum)
                {

                    float3 l_pos = ParticlePosBuff[IdxOut[l]];
                    float3 r_pos = ParticlePosBuff[IdxOut[l + j]];
                    float left = dot(CamNormPos, l_pos);
                    float right = dot(CamNormPos, r_pos);

                    if (k & (i >> 1))
                    {
                        // descending order
                        if (left < right)
                        {
                            uint temp = IdxOut[l];
                            IdxOut[l] = IdxOut[l + j];
                            IdxOut[l + j] = temp;
                        }
                    }
                    else
                    {
                        // ascending order
                        if (left > right)
                        {
                            uint temp = IdxOut[l];
                            IdxOut[l] = IdxOut[l + j];
                            IdxOut[l + j] = temp;
                        }
                    }
                }
            }

            GroupMemoryBarrierWithGroupSync();
        }
    }
}

technique11 ParticleSort
{
    pass P0
    {
        SetVertexShader(NULL);
        SetPixelShader(NULL);
        SetComputeShader(CompileShader(cs_5_0, CSSort()));
    }
}

#endif // __PARTICLES_SORT_EFFECT__