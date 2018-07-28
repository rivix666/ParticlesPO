#include "stdafx.h"
#include "InputsListener.h"

bool CInputsListener::RegisterInputDevices()
{
    RAWINPUTDEVICE inputDevices[2];

    //adds mouse and allow legacy messages
    inputDevices[0].usUsagePage = 0x01;
    inputDevices[0].usUsage = 0x02;
    inputDevices[0].dwFlags = 0;
    inputDevices[0].hwndTarget = 0;

    //adds keyboard and allow legacy messages
    inputDevices[1].usUsagePage = 0x01;
    inputDevices[1].usUsage = 0x06;
    inputDevices[1].dwFlags = 0;
    inputDevices[1].hwndTarget = 0;

    if (RegisterRawInputDevices(inputDevices, 2, sizeof(inputDevices[0])) == FALSE)
    {
        return false;
    }

    return true;
}

void CInputsListener::HandleRawInput(HRAWINPUT &lParam)
{
    //get raw input data buffer size
    UINT dbSize;
    GetRawInputData(lParam, RID_INPUT, NULL, &dbSize, sizeof(RAWINPUTHEADER));

    //allocate memory for raw input data and get data
    BYTE* buffer = new BYTE[dbSize];
    GetRawInputData((HRAWINPUT)lParam, RID_INPUT, buffer, &dbSize, sizeof(RAWINPUTHEADER));
    RAWINPUT* raw = (RAWINPUT*)buffer;

    // Handle Keyboard Input
    //---------------------------------------------------------------------------
    if (raw->header.dwType == RIM_TYPEKEYBOARD)
    {
        switch (raw->data.keyboard.Message)
        {
            //key down
        case WM_KEYDOWN:
            switch (raw->data.keyboard.VKey)
            {
            case VK_ESCAPE: PostQuitMessage(0);
                break;
            case VK_OEM_PLUS: g_Engine->GetCam()->ChangeViewSphereRadius(0.5f);
                break;
            case VK_OEM_MINUS:  g_Engine->GetCam()->ChangeViewSphereRadius(-0.5f);
                break;
            case 'P': g_Engine->SetPhysXPause(!g_Engine->IsPhysXPaused());
                break;
            case '1': 
                g_Engine->m_ActEmittedId = 0; //SMOKE
                g_Engine->m_ActRandomVel = true;
                break;
            case '2':
                g_Engine->m_ActEmittedId = 1; //FLAME
                g_Engine->m_ActRandomVel = true;
                break;
            case '3':
                g_Engine->m_ActEmittedId = 2; //SHOCK-WAVE
                g_Engine->m_ActRandomVel = false;
                break;
            case '4':
                g_Engine->m_ActEmittedId = 3; //FLASH
                g_Engine->m_ActRandomVel = true;
                break;
            case '5':
                g_Engine->m_ActEmittedId = 4; //FRAGS
                g_Engine->m_ActRandomVel = true;
                break;
            case '6':
                g_Engine->m_ActEmittedId = 5; //SPARK
                g_Engine->m_ActRandomVel = true;
                break;
            case '7':
                g_Engine->m_ActEmittedId = 6; //LIGHTNING
                g_Engine->m_ActRandomVel = false;
                break;
            case '8':
                g_Engine->EmitExplosion(0); //Explosion1
                break;
            case '9':
                g_Engine->EmitExplosion(1); //FireWorks
                break;
            case '0':
                g_Engine->m_ActEmittedId = -1; //stop
                break;
            case 'V':
                g_Engine->m_RestrictTo60FPS = !g_Engine->m_RestrictTo60FPS;
                break;
            case VK_F1:
                g_Engine->m_SortOnGPU = true;
                break;
            case VK_F2:
                g_Engine->m_SortOnGPU = false;
                break;
            }
            break;

            //key up
        case WM_KEYUP:
            switch (raw->data.keyboard.VKey)
            {
            }
            break;
        }
    }

    // Handle Mouse Input
    //---------------------------------------------------------------------------
    else if (raw->header.dwType == RIM_TYPEMOUSE)
    {
        //mouse move
        if (isLMBclicked)
        {
            g_Engine->GetCam()->MoveCamSpherical(0.0025f * raw->data.mouse.lLastY, 0.0025f * raw->data.mouse.lLastX);
        }

        if (isRMBclicked)
        {
            g_Engine->GetCam()->AddToView(0.0f, 0.0025f * raw->data.mouse.lLastY, 0.0f);
        }

        //mouse buttons detect
        switch (raw->data.mouse.ulButtons){
        case RI_MOUSE_LEFT_BUTTON_DOWN:
            isLMBclicked = true;
            break;
        case RI_MOUSE_RIGHT_BUTTON_DOWN:
            isRMBclicked = true;
            break;
        case RI_MOUSE_LEFT_BUTTON_UP:
            isLMBclicked = false;
            break;
        case RI_MOUSE_RIGHT_BUTTON_UP:
            isRMBclicked = false;
            break;
        }

        if (raw->data.mouse.usButtonFlags == RI_MOUSE_WHEEL){

            //without projection we get 120 or 65400...
            short wheelTurnDetect = (short)(unsigned short)raw->data.mouse.usButtonData;
            g_Engine->GetCam()->ChangeViewSphereRadius(wheelTurnDetect * 0.0025f);
            if (wheelTurnDetect > 0){

            }
            else{

            }
        }
    }
    //free allocated memory
    delete[] buffer;
}