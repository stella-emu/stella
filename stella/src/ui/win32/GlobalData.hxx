//
// StellaX
// Jeff Miller 05/06/2000
//
#ifndef GLOBALS_H
#define GLOBALS_H
#pragma once

#include "Event.hxx"

class CConfigPage;

class CGlobalData
{
    friend CConfigPage;

public:

    CGlobalData(HINSTANCE hInstance);
    ~CGlobalData();

    int DesiredFrameRate( void ) const
        {
            return m_nDesiredFrameRate;
        }

    //
    // Booleans
    //

    BOOL ShowFPS( void ) const
        {
            return m_fShowFPS;
        }

    BOOL NoSound() const
        {
            return m_fNoSound;
        }

    BOOL DisableJoystick( void ) const
        {
            return m_fDisableJoystick;
        }

    BOOL AutoSelectVideoMode( void ) const
        {
            return m_fAutoSelectVideoMode;
        }

    int PaddleMode( void ) const;
    
    Event::Type PaddleResistanceEvent( void ) const;

    Event::Type PaddleFireEvent( void ) const;
    
    LPCTSTR PathName( void ) const
    {
        if ( m_pszPathName[0] == _T('\0') )
        {
            return NULL;
        }
        
        return m_pszPathName;
    }

    LPCTSTR RomDir( void ) const
        {
            return m_pszRomDir;
        }
    
    HINSTANCE ModuleInstance( void ) const
        {
            return m_hInstance;
        }

    //
    // Modified flags
    //

    void SetModified( void )
        {
            m_fIsModified = TRUE;
        }

    BOOL IsModified( void ) const
        {
            return m_fIsModified;
        }

    // Basic options
    TCHAR* m_pszRomDir;
    int m_nPaddleMode;
    BOOL m_fNoSound;
    BOOL m_fDisableJoystick;

    // Advanced options

    BOOL m_fShowFPS;
    int m_nDesiredFrameRate;
    BOOL m_fAutoSelectVideoMode;


    HINSTANCE m_hInstance;
    TCHAR m_pszPathName[ MAX_PATH ];

    BOOL m_fIsModified;

	CGlobalData( const CGlobalData& );  // no implementation
	void operator=( const CGlobalData& );  // no implementation
};

inline int CGlobalData::PaddleMode(
    void
    ) const
{
    return m_nPaddleMode;
}

inline Event::Type CGlobalData::PaddleResistanceEvent(
    void
    ) const
{
    switch ( m_nPaddleMode )
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
    switch ( m_nPaddleMode )
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
