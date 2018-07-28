#ifndef __BASE_EFFECT__
#define __BASE_EFFECT__

#include "Particles.fx"
#include "ParticleSort.fx"

matrix World;
Texture2D groundTex;

// update PerObject
float StaticObjectTexMul;
matrix StaticObjWorld;

SamplerState AnisotropicSampler
{
    Filter = ANISOTROPIC;
    MaxAnisotropy = 16;
    AddressU = Wrap;
    AddressV = Wrap;
};

BlendState AlphaBlending
{
    AlphaToCoverageEnable = FALSE;
    BlendEnable[0] = TRUE;
    SrcBlend = SRC_ALPHA;
    DestBlend = INV_SRC_ALPHA;
    BlendOp = ADD;
    SrcBlendAlpha = ZERO;
    DestBlendAlpha = ZERO;
    BlendOpAlpha = ADD;
    RenderTargetWriteMask[0] = 0x0F;
};

struct VS_INPUT_BASE
{
    float4 Pos : POSITION;
    float2 Tex : TEXCOORD;
};

struct PS_INPUT_BASE
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD;
};

PS_INPUT_BASE VS_BASE(VS_INPUT_BASE input)
{
    PS_INPUT_BASE output;

    matrix ViewProjection = mul(View, Projection);

    input.Pos = mul(input.Pos, World);
    input.Pos = mul(input.Pos, StaticObjWorld);
    output.Pos = mul(input.Pos, ViewProjection);

    output.Tex = input.Tex;

    return output;
}

float4 PS_BASE(PS_INPUT_BASE input) : SV_Target
{
    return groundTex.Sample(AnisotropicSampler, input.Tex * StaticObjectTexMul);
}

technique11 BaseTech
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS_BASE()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS_BASE()));
        SetBlendState(AlphaBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
        SetRasterizerState(CullOn);
    }
}

#endif // __BASE_EFFECT__