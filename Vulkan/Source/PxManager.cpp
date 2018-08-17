#include "stdafx.h"
#include "PxManager.h"

using namespace physx;
using namespace std;

CPxManager::CPxManager() : D_STEP_SIZE(1.0f / 60.0f)
{
}

CPxManager::~CPxManager()
{
    Shutdown();
}

bool CPxManager::Init()
{
    // Init PhysX
    m_Foundation = PxCreateFoundation(PX_PHYSICS_VERSION, m_DefaultAllocatorCallback, m_DefaultErrorCallback);
    m_PhysicsSDK = PxCreatePhysics(PX_PHYSICS_VERSION, *m_Foundation, PxTolerancesScale());

    if (!m_Foundation || !m_PhysicsSDK)
    {
        utils::FatalError(g_Engine->Hwnd(), "Failed to init PhysX");
        return Shutdown();
    }

    if (!InitScene())
        return Shutdown();

#ifdef _DEBUG
    if (!InitVisualDebuggerConnection())
        utils::FatalError(nullptr, "Failed to connect to PhysX visual debugger");
#endif

    return true;
}

bool CPxManager::Shutdown()
{
    if (m_PhysicsSDK)
    {
        m_PhysicsSDK->release();
        m_PhysicsSDK = nullptr;
    }
    if (m_Foundation)
    {
        m_Foundation->release();
        m_Foundation = nullptr;
    }
    return false;
}

int CPxManager::SimulatePhysX()
{
    static double dTimer = 0.0f;
    dTimer += g_Engine->Timer().getElapsedTime();

    int count = 0;
    while (dTimer > D_STEP_SIZE)
    {
        dTimer = dTimer - D_STEP_SIZE;
        m_Scene->simulate(D_STEP_SIZE);
        m_Scene->fetchResults(true);
        count++;
    }

    return count;
}

void CPxManager::RegisterActor(PxActor* actor)
{
    m_Scene->addActor(*actor);
    m_Actors.push_back(actor);
}

bool CPxManager::UnregisterActor(PxActor* actor)
{
    auto it = std::find(m_Actors.begin(), m_Actors.end(), actor);
    if (it == m_Actors.end())
        return false;

    m_Actors.erase(it, it + 1);
    m_Scene->removeActor(*actor);
    return true;
}

#ifdef _DEBUG
bool CPxManager::InitVisualDebuggerConnection()
{
    // Check if PvdConnection manager is available on this platform
    if (m_PhysicsSDK->getPvdConnectionManager() == nullptr)
        return false;

    // Setup connection parameters
    const char*     pvd_host_ip = "127.0.0.1";
    int             port = 5425;
    unsigned int    timeout = 100;

    PxVisualDebuggerConnectionFlags connectionFlags = PxVisualDebuggerConnectionFlag::eDEBUG;

    // Try to connect
    debugger::comm::PvdConnection* theConnection = PxVisualDebuggerExt::createConnection(m_PhysicsSDK->getPvdConnectionManager(),
        pvd_host_ip, port, timeout, connectionFlags);

    return theConnection != nullptr;
}
#endif

PxFilterFlags CustomFilterShader(PxFilterObjectAttributes attributes0, PxFilterData filterData0,
    PxFilterObjectAttributes attributes1, PxFilterData filterData1,
    PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
{
    // All initial and persisting reports for everything, with per-point data
    pairFlags = PxPairFlag::eCONTACT_DEFAULT
        | PxPairFlag::eTRIGGER_DEFAULT
        | PxPairFlag::eNOTIFY_CONTACT_POINTS
        | PxPairFlag::eCCD_LINEAR; //Set flag to enable CCD (Continuous Collision Detection) 

    return PxFilterFlag::eDEFAULT;
}

bool CPxManager::InitScene()
{
    PxSceneDesc sceneDesc(m_PhysicsSDK->getTolerancesScale());	
    sceneDesc.gravity = PxVec3(0.0f, -9.8f, 0.0f);
    sceneDesc.cpuDispatcher = PxDefaultCpuDispatcherCreate(1);
    sceneDesc.filterShader = CustomFilterShader;
    sceneDesc.flags |= PxSceneFlag::eENABLE_CCD | PxSceneFlag::eENABLE_KINEMATIC_STATIC_PAIRS;
    m_Scene = m_PhysicsSDK->createScene(sceneDesc);

    if (!m_Scene)
        return utils::FatalError(g_Engine->Hwnd(), "Failed to init PhysX scene");

    return true;
}
