#pragma once
#include "Engine.h"

class CInputsListener
{
public:
    bool RegisterInputDevices();
    void HandleRawInput(HRAWINPUT &lParam);

private:
    bool isLMBclicked;
    bool isRMBclicked;
};

