//
// StellaX
// Jeff Miller 05/06/2000
//
#ifndef GLOBALS_H
#define GLOBALS_H
#pragma once

#include "pch.hxx"

class CConfigPage;

class CGlobalData
{
    friend CConfigPage;

public:

    CGlobalData( HINSTANCE hInstance );
    ~CGlobalData( );

    BOOL ParseCommandLine( int argc, TCHAR* argv[] );

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

private:

    // Basic options

    TCHAR m_pszRomDir[ MAX_PATH ];
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


#endif
