#pragma once
#include "Timer.h"

class ICamera;
class CDxRenderer;
class pxManager;
class ParticleManager;

class CEngine
{
public:
    CEngine();
    ~CEngine();

    enum ERendererType
    {
        DX10 = 0,
        // Vulkan
    };

    bool Initialize(HWND hwnd, 
        UINT windowW, UINT windowH, 
        ERendererType type);

    void Frame();
    void UpdateScene();

    //  Getters & Setters
    const UINT&     WindowWidth()   const { return m_WindowWidth; }
    const UINT&     WindowHeight()  const { return m_WindowHeight; }
    HWND            CurrentWindow() const { return m_Hwnd; }
    ICamera*        GetCam();
    CTimer&         GetTimer() { return m_Timer; }

    CDxRenderer* Renderer()      const { return m_Renderer; }
    pxManager*   PhysX()         const { return m_PxMan; }

    bool IsPhysXPaused() { return m_PhysxPause; }
    void SetPhysXPause(bool val) { m_PhysxPause = val; }
    ParticleManager* GetParticleManager() { return m_pParticleMan; }

    // #REFACTOR
    // Particle Input
    void EmitParticle(int id, bool randomiseVelocities = true);
    void EmitExplosion(int id);
    void EmitParticlesInLoop();

    // #REDESIGN
    int                 m_ActEmittedId; // active emitter #REDESIGN
    bool                m_ActRandomVel; // random velocity #REDESIGN
    bool                m_RestrictTo60FPS;
    bool                m_SortOnGPU;

private:
    // Init
    bool InitPhysXAndParticles();
    void CreateParticleEmittersAndExplosions();

    // Misc
    bool Shutdown();

    // Window
    HWND    m_Hwnd;
    UINT    m_WindowWidth;
    UINT    m_WindowHeight;
 
    // Renderer 
    // #VULKAN przy dodawaniu vulkana stworzyc wspolny interfejs i tlyko go tu umiescic
    CDxRenderer*    m_Renderer;

    // Physics
    pxManager*		m_PxMan;
    bool            m_PhysxPause;

    // Particles
    ParticleManager*	m_pParticleMan;

    // Engine Timer
    CTimer m_Timer;
    CTimer m_60FPSTimer;

    // Sort debug emitters
#if defined(DEBUG) || (_DEBUG)
    void CreateDebugEmmiters();
    void EmitDebugEmmiters();

    std::vector<int> m_DebugEmitters;
#endif
};