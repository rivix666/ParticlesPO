#include "stdafx.h"
#include "Engine.h"

// Objects includes
#include "Objects/GBaseObject.h"

// Techniques includes
#include "Techs/BaseObjectTechnique.h"
#include "Techs/TechniqueManager.h"
#include "Techs/ParticleBaseTechnique.h"
#include "Techs/ParticleTechniques.h"
#include "Techs/ParticleSortCmpTechnique.h"

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

        // #UNI_BUFF #DESC_MGR patrz CVulkanRenderer::Init
        // Init Particle manager (It's need to be initialized after renderer and before Techniques)
        // if (!m_ParticleMgr->Init())
        //     return false;

        // Init Techniques manager (It's need to be initialized after renderer)
        m_TechMgr->InitTechniques();

        // Create and init Physics manager
        m_PxMgr = new CPxManager();
        if (!m_PxMgr->Init())
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
        //auto halo_flare_tech = new CParticleHaloFlareTechnique(base_tech);
        //REGISTER_TECH(ParticleVertex, halo_flare_tech);

        // #PARTICLES_ZYEB_TECH vkCreatePipelineLayout(): max per-stage uniform buffer bindings count (16) exceeds device maxPerStageDescriptorUniformBuffers limit (15). 
        // The spec valid usage text states 'The total number of descriptors of the type VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER and VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC 
        // accessible to any shader stage across all elements of pSetLayouts must be less than or equal to VkPhysicalDeviceLimits::maxPerStageDescriptorUniformBuffers
        //////////////////////////////////////////////////////////////////////////
        // #PARTICLES_ZYEB_TECH Toc mogê mieæ jedno ubo a w œrodku tablice wartoœci <face-palm>
        //auto round_sparks_tech = new CParticleRoundSparksTechnique(base_tech);
        //REGISTER_TECH(ParticleVertex, round_sparks_tech);

        // #PARTICLES_ZYEB_TECH jednak to wyzej bardzo pwoazne trzeba szybko fixnac
        // Register Compute techniques
        REGISTER_TECH(ParticleSort, new CParticleSortCmpTechnique); //#TECH dorobic im jakeigos enumma bylatwiej sie je rozroznialo 

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

    //////////////////////////////////////////////////////////////////////////
    if (ParticleMgr()->BuffData().CPUFree > 0)
    {
        // Create staging buffer
        VkDeviceSize bufferSize = ParticleMgr()->BuffData().CPUFree * sizeof(ParticleVertex);
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        g_Engine->Renderer()->CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
        g_Engine->Renderer()->CopyBuffer(ParticleMgr()->m_VertexBuffer, stagingBuffer, bufferSize);

        // Map staging buffer
        void* data;
        vkMapMemory(g_Engine->Device(), stagingBufferMemory, 0, bufferSize, 0, &data);

        // original buff
        //////////////////////////////////////////////////////////////////////////
        ParticleVertex* data2 = new ParticleVertex[ParticleMgr()->BuffData().CPUFree];
        memcpy(data2, data, (size_t)bufferSize);
        
        // DEBUG #COMPUTE
        for (uint32_t i = 0; i < ParticleMgr()->BuffData().CPUFree; i++)
        {
            auto& vec = data2[i].dummy_vec;
            LogD(" | ");
            LogD(vec.y);
        }
        LogD("\n");

        //////////////////////////////////////////////////////////////////////////

        // debug buff
        //////////////////////////////////////////////////////////////////////////
#ifdef _DEBUG
        //CParticleSortCmpTechnique* tech = static_cast<CParticleSortCmpTechnique*>(TechMgr()->GetTechnique(6));
        //void* data2;// = new ParticleVertex[ParticleMgr()->BuffData().CPUFree];
        //vkMapMemory(g_Engine->Device(), tech->m_DebugBufferMemory, 0, bufferSize, 0, &data2);
        //
        //ParticleVertex* meh_data = static_cast<ParticleVertex*>(data2);
        //for (uint32_t i = 0; i < ParticleMgr()->BuffData().CPUFree; i++)
        //{
        //    auto& vec = meh_data[i].dummy_vec;
        //    LogD(" | ");
        //    LogD(vec.y);
        //}
        //LogD("\n");
        //
        //vkUnmapMemory(g_Engine->Device(), tech->m_DebugBufferMemory);
#endif
        //////////////////////////////////////////////////////////////////////////


        // Unmap staging buffer and copy its data into vertex buffer
        vkUnmapMemory(g_Engine->Device(), stagingBufferMemory);

        // Destroy staging buffer
        vkDestroyBuffer(g_Engine->Device(), stagingBuffer, nullptr);
        vkFreeMemory(g_Engine->Device(), stagingBufferMemory, nullptr);
    }
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