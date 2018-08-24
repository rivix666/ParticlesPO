#include "stdafx.h"
#include "Engine.h"

// Objects includes
#include "Objects/GBaseObject.h"

// Techniques includes
#include "Techs/BaseObjectTechnique.h"
#include "Techs/TechniqueManager.h"
#include "Techs/ParticleBaseTechnique.h"
#include "Techs/ParticleTechniques.h"

// Misc includes
#include "InputsListener.h"

CEngine::CEngine(GLFWwindow* window)
    : m_MainWnd(window)
{
    m_FrameTimer.startTimer();
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

        // Create Particle Textures Manager, and register textures in it
        m_ParticleTexMgr = new CParticleTextureManager();
        m_ParticleTexMgr->RegisterBaseTextures();

        // Create Particle Manager
        m_ParticleMgr = new CParticleManager();

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

        // Init Particle manager (It's need to be initialized after renderer and PhysX)
        if (!m_ParticleMgr->Init())
            return false;

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
        // Register Objects techniques
        CGBaseObject::s_TechId = REGISTER_TECH(BaseVertex, new CBaseObjectTechnique);

        // Register Particles techniques
        auto base_tech = new CParticleBaseTechnique();
        REGISTER_TECH(ParticleVertex, base_tech);
        auto flame_tech = new CParticleFlameTechnique(base_tech);
        REGISTER_TECH(ParticleVertex, flame_tech);
        auto smoke_tech = new CParticleSmokeTechnique(base_tech);
        REGISTER_TECH(ParticleVertex, smoke_tech);
        auto debris_tech = new CParticleDebrisTechnique(base_tech);
        REGISTER_TECH(ParticleVertex, debris_tech);
        auto flare_tech = new CParticleFlareTechnique(base_tech);
        REGISTER_TECH(ParticleVertex, flare_tech);
        auto halo_flare_tech = new CParticleHaloFlareTechnique(base_tech);
        REGISTER_TECH(ParticleVertex, halo_flare_tech);

        // #PARTICLES vkCreatePipelineLayout(): max per-stage uniform buffer bindings count (16) exceeds device maxPerStageDescriptorUniformBuffers limit (15). 
        // The spec valid usage text states 'The total number of descriptors of the type VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER and VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC 
        // accessible to any shader stage across all elements of pSetLayouts must be less than or equal to VkPhysicalDeviceLimits::maxPerStageDescriptorUniformBuffers
        //////////////////////////////////////////////////////////////////////////
        // #PARTICLES Toc mog� mie� jedno ubo a w �rodku tablice warto�ci <face-palm>
        //auto round_sparks_tech = new CParticleRoundSparksTechnique(base_tech);
        //REGISTER_TECH(ParticleVertex, round_sparks_tech);

        return true;
    }
    return false;
}

void CEngine::Frame()
{  
    // Framerate lock
    if (m_LockTo60FPS)
    {
        while (m_FrameLockTimer.getElapsedTime() < 0.0166) {}
        m_FrameLockTimer.startTimer();
    }

    // Update scene
    UpdateScene();

    // Store elapsed time
    m_LastFrameTime = m_FrameTimer.getElapsedTime();

    // Reset frame timer before render
    m_FrameTimer.startTimer();

    // Reset command buffers if needed
    if (m_CmdBuffersResetRequested)
    {
        Renderer()->RecreateCommandBuffer();
        m_CmdBuffersResetRequested = false;
    }

    // Render
    m_Renderer->PresentQueueWaitIdle();
    m_Renderer->Render();
}

void CEngine::UpdateScene()
{
    // Move camera
    m_Camera->Update();

    // Simulate PhysX
    m_PxMgr->SimulatePhysX();

    // Simulate/Update particles
    m_ParticleMgr->Simulate();
    m_ParticleMgr->UpdateBuffers();

    // Update Objects
    m_ObjectControl->UpdatePhysXActors();
    m_ObjectControl->UpdateUniBuffers();
}

void CEngine::RecordCommandBuffer(VkCommandBuffer& cmd_buff)
{
    if (m_ObjectControl)
        m_ObjectControl->RecordCommandBuffer(cmd_buff);
    if (m_ParticleMgr)
        m_ParticleMgr->RecordCommandBuffer(cmd_buff);
}