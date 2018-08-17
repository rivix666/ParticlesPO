#pragma once

#include <iostream>
#include <stdio.h>
#include <tchar.h>

#include <math.h>
#include <algorithm>
#include <vector>
#include <array>
#include <set>
#include <sstream>
#include <map>

// GLFW includes
#define GLFW_INCLUDE_VULKAN
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

// GLM includes
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Need to be under GLFW includes
#include <windows.h>

// Internal includes
#include "Engine.h"
#include "Utils/Timer.h"

// Window defines
#define WINDOW_TITLE "Particles v1.1 - Praca Magisterska Tomasz Kud 2017"
#define W_WINDOW_TITLE L"Particles v1.1 - Praca Magisterska Tomasz Kud 2017"

#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 640
#define RENDER_WIDTH WINDOW_WIDTH
#define RENDER_HEIGHT WINDOW_HEIGHT

// Renderer defines
#define OBJ_PER_TECHNIQUE 8
#define PARTICLE_BUFF_SIZE 24576

// Projection defines
#define FOV 60.0f
#define ASPECT (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT
#define Z_NEAR 0.1f
#define Z_FAR 600.0f

// Math defines
#define PI 3.141592653589793238462643383279
#define TWO_PI 6.283185307179586476925286766559
#define HALF_PI 1.57079632679
#define DEG_TO_RAD 0.01745329251994329576923690768489

// Release defines
#define DELETE(x) { delete x; x = nullptr; }
#define SAFE_DELETE(x) { if (x) { delete x; x = nullptr; } }
#define DELETE_ARR(x) { delete[] x; x = nullptr; }
#define SAFE_DELETE_ARR(x) { if (x) { delete[] x; x = nullptr; } }

// Optimize defines #PRAGMA_OPTIMIZE obadac czemu nie dziala
#define PRAGMA(X) _Pragma(#X)
#define OPTIMIZE_DISABLE PRAGMA(optimize("", off))
#define OPTIMIZE_ENABLE PRAGMA(optimize("", on))

// Init defines
#define INIT_PTR_VEC(vec, size) { vec.resize(size); for (int i = 0; i < size; i++) vec[i] = nullptr; }

// Misc defines
#define VKRESULT(res) res != VK_SUCCESS

// Debug defines
#define LogD( s )                           \
{                                           \
    std::wstringstream ws;                  \
    ws << s;                                \
    OutputDebugString(ws.str().c_str());    \
}

typedef unsigned int uint;

extern CEngine* g_Engine;

namespace utils
{
    // Pointers handle
    template<typename T>
    void DeletePtrVec(T& vec)
    {
        uint count = vec.size();
        for (uint i = 0; i < count; i++)
        {
            SAFE_DELETE(vec[i]);
        }
        vec.clear();
    }

    // Errors Management
    bool FatalError(HWND hwnd, LPCWSTR msg);
    bool FatalError(HWND hwnd, LPCSTR msg);
    
    // Misc
    void UpdateWindowBar(HWND hwnd);
}

namespace math_helper
{
    static double dRand(double dMin, double dMax)
    {
        double d = (double)rand() / RAND_MAX;
        return dMin + d * (dMax - dMin);
    }
}