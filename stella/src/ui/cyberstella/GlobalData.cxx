//
// StellaX
// Jeff Miller 05/06/2000
//
#include "pch.hxx"
#include "GlobalData.hxx"
#include "resource.h"

CGlobalData::CGlobalData(HINSTANCE hInstance)
            : instance(hInstance)
              ,bIsModified(FALSE)
              ,rs("GlobalData")
{
    rs.Bind(desiredFrameRate, "desired frame rate", 60);
    rs.Bind(bShowFPS, "Show FPS", false);
    rs.Bind(bNoSound, "No Sound", false);
    rs.Bind(bAutoSelectVideoMode, "Autoselect Video Mode", true);
    rs.Bind(iPaddleMode, "Paddle Mode", 0);
    rs.Bind(bJoystickIsDisabled, "Joystick Is Disabled", false);

    if (desiredFrameRate < 1 || desiredFrameRate > 300)
    {
        desiredFrameRate = 60;
    }

    if (iPaddleMode<0 || iPaddleMode>3)
    {
        iPaddleMode = 0;
    }
}