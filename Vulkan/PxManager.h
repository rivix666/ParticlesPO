#pragma once
#include <PhysX/PxPhysicsAPI.h>

class IGObject;

class CPxManager
{
public:
    CPxManager();
    ~CPxManager();

    // Init
    bool Init();
    bool Shutdown();

    // Frame simulate
    int SimulatePhysX();

    // Actors
    void RegisterActor(physx::PxActor* actor);
    bool UnregisterActor(physx::PxActor* actor);

    // Getters
    physx::PxScene* Scene() { return m_Scene; }
    physx::PxPhysics* SDK() { return m_PhysicsSDK; }

    const double D_STEP_SIZE;

protected:
    bool InitScene();

#ifdef _DEBUG
    bool InitVisualDebuggerConnection();
#endif

    // PhysX
    physx::PxScene*					m_Scene;
    physx::PxPhysics*				m_PhysicsSDK;
    physx::PxFoundation*			m_Foundation;
    physx::PxDefaultErrorCallback	m_DefaultErrorCallback;
    physx::PxDefaultAllocator		m_DefaultAllocatorCallback;

    // Actors
    std::vector<physx::PxActor*>    m_Actors;
};

