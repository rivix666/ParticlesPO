#pragma once

// Includes
#include <vector>
#include <windows.h>
#include <time.h>
#include <list>
#include <xnamath.h>

#include "Engine.h"
#include "InputsListener.h"
#include "ICamera.h"
#include "Structs.h"
#include "mathHelper.h"
#include "Debug.h"

#include "d3dx/d3dxGlobal.h"

#if defined(DEBUG) || defined(_DEBUG)
#include "Debug.h"
#endif

#define WINDOW_TITLE L"Particles v1.1 - Praca Magisterska Tomasz Kud 2017"
#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 640

#define DELETE(x) { delete x; x = nullptr; }
#define SAFE_DELETE(x) { if (x) { delete x; x = nullptr; } }

#define DELETE_ARR(x) { delete[] x; x = nullptr; }
#define SAFE_DELETE_ARR(x) { if (x) { delete[] x; x = nullptr; } }

#define PARTICLE_BUFF_SIZE 24576    // Change also in ParticleSort.fx
#define BITONIC_BLOCK_SIZE 32       // Change also in ParticleSort.fx

extern CEngine* g_Engine;
extern CInputsListener* g_Input;

namespace utils
{
    // Errors Management
    bool FatalError(LPCWSTR msg);
    bool FatalError(LPCSTR msg);
    void UpdateWindowBar(HWND hwnd);
}