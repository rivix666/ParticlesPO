#include "stdafx.h"
#include "pxManager.h"

using namespace physx;
using namespace std;

pxManager* pxManager::pPxManInstance;

pxManager::pxManager() : D_STEP_SIZE(1.0f / 60.0f)
{
}

pxManager::~pxManager()
{
    Foundation->release();
    Material->release();
    staticObj->release();
    pPhysicsSDK->release();
}

pxManager* pxManager::getInstance(){
    if (pPxManInstance == nullptr){
        pPxManInstance = new pxManager();
        pPxManInstance->initPhysX();
    }

    return pPxManInstance;
}

PxFilterFlags customFilterShader(PxFilterObjectAttributes attributes0, PxFilterData filterData0,
    PxFilterObjectAttributes attributes1, PxFilterData filterData1,
    PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
{
    // all initial and persisting reports for everything, with per-point data
    pairFlags = PxPairFlag::eCONTACT_DEFAULT
        | PxPairFlag::eTRIGGER_DEFAULT
        | PxPairFlag::eNOTIFY_CONTACT_POINTS
        | PxPairFlag::eCCD_LINEAR; //Set flag to enable CCD (Continuous Collision Detection) 

    return PxFilterFlag::eDEFAULT;
}

void pxManager::initPhysX()
{
    Foundation = PxCreateFoundation(PX_PHYSICS_VERSION, DefaultAllocatorCallback, DefaultErrorCallback);
    pPhysicsSDK = PxCreatePhysics(PX_PHYSICS_VERSION, *Foundation, PxTolerancesScale());
    initScene();
    initVisualDebuggerConnection();
    initGroundPlane();
}

void pxManager::initScene()
{
    PxSceneDesc sceneDesc(pPhysicsSDK->getTolerancesScale());	//Descriptor class for scenes 

    sceneDesc.gravity = PxVec3(0.0f, -9.8f, 0.0f);		//Setting gravity
    sceneDesc.cpuDispatcher = PxDefaultCpuDispatcherCreate(1);	
    sceneDesc.filterShader = customFilterShader;
    sceneDesc.flags |= PxSceneFlag::eENABLE_CCD | PxSceneFlag::eENABLE_KINEMATIC_STATIC_PAIRS;
    pScene = pPhysicsSDK->createScene(sceneDesc);				//Creates a scene 
}

void pxManager::initVisualDebuggerConnection()
{
    // check if PvdConnection manager is available on this platform
    if (pPhysicsSDK->getPvdConnectionManager() == NULL)
        return;

    // setup connection parameters
    const char*     pvd_host_ip = "127.0.0.1";  
    int             port = 5425;        
    unsigned int    timeout = 100;         

    PxVisualDebuggerConnectionFlags connectionFlags = PxVisualDebuggerConnectionFlag::eDEBUG;

    // try to connect
    debugger::comm::PvdConnection* theConnection = PxVisualDebuggerExt::createConnection(pPhysicsSDK->getPvdConnectionManager(),
        pvd_host_ip, port, timeout, connectionFlags);
}

void pxManager::initGroundPlane()
{
    Material = pPhysicsSDK->createMaterial(1.0, 1.0, 0.0); //Creating a PhysX material

    PxPlane plane(PxVec3(0, 0.1, 0), PxVec3(0, 1, 0));
    staticObj = PxCreatePlane(*pPhysicsSDK, plane, *Material);
    PxShape* planeShape = pPhysicsSDK->createShape(PxPlaneGeometry(), *Material);
    planeShape->setLocalPose(PxTransform(PxVec3(0.0001, 0, 0)));
    planeShape->setFlag(PxShapeFlag::ePARTICLE_DRAIN, true);
    staticObj->attachShape(*planeShape);

    pScene->addActor(*staticObj);
}

int pxManager::SimulatePhysX()
{
    static double dTimer = 0.0f;
    dTimer += g_Engine->GetTimer().getElapsedTime();
    
    int count = 0;
    while (dTimer > D_STEP_SIZE)
    {
        dTimer = dTimer - D_STEP_SIZE;
        pScene->simulate(D_STEP_SIZE);
        pScene->fetchResults(true);
        count++;
    } 

    return count;
}