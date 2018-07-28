#include "stdafx.h"
#include <tchar.h>
#include <sstream>

#include "Timer.h"
#include "Structs.h"

// Forward declarations
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

// Variables
//////////////////////////////////////////////////////////////////////////
HWND hwnd;

// Window Methods
//////////////////////////////////////////////////////////////////////////
bool InitWindow(HWND& hWnd, HINSTANCE hInstance, int width, int height)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = (WNDPROC)WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = 0;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = TEXT("Particles");
    wcex.hIconSm = 0;
    RegisterClassEx(&wcex);

    //Resize the window
    RECT rect = { 0, 0, width, height };
    AdjustWindowRect(&rect, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, FALSE);

    //create window from the class above
    //disable resizing and correct for extra width and height
    hWnd = CreateWindow(L"Particles",
        WINDOW_TITLE,
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        rect.right - rect.left,
        rect.bottom - rect.top,
        NULL,
        NULL,
        hInstance,
        NULL);

    //window handle not created
    if (!hWnd) return false;

    ShowWindow(hWnd, SW_SHOW);

    //if window creation was successful
    return true;
}

int Shutdown()
{
    SAFE_DELETE(g_Input);
    SAFE_DELETE(g_Engine);
    return 0;
}

bool InitEngine()
{
    if (!g_Engine)
        g_Engine = new CEngine();

    if (!g_Engine->Initialize(hwnd, WINDOW_WIDTH, WINDOW_HEIGHT, CEngine::DX10))
    {
        Shutdown();
        return false;
    }
    return true;
}

bool InitInputsListener()
{
    if (!g_Input)
        g_Input = new CInputsListener();

    if (!g_Input->RegisterInputDevices())
    {
        Shutdown();
        return false;
    }
    return true;
}

// An application-defined function that processes messages sent to a window
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INPUT:
        g_Input->HandleRawInput((HRAWINPUT&)lParam);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

// Main
//////////////////////////////////////////////////////////////////////////
int _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
    if (!InitWindow(hwnd, hInstance, WINDOW_WIDTH, WINDOW_HEIGHT))
        return 0;

    if (!InitEngine())
        return 0;

    if (!InitInputsListener())
        return 0;

    //InitTitmer
    g_Engine->GetTimer().setFrequency(TimerType::Seconds);
    g_Engine->GetTimer().startTimer();

    MSG msg = { 0 };

    // Main message loop
    while (true)
    {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) == TRUE)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (msg.message == WM_QUIT)
            {
                DestroyWindow(g_Engine->CurrentWindow());            
                return Shutdown();
            }
        }

        g_Engine->Frame();
        utils::UpdateWindowBar(hwnd);
    }

    return Shutdown();
}