#pragma once
#include "particleTexManager.h" //#REDESIGN wywalic niepotrzebne includy do cpp
#include "Structs.h"
#include "inputLayouts.h"
#include "emitterSmoke.h"
#include "emitterFlame.h"
#include "emitterShockWave.h"
#include "emitterFlash.h"
#include "emitterFrags.h"
#include "emitterSpark.h"
#include "emitterLightning.h"
#include "explosion1.h"
#include "fireWorks.h"

enum ParticleTypes
{
    Smoke,
    Flame,
    ShockWave,
    Flash,
    Frags,
    Spark,
    Lightning
};

enum ExplosionTypes
{
    Explosion1,
    FireWorks
};

namespace physx
{
    class PxScene;
    class PxPhysics;
}

class ID3D11Device;
class ID3D11Buffer;
class ID3D11InputLayout;
class ID3DX11EffectTechnique;

class ParticleManager
{
public:
    ParticleManager() = default;
    ~ParticleManager();

    const UINT& CalcCurBuffSize();
    const UINT& GetCurBuffSize() const;

    //  MAIN METHODS
    //------------------------------
    bool Init(ID3DX11Effect* g_pEffect);
    int  CreateExplosion(XMFLOAT3 pos, ExplosionTypes ty);
    int  CreateExplosion(explosionInterface *ex);
    int  CreateEmitter(XMFLOAT3 pos, ParticleTypes ty);
    int  CreateEmitter(emitterInterface *emit);
    void EmitExplosion(int id);
    void EmitParticles(int id, bool randomiseVelocities = true);
    void EmitAllExplosions();
    void EmitAllParticles();
    void SortOnCPU();

    void Simulate();

    void PreRender();   //#REDESIGN wyciagnac to do interfejsu
    void Render();      //#REDESIGN wyciagnac to do interfejsu
    void PostRender();  //#REDESIGN wyciagnac to do interfejsu

    // GPU SORT
    //------------------------------
    void SortOnGPU();


    //  OTHER METHODS
    //------------------------------
    void ReleaseEmitters();
    void ReleaseExplosions();
    void ReleaseBuffers();
    void ReleaseVariables();
    void ReleaseAll();

    //  GETTERS & SETTERS
    //------------------------------
    void SetCamPosAndNormalizedPos(XMFLOAT3 camPos);

    //  VARIABLES
    //------------------------------
    particleTexManager                 *g_TexMan = nullptr;
    ParticleBufferData                 *m_ParticlesData = nullptr;
    DWORD                              *m_ParticlesIndices = nullptr;

    //  CONTAINERS
    //------------------------------
    std::vector <emitterInterface*>     EmittersVec;
    std::vector <explosionInterface*>   ExplosionsVec;

private:
    //  INIT METHODS
    //------------------------------
    bool InitParticleTexMan(ID3DX11Effect* g_pEffect);
    bool InitParticleBuffers(ID3DX11Effect* g_pEffect);
    bool InitParticleViews(ID3DX11Effect* g_pEffect);
    void InitLayoutAndTechniques(ID3DX11Effect* g_pEffect);

    //  OTHER METHODS
    //------------------------------
    void MapParticles(int numberOfParticles);
    void PrepareVertexBuffer(); 
    void PrepareIndexBuffer();
    void SetEffectVariables();
    void DrawParticles(UINT VertexCount, UINT StartVertexLocation);
    void bubbleSort(double *depths);
    void qsort(double *depths, int left, int right); // #REDESIGN mega wolne, nie uzywac
    void sort1(double *depths);
    bool tp(double a, double b);
    void insertionSort(double *depths);

    XMFLOAT3                            m_CamPos;
    XMFLOAT3                            m_CamNormalizedPos;

    ID3D11InputLayout                  *m_ParticleLayout = nullptr;
    ID3DX11EffectTechnique             *m_ParticleTechnique = nullptr;

    ID3D11Buffer                       *m_ParticleVertexBuffer = nullptr;
    ID3D11Buffer                       *m_ParticlePosBuffer = nullptr;

    ID3D11Buffer                       *m_ParticleIndexBuffer = nullptr;
    ID3D11Buffer                       *m_IndexOutputBuffer = nullptr;
    ID3D11Buffer                       *m_IndexInputBuffer = nullptr;

#if defined(DEBUG) || defined(_DEBUG)  
    ID3D11Buffer                       *m_IndexOutputDebugBuffer = nullptr;
#endif

    ID3D11ShaderResourceView           *m_IndexInputSRV = nullptr;
    ID3D11UnorderedAccessView          *m_IndexOutputUAV = nullptr;
    ID3D11ShaderResourceView           *m_ParticlePosSRV = nullptr;

    ID3DX11EffectVectorVariable         *m_CamNormPosShaderVariable = nullptr; 
    ID3DX11EffectScalarVariable         *m_SortLevelShaderVariable = nullptr; // #REDESIGN te co so updatowane razem rpzerobic na const buffery (czytaj ComputeShaderSort11 z directx sample void SetConstants(...)
    ID3DX11EffectScalarVariable         *m_SortLevelMaskShaderVariable = nullptr;
    ID3DX11EffectScalarVariable         *m_SortWidthShaderVariable = nullptr;
    ID3DX11EffectScalarVariable         *m_SortHeightShaderVariable = nullptr;

    ID3DX11EffectScalarVariable         *m_ParticlesNumShaderVariable = nullptr;
    ID3DX11EffectShaderResourceVariable *m_ParticleDataShaderVariable = nullptr; // #REDESIGN moze faktycznie zrobic swoja klase do zarzadzania efektami
    ID3DX11EffectShaderResourceVariable *m_IndexInputShaderVariable = nullptr;
    ID3DX11EffectUnorderedAccessViewVariable *m_IndexOutputShaderVariable = nullptr;
    
    UINT m_CurBuffSize = 0;

    //  DEPRECATED
    //------------------------------
//public:
    // void RotateParticles(bool rotate);
    //bool AreParticlesRotated();
//private:
    //ID3DX11EffectVariable     		   *g_pRotateParticle = NULL;
    //bool                                m_bRotateParticles;
};

