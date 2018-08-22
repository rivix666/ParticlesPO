#pragma once

// Renderer includes
#include "VulkanRenderer.h"
#include "DescriptorManager.h"

// PhysX includes
#include "PxManager.h"

// Objects/Particles includes
#include "Objects/GObjectControl.h"
#include "Particles/ParticleManager.h"

// Misc includes
#include "Camera.h"
#include "Utils/Timer.h"

class CTechniqueManager;
class CDescriptorManager;

class CEngine
{
public:
    CEngine(GLFWwindow* window);
    ~CEngine();

    // Init
    bool Init();
    bool RegisterTechniques();

    // Frame
    void Frame();
    void UpdateScene();

    // Window Getters
    inline HWND Hwnd() const { return m_Hwnd; }
    inline GLFWwindow* GlfwWindow() const { return m_MainWnd; }

    // Managers Getters
    inline CTechniqueManager* TechMgr() const { return m_TechMgr; }
    inline CPxManager* PxManager() const { return m_PxMgr; }
    inline CParticleManager* ParticleMgr() const { return m_ParticleMgr; }
    inline CGObjectControl* ObjectControl() const { return m_ObjectControl; }

    // Renderer Getters
    inline CVulkanRenderer* Renderer() const { return m_Renderer; }
    inline CDescriptorManager* DescMgr() const { return m_Renderer != nullptr ? m_Renderer->DescMgr() : nullptr; }
    inline VkDevice Device() const { return m_Renderer != nullptr ? m_Renderer->GetDevice() : nullptr; }

    // Timer Getters
    inline CTimer& Timer() { return m_FrameTimer; }
    inline double LastFrameTime() const { return m_LastFrameTime; }

    // Misc Getters
    inline CCamera* Camera() const { return m_Camera; }

    // Misc
    void RecordCommandBuffer(VkCommandBuffer& cmd_buff);
    void RequestCommandBufferReset() { m_CmdBuffersResetRequested = true; }

private:
    // Wnd
    HWND m_Hwnd = nullptr;
    GLFWwindow* m_MainWnd = nullptr;

    // Systems Managers
    CVulkanRenderer*    m_Renderer = nullptr;
    CGObjectControl*    m_ObjectControl = nullptr;
    CParticleManager*   m_ParticleMgr = nullptr;
    CTechniqueManager*  m_TechMgr = nullptr;

    // PhysX
    CPxManager* m_PxMgr = nullptr;

    // Camera
    CCamera* m_Camera = nullptr;

    // Timer
    double m_LastFrameTime = 0.0;
    CTimer m_FrameTimer = CTimer(ETimerType::MiliSeconds);

    // Misc
    bool m_CmdBuffersResetRequested = false;
};

