//
// StellaX
// Jeff Miller 05/06/2000
//
#ifndef GLOBALS_H
#define GLOBALS_H
#pragma once

#include "Event.hxx"
#include "CRegBinding.h"

class CGlobalData
{

public:

    CGlobalData(HINSTANCE hInstance);
    ~CGlobalData() {}

    Event::Type PaddleResistanceEvent( void ) const;
    Event::Type PaddleFireEvent( void ) const;
    
    // Data Members in Registry
    int iPaddleMode;
    BOOL bNoSound;
    BOOL bJoystickIsDisabled;
    BOOL bAutoSelectVideoMode;
    BOOL bShowFPS;
    int desiredFrameRate;

    // Data Members not in Registry
    HINSTANCE instance;
    BOOL bIsModified;

    // Regbinding
    CRegBinding   rs;
};

inline Event::Type CGlobalData::PaddleResistanceEvent(
    void
    ) const
{
    switch (iPaddleMode)
    {
    case 1:
        return Event::PaddleOneResistance;
      
    case 2:
        return Event::PaddleTwoResistance;

    case 3:
        return Event::PaddleThreeResistance;
    }

    return Event::PaddleZeroResistance;
}

inline Event::Type CGlobalData::PaddleFireEvent(
    void
    ) const
{
    switch (iPaddleMode)
    {
    case 1:
        return Event::PaddleOneFire;

    case 2:
        return Event::PaddleTwoFire;

    case 3:
        return Event::PaddleThreeFire;
    }

    return Event::PaddleZeroFire;
}

#endif
