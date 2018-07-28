#include "stdafx.h"
#include "InputsListener.h"


//#UNI_BUFF
//////////////////////////////////////////////////////////////////////////
#include "GObjectControl.h"
#include "TechniqueManager.h"
#include "IGObject.h"

#include "GBaseObject.h" //#UNI_BUFF
//////////////////////////////////////////////////////////////////////////




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
    switch (key)
    {
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
        g_Engine->Camera()->SetMoveSpeed(0.3f);
        break;
    }
    case GLFW_KEY_LEFT_CONTROL:
    {
        g_Engine->Camera()->SetMoveSpeed(0.005f);
        break;
    }
    // #UNI_BUFF DEBUG
    //////////////////////////////////////////////////////////////////////////
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
    //////////////////////////////////////////////////////////////////////////
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
