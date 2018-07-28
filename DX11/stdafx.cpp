#include "stdafx.h"
#include "ParticleManager.h"

CEngine* g_Engine = nullptr;
CInputsListener* g_Input = nullptr;

namespace utils
{
    bool FatalError(LPCWSTR msg)
    {
        if (g_Engine)
            MessageBox(g_Engine->CurrentWindow(), msg, L"Fatal Error!", MB_ICONERROR);
        else
            LogD("Fatal Error - " << msg);
        return false;
    }

    bool FatalError(LPCSTR msg)
    {
        if (g_Engine)
            MessageBoxA(g_Engine->CurrentWindow(), msg, "Fatal Error!", MB_ICONERROR);
        else
            LogD("Fatal Error - " << msg);
        return false;
    }

    void UpdateWindowBar(HWND hwnd)
    {
        //#REDESIGN
        static ParticleBufferData* partData = g_Engine->GetParticleManager()->m_ParticlesData;

        std::wstring ws;
        ws = WINDOW_TITLE;
        ws += L" - Num of particles: ";
        ws += std::to_wstring(partData->m_iCPUFree);
        ws += L" | Sorted on: ";
        ws += g_Engine->m_SortOnGPU ? L"GPU" : L"CPU";
        SetWindowText(hwnd, ws.c_str());
    }
}