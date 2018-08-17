#include "stdafx.h"
#include "Engine.h"
#include "Camera.h"
#include "InputsListener.h"
#include "Objects/GBaseObject.h"
#include "Techs/TechniqueManager.h"

// Techniques
#include "Techs/BaseTechnique.h"
#include "Techs/ParticleBaseTechnique.h"

// #PARTICLES tymczasowe do testów
#include "Particles/Emitters/BaseEmitter.h"

CEngine::CEngine(GLFWwindow* window)
    : m_MainWnd(window)
{
}

CEngine::~CEngine()
{
    if (m_Renderer)
    {
        if (m_ObjectControl)
        {
            m_ObjectControl->Shutdown();
            DELETE(m_ObjectControl);
        }

        if (m_ParticleMgr)
        {
            m_ParticleMgr->Shutdown();
            DELETE(m_ParticleMgr);
        }

        m_Renderer->Shutdown();
        DELETE(m_Renderer);

        if (m_TechMgr)
        {
            m_TechMgr->Shutdown();
            DELETE(m_TechMgr);
        }

        if (m_PxMgr)
        {
            m_PxMgr->Shutdown();
            DELETE(m_PxMgr);
        }

        SAFE_DELETE(m_Camera);
    }
}

bool CEngine::Init()
{
    if (m_MainWnd)
    {
        // Store window handle (hwnd)
        m_Hwnd = glfwGetWin32Window(m_MainWnd);

        // Init keyboard/mouse input
        input::InitInputListeners(m_MainWnd);

        // Create Techniques manager
        m_TechMgr = new CTechniqueManager();

        // Register Techniques
        if (!RegisterTechniques())
            return false;

        // Create and init renderer
        m_Renderer = new CVulkanRenderer(m_MainWnd);
        if (!m_Renderer->Init())
            return false;

        // Init Techniques manager (It's need to be initialized after renderer)
        m_TechMgr->InitTechniques();

        // Create and init Physics manager
        m_PxMgr = new CPxManager();
        if (!m_PxMgr->Init())
            return false;

        // Create and init Particle manager
        m_ParticleMgr = new CParticleManager();
        if (!m_ParticleMgr->Init())
            return false;

        // #PARTICLES tymczasowe do testów
        auto em = new CBaseEmitter(1, 1000);
        REGISTER_EMITTER(em, CBaseEmitter, -1);
        em->Emit(4);

        // Create Object control and camera
        m_ObjectControl = new CGObjectControl(m_Renderer->GetDevice());
        m_Camera = new CCamera();

        return true;
    }
    return false;
}

bool CEngine::RegisterTechniques()
{
    if (m_TechMgr)
    {
        CGBaseObject::s_TechId = REGISTER_TECH(BaseVertex, new CBaseTechnique);
        REGISTER_TECH(ParticleVertex, new CParticleBaseTechnique);
        return true;
    }
    return false;
}

void CEngine::Frame()
{
    // Start frame timer
    m_FrameTimer.startTimer();

    // Update scene
    UpdateScene();

    // Render
    m_Renderer->PresentQueueWaitIdle();
    m_Renderer->Render();

    // Store elapsed time
    m_LastFrameTime = m_FrameTimer.getElapsedTime();
}

void CEngine::UpdateScene()
{
    m_Camera->Update();
    m_ObjectControl->UpdateUniBuffers();
    m_PxMgr->SimulatePhysX();
}