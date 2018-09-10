#include "stdafx.h"
#include <atlbase.h>

CEngine* g_Engine = nullptr;

namespace utils
{
    bool FatalError(HWND hwnd, LPCWSTR msg)
    {
        if (hwnd)
            MessageBox(hwnd, msg, L"Fatal Error!", MB_ICONERROR);
        else
            LogD("Fatal Error - " << msg);
        return false;
    }

    bool FatalError(HWND hwnd, LPCSTR msg)
    {
        if (hwnd)
            MessageBoxA(hwnd, msg, "Fatal Error!", MB_ICONERROR);
        else
            LogD("Fatal Error - " << msg);
        return false;     
    }

    double CalcSPF()
    {
        static uint32_t idx = 0;
        static double acc = 0.0;
        static double last = 0.0;
        static CTimer spf_timer(ETimerType::MiliSeconds);

        double elapsed_time = spf_timer.getElapsedTime();
        if (idx <= 100)
        {
            acc += elapsed_time;
            idx++;
        }
        else
        {
            last = acc / (double)idx;
            acc = 0.0;
            idx = 0;
        }

        spf_timer.startTimer();
        return last;
    }

    void UpdateWindowBar(HWND hwnd)
    {
        std::wstring ws;
        ws = W_WINDOW_TITLE;

        // Particles Info
        ws += L" - Num of particles: ";
        ws += std::to_wstring(g_Engine->ParticleMgr()->BuffData().CPUFree);

        ws += L" | Sorted on: ";
        if (g_Engine->ParticleMgr()->SortMethod() == CParticleManager::ESortType::NONE)
            ws += L"NONE";
        else
            ws += g_Engine->ParticleMgr()->SortMethod() == CParticleManager::ESortType::GPU ? L"GPU" : L"CPU";

        // Fps/Spf
        double spf = CalcSPF();
        ws += L" | SPF: ";
        ws += std::to_wstring(spf);
        ws += L" | FPS: ";
        ws += std::to_wstring(1000.0 / spf);

        // Camera pos
        glm::vec3 cam_pos = g_Engine->Camera()->CameraPosition();
        ws += L" | X: ";
        ws += std::to_wstring(cam_pos.x);
        ws += L" Y: ";
        ws += std::to_wstring(cam_pos.y);
        ws += L" Z: ";
        ws += std::to_wstring(cam_pos.z);

        SetWindowText(hwnd, ws.c_str());  
    }
}