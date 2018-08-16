#include "stdafx.h"
#include "Engine.h"
#include "Camera.h"
#include "InputsListener.h"

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

        m_PxMgr->Shutdown();
        DELETE(m_PxMgr);

        SAFE_DELETE(m_Camera);
    }
}

bool CEngine::Init()
{
    if (m_MainWnd)
    {
        m_Hwnd = glfwGetWin32Window(m_MainWnd);
        input::InitInputListeners(m_MainWnd);

        m_Renderer = new CVulkanRenderer(m_MainWnd);
        if (!m_Renderer->Init())
            return false;

        m_PxMgr = new CPxManager();
        if (!m_PxMgr->Init())
            return false;

        m_ParticleMgr = new CParticleManager();
        if (!m_ParticleMgr->Init())
            return false;

        m_ObjectControl = new CGObjectControl(m_Renderer->GetDevice());
        m_Camera = new CCamera();

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