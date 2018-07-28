#pragma once
#include <PhysX/PxPhysicsAPI.h>
#include <d3d11.h>//#REDESIGN move to cpp
#include <d3dx11.h>
#include "Timer.h"

class pxManager
{
public:
    ~pxManager();

    const double D_STEP_SIZE;

    static pxManager* getInstance();
    int SimulatePhysX();
    physx::PxScene* getPxScene() { return pScene; }
    physx::PxPhysics* getPxSDK() { return pPhysicsSDK; }

private:
    pxManager();
    void initPhysX();
    void initVisualDebuggerConnection();
    void initGroundPlane();
    void initScene();

    static pxManager* pPxManInstance;

    physx::PxFoundation*			Foundation;			
    physx::PxDefaultErrorCallback	DefaultErrorCallback;		
    physx::PxDefaultAllocator		DefaultAllocatorCallback;	

    physx::PxMaterial*				Material;

    physx::PxRigidStatic*           staticObj;

    physx::PxPhysics*				pPhysicsSDK;			
    physx::PxScene*					pScene;				
};

