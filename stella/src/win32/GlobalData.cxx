//============================================================================
//
//   SSSS    tt          lll  lll       
//  SS  SS   tt           ll   ll        
//  SS     tttttt  eeee   ll   ll   aaaa 
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2000 by Jeff Miller
// Copyright (c) 2004 by Stephen Anthony
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: GlobalData.cxx,v 1.2 2004-05-27 22:02:35 stephena Exp $
//============================================================================ 

#include "pch.hxx"
#include "GlobalData.hxx"
#include "resource.h"

static LPCTSTR g_pszIniFile = _T(".\\stellax.ini");
static LPCTSTR g_pszIniSection = _T("Options");

static LPCTSTR g_pszKeyNameRomPath = _T("RomPath");
static LPCTSTR g_pszKeyNameFrameRate = _T("FrameRate");
static LPCTSTR g_pszKeyNameMute = _T("Mute");
static LPCTSTR g_pszKeyNamePaddle = _T("Paddle");

BOOL WritePrivateProfileInt(
    LPCTSTR lpAppName,  // section name
    LPCTSTR lpKeyName,  // key name
    int nValue,
    LPCTSTR lpFileName  // initialization file
    )
{
  TCHAR psz[ 50 ];

  _itoa( nValue, psz, 10 );

  return ::WritePrivateProfileString( lpAppName, lpKeyName, psz, lpFileName );
}

CGlobalData::CGlobalData( HINSTANCE hInstance )
           : m_hInstance(hInstance),
             m_fIsModified( FALSE )
{
  m_pszPathName[0] = _T('\0');

  // Read the ROM directory from the stella.ini file
  // default to "ROMS" directory for compatibility with older StellaX
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
    m_nDesiredFrameRate = 60;

  // Read Mute
  m_fNoSound = (BOOL)::GetPrivateProfileInt( g_pszIniSection, 
                                             g_pszKeyNameMute, 
                                             FALSE, 
                                             g_pszIniFile );

  // Read the Paddle mode
  m_nPaddleMode = (int)::GetPrivateProfileInt( g_pszIniSection, 
                                               g_pszKeyNamePaddle,
                                               0, 
                                               g_pszIniFile);
  if ( m_nPaddleMode < 0 || m_nPaddleMode > 3 )
    m_nPaddleMode = 0;

}

CGlobalData::~CGlobalData()
{
  // Write out settings (if changed)
  if ( m_fIsModified )
  {
    // RomPath
    ::WritePrivateProfileString( g_pszIniSection,
                                 g_pszKeyNameRomPath,
                                 m_pszRomDir,
                                 g_pszIniFile );

    // FrameRate
    ::WritePrivateProfileInt( g_pszIniSection,
                              g_pszKeyNameFrameRate,
                              m_nDesiredFrameRate,
                              g_pszIniFile );

    // Mute
    ::WritePrivateProfileInt( g_pszIniSection,
                              g_pszKeyNameMute,
                              m_fNoSound,
                              g_pszIniFile );

    // Paddle
    ::WritePrivateProfileInt( g_pszIniSection,
                              g_pszKeyNamePaddle,
                              m_nPaddleMode,
                              g_pszIniFile );
  }
}

BOOL CGlobalData::ParseCommandLine(
    int argc,
    TCHAR** argv
    )
{
    // parse arguments

    for (int i = 1; i < argc; ++i)
    {
        LPCTSTR ctszArg = argv[i];

        if (ctszArg && (ctszArg[0] != _T('-')))
        {
            // assume this is the start rom name

            lstrcpy( m_pszPathName, ctszArg );
        }
        else
        {
            return FALSE;
        }
    }

    return TRUE;
}