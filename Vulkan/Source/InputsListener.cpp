#include "stdafx.h"
#include "InputsListener.h"

// Objects/UniBuff debug
#ifdef _DEBUG
#include "Objects/IGObject.h"
#include "Objects/GBaseObject.h" 
#include "Objects/GObjectControl.h"
#include "Techs/TechniqueManager.h"
#endif

void input::InitInputListeners(GLFWwindow* window)
{
    // Set keyboard
    glfwSetKeyCallback(window, KeyCallback);

    // Set mouse
    glfwSetCursorPosCallback(window, CursorPositionCallback);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    glfwSetScrollCallback(window, ScrollCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void input::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS)
        HandlePress(window, key, scancode, action, mods);
    else if (action == GLFW_RELEASE)
        HandleRelease(window, key, scancode, action, mods);
}

void input::HandlePress(GLFWwindow* window, const int& key, const int& scancode, const int& action, const int& mods)
{
    // Objects/UniBuff debug
#ifdef _DEBUG
    static IGObject* debug_obj = nullptr;
#endif

    switch (key)
    {
        // Camera control
    case GLFW_KEY_W:
    {
        g_Engine->Camera()->MoveFreeCam(ECamMoveDir::FORWARD, true);
        break;
    }
    case GLFW_KEY_S:
    {
        g_Engine->Camera()->MoveFreeCam(ECamMoveDir::BACKWARD, true);
        break;
    }
    case GLFW_KEY_A:
    {
        g_Engine->Camera()->MoveFreeCam(ECamMoveDir::LEFT, true);
        break;
    }
    case GLFW_KEY_D:
    {
        g_Engine->Camera()->MoveFreeCam(ECamMoveDir::RIGHT, true);
        break;
    }
    case GLFW_KEY_Q:
    {
        g_Engine->Camera()->MoveFreeCam(ECamMoveDir::DOWN, true);
        break;
    }
    case GLFW_KEY_E:
    {
        g_Engine->Camera()->MoveFreeCam(ECamMoveDir::UP, true);
        break;
    }
    case GLFW_KEY_LEFT_SHIFT:
    {
        g_Engine->Camera()->SetMoveSpeed(CCamera::DEFAULT_MOVE_SPEED * 4.0f);
        break;
    }
    case GLFW_KEY_LEFT_CONTROL:
    {
        g_Engine->Camera()->SetMoveSpeed(CCamera::DEFAULT_MOVE_SPEED / 4.0f);
        break;
    }

    // Emitters
    case GLFW_KEY_1:
    {
        TryToActivateEmitter(0, 1);
        break;
    }
    case GLFW_KEY_2:
    {
        TryToActivateEmitter(1, 1);
        break;
    }
    case GLFW_KEY_3:
    {
        TryToActivateEmitter(2, 1);
        break;
    }
    case GLFW_KEY_4:
    {
        TryToActivateEmitter(3, 1);
        break;
    }
    case GLFW_KEY_5:
    {
        TryToActivateEmitter(4, 2);
        break;
    }
    case GLFW_KEY_0:
    {
        g_Engine->ParticleMgr()->DeactivateAllEmitters();
        break;
    }

    // Sort
    case GLFW_KEY_F1:
    {
        g_Engine->ParticleMgr()->SetSortMethod(CParticleManager::ESortType::GPU);
        g_Engine->Renderer()->RecreateCommandBuffer();
        break;
    }
    case GLFW_KEY_F2:
    {
        g_Engine->ParticleMgr()->SetSortMethod(CParticleManager::ESortType::CPU);
        g_Engine->Renderer()->RecreateCommandBuffer();
        break;
    }
    case GLFW_KEY_F3:
    {
        g_Engine->ParticleMgr()->SetSortMethod(CParticleManager::ESortType::NONE);
        g_Engine->Renderer()->RecreateCommandBuffer();
        break;
    }

    // Misc
    case GLFW_KEY_V:
    {
        g_Engine->LockFramerate(!g_Engine->IsFramerateLocked());
        break;
    }
    
    // Central object move
    case GLFW_KEY_LEFT:
    {
        glm::vec3 cur_pos(0.0f);
        cur_pos.x -= 0.3f;
        g_Engine->ObjectControl()->GetTech2ObjVec().front()[1]->Translate(cur_pos);
        break;
    }
    case GLFW_KEY_RIGHT:
    {
        glm::vec3 cur_pos(0.0f);
        cur_pos.x += 0.3f;
        g_Engine->ObjectControl()->GetTech2ObjVec().front()[1]->Translate(cur_pos);
        break;
    }
    case GLFW_KEY_UP:
    {
        glm::vec3 cur_pos(0.0f);
        cur_pos.y += 0.3f;
        g_Engine->ObjectControl()->GetTech2ObjVec().front()[1]->Translate(cur_pos);
        break;
    }
    case GLFW_KEY_DOWN:
    {
        glm::vec3 cur_pos(0.0f);
        cur_pos.y -= 0.3f;
        g_Engine->ObjectControl()->GetTech2ObjVec().front()[1]->Translate(cur_pos);
        break;
    }

    // Objects/UniBuff debug
#ifdef _DEBUG
    case GLFW_KEY_INSERT:
    {
        if (debug_obj)
        {
            REGISTER_OBJ(debug_obj);
            debug_obj = nullptr;
        }
        break;
    }
    case GLFW_KEY_DELETE:
    {
        if (!debug_obj)
        {
            debug_obj = g_Engine->ObjectControl()->GetTech2ObjVec().front()[1];
            UNREGISTER_OBJ(debug_obj);
        }
        break;
    }
#endif // _DEBUG
    }
}

void input::HandleRelease(GLFWwindow* window, const int& key, const int& scancode, const int& action, const int& mods)
{
    switch (key)
    {
    case GLFW_KEY_W:
    {
        g_Engine->Camera()->MoveFreeCam(ECamMoveDir::FORWARD, false);
        break;
    }
    case GLFW_KEY_S:
    {
        g_Engine->Camera()->MoveFreeCam(ECamMoveDir::BACKWARD, false);
        break;
    }
    case GLFW_KEY_A:
    {
        g_Engine->Camera()->MoveFreeCam(ECamMoveDir::LEFT, false);
        break;
    }
    case GLFW_KEY_D:
    {
        g_Engine->Camera()->MoveFreeCam(ECamMoveDir::RIGHT, false);
        break;
    }
    case GLFW_KEY_Q:
    {
        g_Engine->Camera()->MoveFreeCam(ECamMoveDir::DOWN, false);
        break;
    }
    case GLFW_KEY_E:
    {
        g_Engine->Camera()->MoveFreeCam(ECamMoveDir::UP, false);
        break;
    }
    case GLFW_KEY_LEFT_SHIFT:
    case GLFW_KEY_LEFT_CONTROL:
    {
        g_Engine->Camera()->SetMoveSpeed(CCamera::DEFAULT_MOVE_SPEED);
        break;
    }
    case GLFW_KEY_SPACE:
    {
        g_Engine->Camera()->SetUseFreeCam(!g_Engine->Camera()->UseFreeCam());
        break;
    }
    case GLFW_KEY_ESCAPE:
    {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        break;
    }
    }
}

void input::CursorPositionCallback(GLFWwindow* window, double xpos, double ypos)
{
    static double oldx = xpos;
    static double oldy = ypos;

    g_Engine->Camera()->ProcessMouseMoveInput(xpos - oldx, oldy - ypos);
    oldx = xpos;
    oldy = ypos;
}

void input::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    // ...
}

void input::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    g_Engine->Camera()->ChangeViewSphereRadius(yoffset);
}

void input::TryToActivateEmitter(const uint32_t& idx, const uint32_t& count)
{
    if (g_Engine->ParticleMgr()->IsEmitterActive(idx))
        g_Engine->ParticleMgr()->DeactivateEmitter(idx);
    else
        g_Engine->ParticleMgr()->ActivateEmitter(idx, count);
}