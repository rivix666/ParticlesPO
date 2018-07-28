#ifndef __PARTICLES_EFFECT__
#define __PARTICLES_EFFECT__

Texture2D txParticleColorGradient;
Texture3D txParticleVolumeAtlas;
Texture2D txDepth;
float3 CameraUp;

// update perFrame
float3 CameraPos;
matrix View;
matrix Projection;
matrix InvProjection;

// particle pos buffer
StructuredBuffer<float3> ParticlePosBuff;

//  SAMPLERS
//------------------------------------------------------
SamplerState samLinear
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
};

SamplerState samLinearClamp
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Clamp;
    AddressV = Clamp;
};

SamplerState samPoint
{
    Filter = min_mag_mip_point;
    AddressU = MIRROR;
    AddressV = MIRROR;
};

//  BLEND STATES
//------------------------------------------------------
BlendState ControlledAdditiveBlending
{
    AlphaToCoverageEnable = FALSE;
    BlendEnable[0] = TRUE;
    SrcBlend = ONE;
    DestBlend = INV_SRC_ALPHA;
    BlendOp = ADD;
    SrcBlendAlpha = ZERO;
    DestBlendAlpha = ZERO;
    BlendOpAlpha = ADD;
    RenderTargetWriteMask[0] = 0x0F;
};

//  RASTERIZER STATES
//------------------------------------------------------
RasterizerState CullOn
{
    CullMode = BACK;
};

RasterizerState CullOff
{
    CullMode = NONE;
};

//  INPUT
//------------------------------------------------------
struct VSParticleIn
{
    //float3 Pos              : POSITION;
    float  Life             : LIFE;
    float  Frame            : FRAME;
    float3 TexParam         : TEXPARAM;
    float  Burn             : BURN;
    uint   Tech             : TECH;
    float  MaxSize          : MAX_SIZE;
    uint   Idx              : SV_VertexID;
};

struct GSParticleIn
{
    float3 Pos              : POSITION;
    float Life              : LIFE;
    float Frame             : FRAME;
    float Burn              : BURN;
    float3 TexParam         : TEXPARAM;
    uint Tech               : TECH;
    float MaxSize           : MAX_SIZE;
};

struct PSParticleIn
{
    float4 Pos			    : SV_POSITION;
    float2 Tex			    : TEXCOORD0;
    float2 ScreenTex	    : TEXCOORD1;
    float2 Depth		    : TEXCOORD2;
    float4 particleColor    : COLOR;
    uint Tech               : TECH;
    float Life              : LIFE;
    float Burn              : BURN;
    float TexParam          : TEXPARAM;
};

//  METHODS INIT
//------------------------------------------------------
//  GS METHODS INIT
void GS_ShockWave(point GSParticleIn input[1], inout float2 texCoord[4], inout float3 vVerts[4], in float particleColorAlpha);
void GS_Default(point GSParticleIn input[1], inout float2 texCoord[4], inout float3 vVerts[4], in float particleColorAlpha);
void CalcRightAndUpCoordShift(out float3 vRight, out float3 vUp, in float3 vPlaneNormal);
void CalcParticleSizeByGradientAlpha(inout float3 vRight, inout float3 vUp, in float particleColorAlpha, in float MaxSize);
void RotateParticle(inout float3 vRight, inout float3 vUp, in float Life);
void CalcParticleVertexPos(out float3 vVerts[4], in float3 vRight, in float3 vUp, in float3 Pos);
void CalcFrameCoordInTexAtlas(out float2 texCoord[4], float3 TexParam, float Frame);

//  PS METHODS INIT
void CalcSoftParticles(inout float depthFade, in float2 ScreenTex, in float2 Depth);
float4 CalcControlAdditiveBlend(in float4 particleSample, in float Burn);

//  PARTICLE
//------------------------------------------------------
GSParticleIn VSParticle(VSParticleIn input)
{
    GSParticleIn output;

    output.Pos = ParticlePosBuff[input.Idx];

    output.Life = input.Life;
    output.Frame = input.Frame;
    output.Burn = input.Burn;
    output.TexParam = input.TexParam;
    output.Tech = input.Tech;
    output.MaxSize = input.MaxSize;

    return output;
}

[maxvertexcount(4)]
void GSParticle(point GSParticleIn input[1], inout TriangleStream<PSParticleIn> SpriteStream)
{
    if (input[0].Life > 0.f)
    {
        PSParticleIn output;
        float2 texCoord[4];
        float3 vVerts[4];
        output.particleColor = txParticleColorGradient.SampleLevel(samLinearClamp, float2(input[0].Life, input[0].TexParam.b), 0);

        switch (input[0].Tech)
        {
            case 1: //  shockwave
                GS_ShockWave(input, texCoord, vVerts, output.particleColor.a);
                break;
            default:
                GS_Default(input, texCoord, vVerts, output.particleColor.a);
                break;
        }

        output.Life = input[0].Life;
        output.Burn = input[0].Burn;
        output.Tech = input[0].Tech;
        output.TexParam = input[0].TexParam.b;

        matrix view_proj = mul(View, Projection);
        [unroll] for (int i = 0; i < 4; i++)
        {
            output.Pos = mul(float4(vVerts[i], 1.0), view_proj);
            output.ScreenTex = output.Pos.xy / output.Pos.w; // screenspace coordinates for the lookup into the depth buffer
            output.Tex = texCoord[i];
            output.Depth = output.Pos.zw;
            SpriteStream.Append(output);
        }
        SpriteStream.RestartStrip();
    }
}

float4 PSParticle(PSParticleIn input) : SV_TARGET
{
    float depthFade = 1.0f;
    float4 particleSample = txParticleVolumeAtlas.SampleLevel(samLinear, float3(input.Tex, input.TexParam), 0);

    if (input.particleColor.r > 0.0)    // if gradient color > 0, use it
        particleSample.rgb *= input.particleColor.rgb;

    switch (input.Tech)
    {
        case 1://shockwave
            particleSample.a *= 1.0f - input.Life;
            break;
        case 2: //frags
            particleSample.a *= 1.0f - input.Life;
            break;
        default:
            particleSample.a *= 1.0f - input.Life;
            CalcSoftParticles(depthFade, input.ScreenTex, input.Depth);
            particleSample.a *= depthFade;
            break;
    }

    return CalcControlAdditiveBlend(particleSample, input.Burn);
}

//  TECHNIQUES
//------------------------------------------------------
technique11 Particle
{
    pass p0
    {
        SetVertexShader(CompileShader(vs_5_0, VSParticle()));
        SetGeometryShader(CompileShader(gs_5_0, GSParticle()));
        SetPixelShader(CompileShader(ps_5_0, PSParticle()));
        SetBlendState(ControlledAdditiveBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
        SetRasterizerState(CullOn);
    }
}

//  GS METHODS
//------------------------------------------------------
void GS_ShockWave(point GSParticleIn input[1], inout float2 texCoord[4], inout float3 vVerts[4], in float particleColorAlpha)
{
    float3 vUp;
    float3 vRight;
    float3 vPlaneNormal = float3(0.0, -0.1, 0.0001);
    CalcRightAndUpCoordShift(vRight, vUp, vPlaneNormal);
    CalcParticleSizeByGradientAlpha(vRight, vUp, particleColorAlpha, input[0].MaxSize);
    CalcParticleVertexPos(vVerts, vRight, vUp, input[0].Pos);
    CalcFrameCoordInTexAtlas(texCoord, input[0].TexParam, input[0].Frame);
}

void GS_Default(point GSParticleIn input[1], inout float2 texCoord[4], inout float3 vVerts[4], in float particleColorAlpha)
{
    float3 vUp;
    float3 vRight;
    float3 vPlaneNormal = normalize(input[0].Pos.xyz - CameraPos);
    CalcRightAndUpCoordShift(vRight, vUp, vPlaneNormal);
    CalcParticleSizeByGradientAlpha(vRight, vUp, particleColorAlpha, input[0].MaxSize);
    CalcParticleVertexPos(vVerts, vRight, vUp, input[0].Pos);
    CalcFrameCoordInTexAtlas(texCoord, input[0].TexParam, input[0].Frame);
}

void CalcRightAndUpCoordShift(out float3 vRight, out float3 vUp, in float3 vPlaneNormal)
{
    vRight = normalize(cross(vPlaneNormal, CameraUp));
    vUp = normalize(cross(vRight, vPlaneNormal));
}

void CalcParticleSizeByGradientAlpha(inout float3 vRight, inout float3 vUp, in float particleColorAlpha, in float MaxSize)
{
    vRight *= particleColorAlpha * MaxSize;
    vRight *= 0.5f;
    vUp *= particleColorAlpha * MaxSize;
    vUp *= 0.5f;
}

void RotateParticle(inout float3 vRight, inout float3 vUp, in float Life)
{
    float s, c;
    sincos(3.0 * Life, s, c);
    float3 vRightNew = c * vRight - s * vUp;
    float3 vUpNew = s * vRight + c * vUp;
    vRight = vRightNew;
    vUp = vUpNew;
}

void CalcParticleVertexPos(out float3 vVerts[4], in float3 vRight, in float3 vUp, in float3 Pos)
{
    vVerts[0] = Pos.xyz + vRight + vUp;
    vVerts[1] = Pos.xyz - vRight + vUp;
    vVerts[2] = Pos.xyz + vRight - vUp;
    vVerts[3] = Pos.xyz - vRight - vUp;
}

void CalcFrameCoordInTexAtlas(out float2 texCoord[4], in float3 TexParam, in float Frame)
{
    float texSizeX = 1 / TexParam.r;
    float texSizeY = 1 / TexParam.g;
    float2 vIndex = float2(floor(fmod(Frame, TexParam.r)), floor(Frame / TexParam.g));
    float4 vTexCoords = float4(vIndex.x * texSizeX, vIndex.y * texSizeY, (vIndex.x + 1) * texSizeX, (vIndex.y + 1) * texSizeY);
    texCoord[0] = float2(vTexCoords.x, vTexCoords.y);
    texCoord[1] = float2(vTexCoords.z, vTexCoords.y);
    texCoord[2] = float2(vTexCoords.x, vTexCoords.w);
    texCoord[3] = float2(vTexCoords.z, vTexCoords.w);
}

//  PS METHODS
//------------------------------------------------------
void CalcSoftParticles(inout float depthFade, in float2 ScreenTex, in float2 Depth)
{
    float2 screenTex = 0.5*(ScreenTex + float2(1, 1));
    screenTex.y = 1 - screenTex.y;

    float particleDepth = Depth.x;
    particleDepth /= Depth.y;

    float depthSample;
    depthSample = txDepth.Sample(samPoint, screenTex);

    float4 depthViewSample = mul(float4(ScreenTex, depthSample, 1), InvProjection);
    float4 depthViewParticle = mul(float4(ScreenTex, particleDepth, 1), InvProjection);
    float depthDiff = depthViewSample.z / depthViewSample.w - depthViewParticle.z / depthViewParticle.w;

    if (depthDiff < 0)
        discard;

    depthFade = saturate(depthDiff / 1.0f);
}


float4 CalcControlAdditiveBlend(in float4 particleSample, in float Burn)
{
    float3 testCol = particleSample.rgb * particleSample.a;
    float alpha = particleSample.a * (1.0f - Burn);
    return float4(testCol.rgb, alpha);
}

#endif // __PARTICLES_EFFECT__