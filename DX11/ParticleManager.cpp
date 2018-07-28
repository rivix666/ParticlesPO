#include "stdafx.h"
#include "ParticleManager.h"

#include <utility> 
#include <fstream>
#include <PhysX/PxPhysicsAPI.h>
#include <d3d11.h>
#include <d3dx11.h>
#include "d3dx/d3dx11effect.h"

#include "DxRenderer.h"

ParticleManager::~ParticleManager()
{
    ReleaseAll();
}

const UINT& ParticleManager::CalcCurBuffSize()
{
    float tmp = (float)m_ParticlesData->m_iCPUFree / (float)BITONIC_BLOCK_SIZE;
    m_CurBuffSize = (UINT)ceil(tmp) * BITONIC_BLOCK_SIZE;
    return m_CurBuffSize;
}

const UINT& ParticleManager::GetCurBuffSize() const
{
    return m_CurBuffSize;
}

//  INIT METHODS
//-------------------------------------------------------------------
bool ParticleManager::Init(ID3DX11Effect* g_pEffect)
{
    bool bSuccess = true;
    InitLayoutAndTechniques(g_pEffect);
    bSuccess &= InitParticleBuffers(g_pEffect);
    bSuccess &= InitParticleViews(g_pEffect);
    bSuccess &= InitParticleTexMan(g_pEffect);
    return bSuccess;
}

bool ParticleManager::InitParticleBuffers(ID3DX11Effect* g_pEffect)
{
    m_ParticlesData = new ParticleBufferData(PARTICLE_BUFF_SIZE);

    // Create Vertex Buffer
    D3D11_BUFFER_DESC vbdesc;
    vbdesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbdesc.ByteWidth = PARTICLE_BUFF_SIZE * sizeof(ParticleData);
    vbdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    vbdesc.MiscFlags = 0;
    vbdesc.Usage = D3D11_USAGE_DYNAMIC;
    if (FAILED(g_Engine->Renderer()->GetDevice()->CreateBuffer(&vbdesc, nullptr, &m_ParticleVertexBuffer)))
        return utils::FatalError(L"Could not create vertex buffer!");

    // Create Pos Buffer
    D3D11_BUFFER_DESC dbdesc;
    dbdesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    dbdesc.ByteWidth = PARTICLE_BUFF_SIZE * sizeof(XMFLOAT3);
    dbdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    dbdesc.Usage = D3D11_USAGE_DYNAMIC;
    dbdesc.StructureByteStride = sizeof(XMFLOAT3);
    dbdesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    if (FAILED(g_Engine->Renderer()->GetDevice()->CreateBuffer(&dbdesc, nullptr, &m_ParticlePosBuffer)))
        return utils::FatalError(L"Could not create particle data buffer!");

    // Create Index Buffer
    m_ParticlesIndices = new DWORD[PARTICLE_BUFF_SIZE]; // #P_SORT to bedzie niepotrzebne jak sortowanie bedzie dzialalo na indexach, bo bedzie sie odwiezac co klatke
    for (DWORD i = 0; i < PARTICLE_BUFF_SIZE; i++)
        m_ParticlesIndices[i] = i;

    D3D11_BUFFER_DESC ibdesc;
    ibdesc.Usage = D3D11_USAGE_DYNAMIC;
    ibdesc.ByteWidth = sizeof(DWORD) * PARTICLE_BUFF_SIZE;
    ibdesc.BindFlags = D3D11_BIND_INDEX_BUFFER | D3D11_BIND_SHADER_RESOURCE;
    ibdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    ibdesc.MiscFlags = 0;
    ibdesc.StructureByteStride = sizeof(DWORD);
    D3D11_SUBRESOURCE_DATA iinitData;
    iinitData.pSysMem = m_ParticlesIndices;
    if (FAILED(g_Engine->Renderer()->GetDevice()->CreateBuffer(&ibdesc, &iinitData, &m_ParticleIndexBuffer))) 
        return utils::FatalError(L"Could not create index buffer!");

    // Create Index compute shader buffers
    D3D11_BUFFER_DESC iibdesc;
    iibdesc.Usage = D3D11_USAGE_DYNAMIC;
    iibdesc.ByteWidth = sizeof(DWORD) * PARTICLE_BUFF_SIZE;
    iibdesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    iibdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    iibdesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    iibdesc.StructureByteStride = sizeof(DWORD);
    
//      static DWORD kaka[PARTICLE_BUFF_SIZE];
//      for (int i = 0, j = PARTICLE_BUFF_SIZE; i < PARTICLE_BUFF_SIZE; i++, j--)
//          kaka[i] = j;

    static DWORD kaka[PARTICLE_BUFF_SIZE];
    for (int i = 0; i < PARTICLE_BUFF_SIZE; i++)
         kaka[i] = i;

    /*#TEST*/
    D3D11_SUBRESOURCE_DATA iinitData2;
    iinitData2.pSysMem = &kaka[0];

    if (FAILED(g_Engine->Renderer()->GetDevice()->CreateBuffer(&iibdesc, &iinitData2 /*#TEST*/, &m_IndexInputBuffer)))
        return utils::FatalError(L"Could not create index cs input buffer!");

    D3D11_BUFFER_DESC iobdesc;
    iobdesc.Usage = D3D11_USAGE_DEFAULT;
    iobdesc.ByteWidth = sizeof(DWORD) * PARTICLE_BUFF_SIZE;
    iobdesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
    iobdesc.CPUAccessFlags = 0;
    iobdesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    iobdesc.StructureByteStride = sizeof(DWORD);
    if (FAILED(g_Engine->Renderer()->GetDevice()->CreateBuffer(&iobdesc, nullptr, &m_IndexOutputBuffer)))
        return utils::FatalError(L"Could not create index cs output buffer!");
    
#if defined(DEBUG) || defined(_DEBUG)  
    iobdesc.Usage = D3D11_USAGE_STAGING;
    iobdesc.BindFlags = 0;
    iobdesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    if (FAILED(g_Engine->Renderer()->GetDevice()->CreateBuffer(&iobdesc, nullptr, &m_IndexOutputDebugBuffer)))
        return utils::FatalError(L"Could not create index cs output buffer!");
#endif

    return true;
}

bool ParticleManager::InitParticleViews(ID3DX11Effect* g_pEffect)
{
    if (m_ParticlesData->m_iCPUFree > (BITONIC_BLOCK_SIZE * 2))  // #P_SORT potrzebne?
        return true;

    // Init Particle Pos SRV
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
    srvDesc.BufferEx.FirstElement = 0;
    srvDesc.BufferEx.Flags = 0;
    srvDesc.BufferEx.NumElements = PARTICLE_BUFF_SIZE;
    if (FAILED(g_Engine->Renderer()->GetDevice()->CreateShaderResourceView(m_ParticlePosBuffer, &srvDesc, &m_ParticlePosSRV)))
        return utils::FatalError(L"Could not create particles data buffer SRV");

    // Init Particle Data Shader Variables
    m_ParticleDataShaderVariable = g_pEffect->GetVariableByName("ParticlePosBuff")->AsShaderResource();
    m_ParticlesNumShaderVariable = g_pEffect->GetVariableByName("ParticlesNum")->AsScalar();

    // Init Sort SRV
    D3D11_SHADER_RESOURCE_VIEW_DESC srvSortDesc;
    srvSortDesc.Format = DXGI_FORMAT_UNKNOWN; // #TEST dla indexinputbuffer unknown, dla indexbuffer DXGI_FORMAT_R32_UINT;
    srvSortDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
    srvSortDesc.BufferEx.FirstElement = 0;
    srvSortDesc.BufferEx.Flags = 0;
    srvSortDesc.BufferEx.NumElements = PARTICLE_BUFF_SIZE;
    if (FAILED(g_Engine->Renderer()->GetDevice()->CreateShaderResourceView(m_IndexInputBuffer/*#TEST normalnie ma byc index buffer*/, &srvSortDesc, &m_IndexInputSRV)))
        return utils::FatalError(L"Could not create sort input buffer SRV");

    // Init Sort UAV
    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
    uavDesc.Format = DXGI_FORMAT_UNKNOWN;
    uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.Flags = 0;
    uavDesc.Buffer.NumElements = PARTICLE_BUFF_SIZE;
    if (FAILED(g_Engine->Renderer()->GetDevice()->CreateUnorderedAccessView(m_IndexOutputBuffer, &uavDesc, &m_IndexOutputUAV)))
        return utils::FatalError(L"Could not create sort output buffer UAV");

    // Init Sort Shader Variables
    m_IndexInputShaderVariable = g_pEffect->GetVariableByName("IdxIn")->AsShaderResource();
    m_IndexOutputShaderVariable = g_pEffect->GetVariableByName("IdxOut")->AsUnorderedAccessView();

    m_CamNormPosShaderVariable = g_pEffect->GetVariableByName("CamNormPos")->AsVector();
    m_SortLevelShaderVariable = g_pEffect->GetVariableByName("SortLevel")->AsScalar();
    m_SortLevelMaskShaderVariable = g_pEffect->GetVariableByName("SortLevelMask")->AsScalar();
    m_SortWidthShaderVariable = g_pEffect->GetVariableByName("SortWidth")->AsScalar();
    m_SortHeightShaderVariable = g_pEffect->GetVariableByName("SortHeight")->AsScalar();

    return true;
}

bool ParticleManager::InitParticleTexMan(ID3DX11Effect* g_pEffect)
{
    g_TexMan = particleTexManager::getInstance();
    return g_TexMan->init(g_Engine->Renderer()->GetDevice(), g_pEffect);
}

void ParticleManager::SetEffectVariables()
{
    //g_pRotateParticle->AsScalar()->SetBool(m_bRotateParticles);
}

void ParticleManager::InitLayoutAndTechniques(ID3DX11Effect* g_pEffect)
{
    inputLayouts *inputLay = inputLayouts::getInstance(g_Engine->Renderer()->GetDevice(), g_pEffect);
    m_ParticleTechnique = inputLay->getTechnique(eInputLayouts::PARTICLE);
    m_ParticleLayout = inputLay->getLayout(eInputLayouts::PARTICLE);
    // m_ParticleTechnique->GetDesc(&m_TechniqueDesc); //#DX11
}

//  GPU SORT
//-------------------------------------------------------------------
// #WAZNE_BARDZO okazalo sie ze nawet przy renderwaniu wszystkiego miga. 
// Zauwazylem ze do migania dochodzi tylko podczas emisji nowych particli, badz wyemitowania kilku rodzajow na raz.
// Gdy znikna inne rodzaje particli albo przestana byc emitowane ustaje tez miganie.
void ParticleManager::SortOnGPU()
{
    CLogProfile s_prof("SortOnGPU");
    g_Engine->Renderer()->GetDeviceCtx()->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0); // #P_SORT czy potrzebne?

    auto sort_tech = inputLayouts::getInstance(nullptr, nullptr)->getTechnique(PARTICLE_SORT);

    // #TEST
//     {
//         CLogProfile test("CopyResourceGPU");
//         g_Engine->Renderer()->GetDeviceCtx()->CopyResource(m_IndexOutputBuffer, m_IndexInputBuffer);
//     }

    {
        CLogProfile test("CopyPartOfResourceGPU");
        D3D11_BOX box;
        box.left = 0;
        box.top = 0;
        box.front = 0;
        box.right = sizeof(DWORD) * GetCurBuffSize();
        box.bottom = 1;
        box.back = 1;
        g_Engine->Renderer()->GetDeviceCtx()->CopySubresourceRegion(m_IndexOutputBuffer, 0, 0, 0, 0, m_IndexInputBuffer, 0, &box);
    }

    {
        CLogProfile test1("SetResourcesGPU");
        m_IndexInputShaderVariable->SetResource(m_IndexInputSRV);
        m_IndexOutputShaderVariable->SetUnorderedAccessView(m_IndexOutputUAV);
        m_CamNormPosShaderVariable->SetFloatVector(reinterpret_cast<float*>(&m_CamNormalizedPos)); // #REDESIGN to moze byc przeniesione do kamery i ustawiane co update z kamery a nie co 
    }

    {
        CLogProfile test2("DispatchGPU");
        D3DX11_TECHNIQUE_DESC techDesc;
        sort_tech->GetDesc(&techDesc);
        for (UINT p = 0; p < techDesc.Passes; ++p)
        {
            ID3DX11EffectPass* pass = sort_tech->GetPassByIndex(p);
            pass->Apply(0, g_Engine->Renderer()->GetDeviceCtx());
            g_Engine->Renderer()->GetDeviceCtx()->Dispatch((GetCurBuffSize() / BITONIC_BLOCK_SIZE) / 2, 1, 1);
        }
    }

    {
        CLogProfile test3("NullShaderResourcesGPU");
//         ID3D11ShaderResourceView* nullSRV[1] = { 0 };
//         g_Engine->Renderer()->GetDeviceCtx()->CSSetShaderResources(0, 1, nullSRV);
// 
//         ID3D11UnorderedAccessView* nullUAV[1] = { 0 };
//         g_Engine->Renderer()->GetDeviceCtx()->CSSetUnorderedAccessViews(0, 1, nullUAV, 0);
// 
//         // Disable CS
//         g_Engine->Renderer()->GetDeviceCtx()->CSSetShader(0, 0, 0);
    }

//     {
//         CLogProfile test4("CopyIntoIndexBuffer");
//         g_Engine->Renderer()->GetDeviceCtx()->CopyResource(m_ParticleIndexBuffer, m_IndexOutputBuffer);
//     }

    {
        CLogProfile test("CopyPartIntoIndexBufferGPU");
        D3D11_BOX box;
        box.left = 0;
        box.top = 0;
        box.front = 0;
        box.right = sizeof(DWORD) * GetCurBuffSize();
        box.bottom = 1;
        box.back = 1;
        g_Engine->Renderer()->GetDeviceCtx()->CopySubresourceRegion(m_ParticleIndexBuffer, 0, 0, 0, 0, m_IndexOutputBuffer, 0, &box);
    }

#if defined(DEBUG) || defined(_DEBUG)  
//     g_Engine->Renderer()->GetDeviceCtx()->CopyResource(m_IndexOutputDebugBuffer, m_IndexOutputBuffer);
// 
//     D3D11_MAPPED_SUBRESOURCE mappedData;
//     g_Engine->Renderer()->GetDeviceCtx()->Map(m_IndexOutputDebugBuffer, 0, D3D11_MAP_READ, 0, &mappedData);
//     DWORD* dataView = reinterpret_cast<DWORD*>(mappedData.pData);
//     {
//         CDebugFileLog log("SortDebug.txt", std::ofstream::/*trunc*/app);
//         DWORD previous = 0;
//         int series_counter = 0;
//         for (int i = 0; i < m_ParticlesData->m_iCPUFree; i++)
//         {
//             for (int j = i + 1; j < m_ParticlesData->m_iCPUFree; j++)
//             {
//                 if (dataView[i] == dataView[j])
//                 {
//                     LogD("GPU Sort Debug ");
//                     LogD(dataView[i]);
//                     LogD(" ");
//                     LogD(m_ParticlesData->m_iCPUFree);
//                     LogD(" ");
//                     LogD(series_counter++);
//                     LogD("\n");
//                 }
//             }
//         }
//     }
//     g_Engine->Renderer()->GetDeviceCtx()->Unmap(m_IndexOutputDebugBuffer, 0);
#endif

    m_IndexInputShaderVariable->SetResource(nullptr);
}

//  MAIN PUBLIC METHODS
//-------------------------------------------------------------------
int ParticleManager::CreateExplosion(XMFLOAT3 pos, ExplosionTypes ty)
{
    explosionInterface *ex;
    switch (ty)
    {
    case Explosion1:
        ex = new explosion1(m_ParticlesData, pos);
        break;
    case FireWorks:
        ex = new fireWorks(m_ParticlesData, pos);
        break;
    default:
        ex = new explosion1(m_ParticlesData, pos);
    }

    ExplosionsVec.push_back(ex);
    return ExplosionsVec.size() - 1;
}

int ParticleManager::CreateExplosion(explosionInterface *ex)
{
    ExplosionsVec.push_back(ex);
    return ExplosionsVec.size() - 1;
}

int ParticleManager::CreateEmitter(XMFLOAT3 pos, ParticleTypes ty)
{
    emitterInterface *emit;
    switch (ty)
    {
    case Smoke:
        emit = new emitterSmoke(m_ParticlesData);
        break;
    case Flame:
        emit = new emitterFlame(m_ParticlesData);
        break;
    case ShockWave:
        emit = new emitterShockWave(m_ParticlesData);
        break;
    case Flash:
        emit = new emitterFlash(m_ParticlesData);
        break;
    case Frags:
        emit = new emitterFrags(m_ParticlesData);
        break;
    case Spark:
        emit = new emitterSpark(m_ParticlesData);
        break;
    case Lightning:
        emit = new emitterLightning(m_ParticlesData);
        break;
    default:
        emit = new emitterSmoke(m_ParticlesData);
    }
    emit->setEmitterPosition(pos);
    EmittersVec.push_back(emit);
    return EmittersVec.size() - 1;
}

int ParticleManager::CreateEmitter(emitterInterface *emit)
{
    EmittersVec.push_back(emit);
    return EmittersVec.size() - 1;
}

//  MAIN METHODS
//-------------------------------------------------------------------
void ParticleManager::EmitExplosion(int id)
{
    ExplosionsVec[id]->Emit();
}

void ParticleManager::EmitParticles(int id, bool randomiseVelocities) // #REDESIGN dodac mechanizm ktory emituje wybrana ilsoc czasteczek ans ekunde bo narazie jest to per ilosc ramek
{
    EmittersVec[id]->Emit(randomiseVelocities);
}

void ParticleManager::EmitAllExplosions()
{
    for (int i = 0; i < ExplosionsVec.size(); i++)
        ExplosionsVec[i]->Emit();
}

void ParticleManager::EmitAllParticles()
{
    for (int i = 0; i < EmittersVec.size(); i++)
        EmittersVec[i]->Emit();
}

void ParticleManager::Simulate()
{
    g_Engine->GetTimer().getElapsedTime();
    for (int i = 0; i < EmittersVec.size(); i++)
        EmittersVec[i]->Simulate();   
    for (int i = 0; i < ExplosionsVec.size(); i++)
        ExplosionsVec[i]->Simulate();
}

void ParticleManager::PreRender()
{
    if (m_ParticlesData->m_iCPUFree > 0)
    {
        CalcCurBuffSize();
        MapParticles(GetCurBuffSize());

        {
            m_ParticlesNumShaderVariable->SetInt(/*m_ParticlesData->m_iCPUFree*/GetCurBuffSize()); //#FOO
            m_ParticleDataShaderVariable->SetResource(m_ParticlePosSRV);
        }

        if (g_Engine->m_SortOnGPU)
            g_Engine->GetParticleManager()->SortOnGPU();
        else
            g_Engine->GetParticleManager()->SortOnCPU();

        PrepareVertexBuffer();
        PrepareIndexBuffer(); // #P_SORT tylko przy sortowaniu gpu
                              // #P_SORT ew. zrobic by zwykly sort po cpu tez uzywal indeksow
    }
}

void ParticleManager::Render()
{
    if (m_ParticlesData->m_iCPUFree > 0)
    {
        g_Engine->Renderer()->GetDeviceCtx()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
        DrawParticles(/*m_ParticlesData->m_iCPUFree*/GetCurBuffSize(), 0); //#FOO
    }
}

void ParticleManager::PostRender()
{

}

//  OTHER METHODS
//-------------------------------------------------------------------
void ParticleManager::MapParticles(int numberOfParticles)
{
    // Map particles positions in Vertex Buffer
    D3D11_MAPPED_SUBRESOURCE vsubsrc;
    /*vsubsrc.pData = malloc(m_ParticlesData->m_iCPUFree * sizeof(ParticleData));*/
    g_Engine->Renderer()->GetDeviceCtx()->Map(m_ParticleVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &vsubsrc);
    memcpy(vsubsrc.pData, &m_ParticlesData->CPU_Particle_Data[0], numberOfParticles * sizeof(ParticleData));
    g_Engine->Renderer()->GetDeviceCtx()->Unmap(m_ParticleVertexBuffer, 0);

    // Map particles data in Shader Resource
    D3D11_MAPPED_SUBRESOURCE dsubsrc;
    /*dsubsrc.pData = malloc(m_ParticlesData->m_iCPUFree * sizeof(ParticleData));*/
    g_Engine->Renderer()->GetDeviceCtx()->Map(m_ParticlePosBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &dsubsrc);
    memcpy(dsubsrc.pData, &m_ParticlesData->CPU_Particle_Pos[0], numberOfParticles * sizeof(XMFLOAT3));
    g_Engine->Renderer()->GetDeviceCtx()->Unmap(m_ParticlePosBuffer, 0);
}

void ParticleManager::PrepareVertexBuffer()
{
    g_Engine->Renderer()->GetDeviceCtx()->IASetInputLayout(m_ParticleLayout);
    ID3D11Buffer *pBuffers[1] = { m_ParticleVertexBuffer };
    UINT stride = sizeof(ParticleData);
    UINT offset = 0;
    g_Engine->Renderer()->GetDeviceCtx()->IASetVertexBuffers(0, 1, pBuffers, &stride, &offset);
}

void ParticleManager::PrepareIndexBuffer()
{
    g_Engine->Renderer()->GetDeviceCtx()->IASetIndexBuffer(m_ParticleIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
}

void ParticleManager::DrawParticles(UINT VertexCount, UINT StartVertexLocation)
{
    D3DX11_TECHNIQUE_DESC techDesc;
    m_ParticleTechnique->GetDesc(&techDesc);   
    for (UINT p = 0; p < techDesc.Passes; ++p)
    {
        m_ParticleTechnique->GetPassByIndex(p)->Apply(0, g_Engine->Renderer()->GetDeviceCtx());

        //g_pD3DDevice->Draw(VertexCount, StartVertexLocation); // #P_SORT to powinno leciec na sortowaniu cpu

        g_Engine->Renderer()->GetDeviceCtx()->DrawIndexed(VertexCount, 0, 0); // #P_SORT a to gpu
    }
}

void ParticleManager::SortOnCPU() //#P_SORT chyba zepsuje sie po rozlozeniu na dwa vektory
{
    CLogProfile s_prof("SortOnCPU");
    static int loopOffset = 0;  
    if (m_ParticlesData->m_iCPUFree > 1)
    {
        double *depths = new double[m_ParticlesData->m_iCPUFree];
        for (int i = 0; i < m_ParticlesData->m_iCPUFree; i++) 
            depths[i] = XMVector3Dot(XMLoadFloat3(&m_CamNormalizedPos), XMLoadFloat3(&m_ParticlesData->CPU_Particle_Pos[i])).m128_f32[0];

        if (loopOffset > 20)
        {
            qsort(depths, 0, m_ParticlesData->m_iCPUFree - 1);
            loopOffset = 0;
        }
        else
            bubbleSort(depths);

        delete depths;
    }
    loopOffset++;
}

void ParticleManager::bubbleSort(double *depths)
{
    int dep = m_ParticlesData->m_iCPUFree - 1;
    int i;
    if (m_ParticlesData->m_iCPUFree > 201)
        i = m_ParticlesData->m_iCPUFree - 200;
    else
        i = 0;
    for (i; i < dep; i++)
    {
        int m = i + 1;
        if (depths[i] > depths[m])
        {
            std::swap(m_ParticlesData->CPU_Particle_Pos[i], m_ParticlesData->CPU_Particle_Pos[m]);
            std::swap(m_ParticlesData->CPU_Particle_Data[i], m_ParticlesData->CPU_Particle_Data[m]);
            std::swap(m_ParticlesData->CPU2Px[i], m_ParticlesData->CPU2Px[m]);
            emitterInterface::g_pAllEmitters[m_ParticlesData->CPU2Px[i].EmitId]->Px2CPU[m_ParticlesData->CPU2Px[i].PartId] = i;
            emitterInterface::g_pAllEmitters[m_ParticlesData->CPU2Px[m].EmitId]->Px2CPU[m_ParticlesData->CPU2Px[m].PartId] = m;
            std::swap(depths[i], depths[m]);
        }
    }
}

bool ParticleManager::tp(double a, double b)
{
    return (a < b);
}

void ParticleManager::sort1(double *depths)
{
    int g, i, j;
    double t;
    for (g = m_ParticlesData->m_iCPUFree / 2; g > 0; g /= 2)
        for (i = m_ParticlesData->m_iCPUFree - g - 1; i >= 0; i--)
        {
            t = depths[i];
            for (j = i + g; (j < m_ParticlesData->m_iCPUFree) && !tp(t, depths[j]); j += g)
            {
                std::swap(m_ParticlesData->CPU_Particle_Pos[j - g], m_ParticlesData->CPU_Particle_Pos[j]);
                std::swap(m_ParticlesData->CPU_Particle_Data[j - g], m_ParticlesData->CPU_Particle_Data[j]);
                std::swap(m_ParticlesData->CPU2Px[j - g], m_ParticlesData->CPU2Px[j]);
                emitterInterface::g_pAllEmitters[m_ParticlesData->CPU2Px[j - g].EmitId]->Px2CPU[m_ParticlesData->CPU2Px[j - g].PartId] = j - g;
                emitterInterface::g_pAllEmitters[m_ParticlesData->CPU2Px[j].EmitId]->Px2CPU[m_ParticlesData->CPU2Px[j].PartId] = j;
                std::swap(depths[j - g], depths[j]);
            }

            depths[j - g] = t;
        };

}

void ParticleManager::insertionSort(double *depths)
{
    int dep;
    if (m_ParticlesData->m_iCPUFree > 201)
        dep = m_ParticlesData->m_iCPUFree - 200;
    else
        dep = 0;

    for (int i = m_ParticlesData->m_iCPUFree - 2; i > dep; i--)
    {
        int m = i + 1;
        if (depths[i] > depths[m])
        {
            std::swap(m_ParticlesData->CPU_Particle_Pos[i], m_ParticlesData->CPU_Particle_Pos[m]);
            std::swap(m_ParticlesData->CPU_Particle_Data[i], m_ParticlesData->CPU_Particle_Data[m]);
            std::swap(m_ParticlesData->CPU2Px[i], m_ParticlesData->CPU2Px[m]);
            emitterInterface::g_pAllEmitters[m_ParticlesData->CPU2Px[i].EmitId]->Px2CPU[m_ParticlesData->CPU2Px[i].PartId] = i;
            emitterInterface::g_pAllEmitters[m_ParticlesData->CPU2Px[m].EmitId]->Px2CPU[m_ParticlesData->CPU2Px[m].PartId] = m;
            std::swap(depths[i], depths[m]);
        }
    }
}

void ParticleManager::qsort(double *depths, int left, int right)
{
    int i = left, j = right;
    int index;
    float x = depths[(left + right) / 2];

    //  partition
    do
    {
        while (depths[i] < x) i++;
        while (depths[j] > x) j--;
        if (i <= j)
        {
            // if (g_pParticlesData->CPU_Particle[i].Life < 0 || g_pParticlesData->CPU_Particle[j].Life < 0) continue;
            std::swap(m_ParticlesData->CPU_Particle_Pos[i], m_ParticlesData->CPU_Particle_Pos[j]);
            std::swap(m_ParticlesData->CPU_Particle_Data[i], m_ParticlesData->CPU_Particle_Data[j]);
            std::swap(m_ParticlesData->CPU2Px[i], m_ParticlesData->CPU2Px[j]);
            emitterInterface::g_pAllEmitters[m_ParticlesData->CPU2Px[i].EmitId]->Px2CPU[m_ParticlesData->CPU2Px[i].PartId] = i;
            emitterInterface::g_pAllEmitters[m_ParticlesData->CPU2Px[j].EmitId]->Px2CPU[m_ParticlesData->CPU2Px[j].PartId] = j;
            std::swap(depths[i], depths[j]);
            i++; j--;
        }
    } while (i <= j);

    if (left < j) qsort(depths, left, j);
    if (i < right) qsort(depths, i, right);
}

void ParticleManager::ReleaseEmitters()
{
    for each(emitterInterface *emit in EmittersVec)
        delete emit;
    EmittersVec.clear();
}

void ParticleManager::ReleaseExplosions()
{
    for each(explosionInterface *ex in ExplosionsVec)
        delete ex;
    ExplosionsVec.clear();
}

void ParticleManager::ReleaseBuffers()
{
    SAFE_RELEASE(m_ParticleVertexBuffer);
    SAFE_RELEASE(m_ParticleIndexBuffer);
    SAFE_RELEASE(m_IndexOutputBuffer);
    SAFE_RELEASE(m_IndexInputBuffer);
    SAFE_RELEASE(m_IndexInputSRV);
    SAFE_RELEASE(m_IndexOutputUAV);
    SAFE_RELEASE(m_ParticlePosSRV);
    SAFE_DELETE(m_ParticlesData);
    SAFE_DELETE_ARR(m_ParticlesIndices);
#if defined(DEBUG) || defined(_DEBUG)
    SAFE_RELEASE(m_IndexOutputDebugBuffer);
#endif
}

void ParticleManager::ReleaseVariables()
{
    delete g_TexMan;
}

void ParticleManager::ReleaseAll()
{
    ReleaseEmitters();
    ReleaseExplosions();
    ReleaseBuffers();
    ReleaseVariables();
}

//  GETTERS & SETTERS
//-------------------------------------------------------------------
void ParticleManager::SetCamPosAndNormalizedPos(XMFLOAT3 camPos) //#REDESIGN to w kamerze powinno byc
{
    m_CamPos = camPos;
    XMStoreFloat3(&m_CamNormalizedPos, XMVector3Normalize(XMLoadFloat3(&camPos)));
}

//  DEPRECATED
//-------------------------------------------------------------------
//void ParticleManager::RotateParticles(bool rotate)
//{
//    m_bRotateParticles = rotate;
//    g_pRotateParticle->AsScalar()->SetBool(m_bRotateParticles);
//}
//
//bool ParticleManager::AreParticlesRotated() { return m_bRotateParticles; }