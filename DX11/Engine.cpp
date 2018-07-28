#include "stdafx.h"
#include "Engine.h"

#include <d3dx11.h>
#include <d3d11.h>
#include <dxerr.h>

#include "DxRenderer.h"
#include "StaticObject.h"
#include "pxManager.h"
#include "ParticleManager.h"

CEngine::CEngine()
    : m_Hwnd(nullptr)
    , m_WindowWidth(0)
    , m_WindowHeight(0)
    , m_Renderer(nullptr)
    , m_PxMan(nullptr)
    , m_pParticleMan(nullptr)
    , m_PhysxPause(false)
    , m_ActEmittedId(-1)
    , m_ActRandomVel(true)
    , m_RestrictTo60FPS(false)
    , m_SortOnGPU(true)
{
    m_60FPSTimer.setFrequency(TimerType::Seconds);
    m_60FPSTimer.startTimer();
}

CEngine::~CEngine()
{
    Shutdown();
}

//	RENDER METHODS
//-----------------------------------------------------------
void CEngine::Frame()
{
    if (m_RestrictTo60FPS)
    {
        while (m_60FPSTimer.getElapsedTime() < 0.0166) {}
        m_60FPSTimer.startTimer();
    }

#if defined(DEBUG) || defined(_DEBUG)
    //EmitDebugEmmiters();
#endif

    g_Engine->UpdateScene();
    m_Timer.startTimer();
    m_Renderer->Render();
}

void CEngine::UpdateScene()
{
     EmitParticlesInLoop();
     GetCam()->Update();
     m_pParticleMan->SetCamPosAndNormalizedPos(reinterpret_cast<CDxCamera*>(GetCam())->CameraPosition()); //#REDESIGN jednak trzeba by zrobiæ w³asne macierze i vectory jak ma byc i vulkan
     if (!m_PhysxPause)
     {
         int count = m_PxMan->SimulatePhysX();
         for (int i = 0; i < count; i++)
         {
             m_pParticleMan->Simulate();
         }
     }
}

//	INIT METHODS
//////////////////////////////////////////////////////////////////////////
bool CEngine::Initialize(HWND hwnd, UINT windowW, UINT windowH, ERendererType type) 
{
    // #REDESIGN poprawic fajnie by bylo moc zonglowac hwnd also plynna zmiana rozdzialki
    m_Hwnd = hwnd;
    m_WindowWidth = windowW;
    m_WindowHeight = windowH;
    m_Renderer = (type == DX10) ? new CDxRenderer() : nullptr;
    if (!m_Renderer || !m_Renderer->Init())
    {
        Shutdown();
        return false;
    }

    srand(time(NULL));

    // register objects
    auto floor_plane = new CGStaticObject();
    floor_plane->Init(StaticObjType::Floor);
    m_Renderer->RegisterRenderObject(floor_plane);

    // init physx and aprticles
    InitPhysXAndParticles();
    CreateParticleEmittersAndExplosions();

#if defined(DEBUG) || defined(_DEBUG)
    CreateDebugEmmiters();
#endif

    return true;
}

bool CEngine::InitPhysXAndParticles()
{
     m_PxMan = pxManager::getInstance();
     m_pParticleMan = new ParticleManager();
     if (!m_pParticleMan->Init(m_Renderer->GetEffects()))
         return utils::FatalError(L"Init Particles Error!");
    return true;
}

void CEngine::CreateParticleEmittersAndExplosions()
{
    //EXPLOSIONS
    m_pParticleMan->CreateExplosion(XMFLOAT3(0.f, 4.5f, 0.f), ExplosionTypes::Explosion1);
    m_pParticleMan->CreateExplosion(XMFLOAT3(0.f, 0.1f, 0.f), ExplosionTypes::FireWorks);

    //EMITTERS
    m_pParticleMan->CreateEmitter(XMFLOAT3(0.f, 4.5f, 0.f), ParticleTypes::Smoke);
    m_pParticleMan->CreateEmitter(XMFLOAT3(0.f, 1.0f, 0.f), ParticleTypes::Flame);
    m_pParticleMan->CreateEmitter(XMFLOAT3(0.f, 1.0f, 0.f), ParticleTypes::ShockWave);
    m_pParticleMan->CreateEmitter(XMFLOAT3(0.f, 4.5f, 0.f), ParticleTypes::Flash);
    m_pParticleMan->CreateEmitter(XMFLOAT3(0.f, 4.5f, 0.f), ParticleTypes::Frags);
    m_pParticleMan->CreateEmitter(XMFLOAT3(0.f, 4.5f, 0.f), ParticleTypes::Spark);
    m_pParticleMan->CreateEmitter(XMFLOAT3(0.f, 4.5f, 0.f), ParticleTypes::Lightning);
}

//	OTHER METHODS
//-----------------------------------------------------------
void CEngine::EmitParticle(int id, bool randomiseVelocities)
{
    m_pParticleMan->EmitParticles(id, randomiseVelocities);
}

void CEngine::EmitExplosion(int id)
{
    m_pParticleMan->EmitExplosion(id);
}

void CEngine::EmitParticlesInLoop()
{
    static UINT loopOffset = 0;
    if (m_ActEmittedId == -1)
        return;
    if ((loopOffset & 1) == 0)
        EmitParticle(m_ActEmittedId, m_ActRandomVel);
    loopOffset++;
}

ICamera* CEngine::GetCam()
{
    return m_Renderer->GetCam();
}

bool CEngine::Shutdown()
{
    SAFE_DELETE(m_Renderer);
    SAFE_DELETE(m_PxMan);
    SAFE_DELETE(m_pParticleMan);

    return false;
}

#if defined(DEBUG) || (_DEBUG)
void CEngine::CreateDebugEmmiters()
{
    m_DebugEmitters.push_back(m_pParticleMan->CreateEmitter(XMFLOAT3(0.f, 3.f, 8.f), ParticleTypes::Smoke));
    m_DebugEmitters.push_back(m_pParticleMan->CreateEmitter(XMFLOAT3(0.f, 3.f, 4.f), ParticleTypes::Smoke));
    m_DebugEmitters.push_back(m_pParticleMan->CreateEmitter(XMFLOAT3(0.f, 3.f, 0.f), ParticleTypes::Smoke));
    m_DebugEmitters.push_back(m_pParticleMan->CreateEmitter(XMFLOAT3(0.f, 3.f, -4.f), ParticleTypes::Smoke));
    m_DebugEmitters.push_back(m_pParticleMan->CreateEmitter(XMFLOAT3(0.f, 3.f, -8.f), ParticleTypes::Smoke));

    m_DebugEmitters.push_back(m_pParticleMan->CreateEmitter(XMFLOAT3(0.f, 1.0f, 8.f), ParticleTypes::Flame));
    m_DebugEmitters.push_back(m_pParticleMan->CreateEmitter(XMFLOAT3(0.f, 1.0f, 4.f), ParticleTypes::Flame));
    m_DebugEmitters.push_back(m_pParticleMan->CreateEmitter(XMFLOAT3(0.f, 1.0f, 0.f), ParticleTypes::Flame));
    m_DebugEmitters.push_back(m_pParticleMan->CreateEmitter(XMFLOAT3(0.f, 1.0f, -4.f), ParticleTypes::Flame));
    m_DebugEmitters.push_back(m_pParticleMan->CreateEmitter(XMFLOAT3(0.f, 1.0f, -8.f), ParticleTypes::Flame));
}

void CEngine::EmitDebugEmmiters()
{
    static const double STEP_SIZE = 1.0f / 40.0f;
    static double dTimer = 0.0f;
    dTimer += GetTimer().getElapsedTime();

    while (dTimer > STEP_SIZE)
    {
        dTimer = dTimer - STEP_SIZE;
        for (const auto& id : m_DebugEmitters)
            m_pParticleMan->EmitParticles(id); // #REDESIGN przeniesc to po prostu do emitterow by sie ustalona liczba na sekunde emitowala
    }
}
#endif // DEBUG || _DEBUG