#include "stdafx.h"
#include "Objects/GBaseObject.h"
#include "Particles/Emitters/BaseEmitter.h" 

//#pragma optimize("", off)

GLFWwindow* window = nullptr;
HWND hwnd = nullptr;

// Window hanlde
//////////////////////////////////////////////////////////////////////////
bool InitWindow()
{
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, nullptr, nullptr);
    hwnd = glfwGetWin32Window(window);

    if (!window || !hwnd)
    {
        utils::FatalError(hwnd, "Application could not create window");
        return false;
    }

    return true;
}

// Engine
//////////////////////////////////////////////////////////////////////////
bool InitEngine()
{
    if (!g_Engine)
        g_Engine = new CEngine(window);

    if (!g_Engine->Init())
    {
        utils::FatalError(hwnd, "Engine could not be created");
        return false;
    }

    return true;
}

void RegisterBaseObjects()
{
    REGISTER_OBJ_TECH(0, new CGBaseObject(EBaseObjInitType::PLANE));
    REGISTER_OBJ_TECH(0, new CGBaseObject(EBaseObjInitType::BOX, glm::vec3(0.0f, 2.0f, 0.0f)));
    REGISTER_OBJ_TECH(0, new CGBaseObject(EBaseObjInitType::BOX, glm::vec3(-4.0f, 2.0f, 0.0f)));
    REGISTER_OBJ_TECH(0, new CGBaseObject(EBaseObjInitType::BOX, glm::vec3(4.0f, 2.0f, 0.0f)));
    g_Engine->Renderer()->RecreateCommandBuffer();
}

void RegisterEmitters()
{
    auto tmp = new CBaseEmitter(2, 1000);
    REGISTER_EMITTER(tmp, -1);
    tmp->SetPos(glm::vec3(-8.0f, 6.0f, 0.0f));

    tmp = new CBaseEmitter(3, 1000);
    REGISTER_EMITTER(tmp, -1);
    tmp->SetPos(glm::vec3(-4.0f, 6.0f, 0.0f));

    tmp = new CBaseEmitter(4, 1000);
    REGISTER_EMITTER(tmp, -1);
    tmp->SetPos(glm::vec3(0.0f, 6.0f, 0.0f));

    tmp = new CBaseEmitter(5, 1000);
    REGISTER_EMITTER(tmp, -1);
    tmp->SetPos(glm::vec3(4.0f, 6.0f, 0.0f));

    tmp = new CBaseEmitter(6, 1000);
    REGISTER_EMITTER(tmp, -1);
    tmp->SetPos(glm::vec3(8.0f, 6.0f, 0.0f));
}

// Shutdown
//////////////////////////////////////////////////////////////////////////
int Shutdown()
{
    SAFE_DELETE(g_Engine);

    if (window)
        glfwDestroyWindow(window);

    return 0;
}

// Main
//////////////////////////////////////////////////////////////////////////
int _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
    glfwInit();

    if (!InitWindow())
        return Shutdown();

    if (!InitEngine())
        return Shutdown();

    RegisterBaseObjects();
    RegisterEmitters();

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        g_Engine->Frame();

        utils::UpdateWindowBar(hwnd);
    }

    return Shutdown();
}


