//
// StellaX
// Jeff Miller 05/06/2000
//
#include "pch.hxx"
#include "GlobalData.hxx"

#include "resource.h"

static LPCTSTR g_pszIniFile = _T(".\\stella.ini");
static LPCTSTR g_pszIniSection = _T("Options");

static LPCTSTR g_pszKeyNameRomPath = _T("RomPath");
static LPCTSTR g_pszKeyNameFrameRate = _T("FrameRate");
static LPCTSTR g_pszKeyNameShowFPS = _T("ShowFPS");
static LPCTSTR g_pszKeyNameMute = _T("Mute");
static LPCTSTR g_pszKeyNamePaddle = _T("Paddle");
static LPCTSTR g_pszKeyNameDisableJoystick = _T("DisableJoystick");
static LPCTSTR g_pszKeyNameAutoSelectVideoMode = _T("AutoSelectVideoMode");

BOOL WritePrivateProfileInt(
    LPCTSTR lpAppName,  // section name
    LPCTSTR lpKeyName,  // key name
    int nValue,
    LPCTSTR lpFileName  // initialization file
    )
{
    TCHAR psz[ 50 ];

    _itoa( nValue, psz, 10 );

    return ::WritePrivateProfileString( lpAppName,
                                        lpKeyName,
                                        psz,
                                        lpFileName );
}

CGlobalData::CGlobalData(
    HINSTANCE hInstance
    ) : \
    m_hInstance(hInstance),
    m_fIsModified( FALSE )
{
    m_pszPathName[0] = _T('\0');

    //
    // Read the ROM directory from the stella.ini file
    // default to "ROMS" directory for compatibility with older StellaX
    //

    ::GetPrivateProfileString( g_pszIniSection, 
                               g_pszKeyNameRomPath, 
                               _T("ROMS"), 
                               m_pszRomDir, _MAX_PATH, 
                               g_pszIniFile);

    // Read the desired frame rate

    m_nDesiredFrameRate = (int)::GetPrivateProfileInt( g_pszIniSection, 
                                                       g_pszKeyNameFrameRate, 
                                                       60, 
                                                       g_pszIniFile );
    if (m_nDesiredFrameRate < 1 || m_nDesiredFrameRate > 300)
    {
        m_nDesiredFrameRate = 60;
    }

    // Read ShowFPS

    m_fShowFPS = (BOOL)::GetPrivateProfileInt( g_pszIniSection, 
                                               g_pszKeyNameShowFPS,
                                               FALSE, 
                                               g_pszIniFile);

    //
    // Read Mute
    //

    m_fNoSound = (BOOL)::GetPrivateProfileInt( g_pszIniSection, 
                                               g_pszKeyNameMute, 
                                               FALSE, 
                                               g_pszIniFile );

    //
    // Get AutoSelectVideoMode
    //

    m_fAutoSelectVideoMode = 
        (BOOL)::GetPrivateProfileInt( g_pszIniSection, 
                                      g_pszKeyNameAutoSelectVideoMode, 
                                      TRUE, 
                                      g_pszIniFile );

    //
    // Read the Paddle mode
    //

    m_nPaddleMode = (int)::GetPrivateProfileInt( g_pszIniSection, 
                                                 g_pszKeyNamePaddle,
                                                 0, 
                                                 g_pszIniFile);
    if ( m_nPaddleMode < 0 || m_nPaddleMode > 3 )
    {
        m_nPaddleMode = 0;
    }

    // Read DisableJoystick

    m_fDisableJoystick = (BOOL)::GetPrivateProfileInt( g_pszIniSection,
                                                       g_pszKeyNameDisableJoystick, 
                                                       FALSE, 
                                                       g_pszIniFile );

}

CGlobalData::~CGlobalData(
    )
{
    // 
    // Write out settings (if changed)
    //

    if ( m_fIsModified )
    {
        ::WritePrivateProfileString( g_pszIniSection,
                                     g_pszKeyNameRomPath,
                                     m_pszRomDir,
                                     g_pszIniFile );

        ::WritePrivateProfileInt( g_pszIniSection,
                                  g_pszKeyNameFrameRate,
                                  m_nDesiredFrameRate,
                                  g_pszIniFile );

        ::WritePrivateProfileInt( g_pszIniSection,
                                  g_pszKeyNameMute,
                                  m_fNoSound,
                                  g_pszIniFile );

        ::WritePrivateProfileInt( g_pszIniSection,
                                  g_pszKeyNameAutoSelectVideoMode,
                                  m_fAutoSelectVideoMode,
                                  g_pszIniFile );

        ::WritePrivateProfileInt( g_pszIniSection,
                                  g_pszKeyNamePaddle,
                                  m_nPaddleMode,
                                  g_pszIniFile );

        ::WritePrivateProfileInt( g_pszIniSection,
                                  g_pszKeyNameDisableJoystick,
                                  m_fDisableJoystick,
                                  g_pszIniFile );
    }
}