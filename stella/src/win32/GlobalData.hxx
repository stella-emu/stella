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
// $Id: GlobalData.hxx,v 1.2 2004-05-27 22:02:35 stephena Exp $
//============================================================================ 

#ifndef GLOBALS_H
#define GLOBALS_H

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

    // Booleans
    BOOL NoSound() const
    {
      return m_fNoSound;
    }

    int PaddleMode( void ) const
    {
      return m_nPaddleMode;
    }
    
    LPCTSTR PathName( void ) const
    {
      if ( m_pszPathName[0] == _T('\0') )
        return NULL;

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

    // Modified flags
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

    // Advanced options
    int m_nDesiredFrameRate;

    HINSTANCE m_hInstance;
    TCHAR m_pszPathName[ MAX_PATH ];

    BOOL m_fIsModified;

    CGlobalData( const CGlobalData& );  // no implementation
    void operator=( const CGlobalData& );  // no implementation
};

#endif
