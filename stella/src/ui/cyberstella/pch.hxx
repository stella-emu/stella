//
// StellaX
// Jeff Miller 05/01/2000
//
#ifndef __PCH_H__
#define __PCH_H__
#pragma once

#ifndef _WIN32
#error This file can only be compiled for a Win32 platform
#endif

#define WIN32_LEAN_AND_MEAN

//#include <windows.h>
#include <windowsx.h>
#include <tchar.h>

// warning C4201: nonstandard extension used : nameless struct/union

#pragma warning ( once: 4201 )

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT


#include <mmsystem.h>
#include <commdlg.h>
#include <commctrl.h>

#include <ddraw.h>
#include <dsound.h>
#include <dinput.h>

// Stella Messages:
#define MSG_GAMELIST_UPDATE         WM_USER+0x1000
#define MSG_GAMELIST_DISPLAYNOTE    WM_USER+0x1001
#define MSG_VIEW_INITIALIZE         WM_USER+0x1002

#define _countof(array) (sizeof(array)/sizeof(array[0]))

// ---------------------------------------------------------------------------
// Conditional defines

#define DOUBLE_WIDTH

//
// Macros
//

#ifdef _DEBUG
#define UNUSED(x)
#else
#define UNUSED(x) x
#endif
#define UNUSED_ALWAYS(x) x

//
// Simple string class
//

class CSimpleString
{
public:

    CSimpleString() : 
        m_psz( NULL ),
        m_cch( -1 )
        {
        }

    ~CSimpleString()
        {
            delete[] m_psz;
            m_psz = NULL;

            m_cch = -1;
        }

    BOOL Set( LPCTSTR psz )
        {
            int cch = lstrlen( psz );
            if ( cch > m_cch )
            {
                delete[] m_psz;
                m_psz = NULL;
                m_cch = -1;

                m_psz = new TCHAR[ cch + 1 ];
                if ( m_psz == NULL )
                {
                    return FALSE;
                }

                m_cch = cch;
            }

            memcpy( m_psz, psz, ( cch + 1 ) * sizeof( TCHAR ) );

            return TRUE;
        }

    LPCTSTR Get( void ) const
        {
            ASSERT( m_psz != NULL );
            return m_psz;
        }

    int Length( void ) const
        {
            return m_cch;
        }

private:

    //
    // The string and its size (-1 means not initialized)
    //

    LPTSTR m_psz;
    int m_cch;

	CSimpleString( const CSimpleString& );  // no implementation
	void operator=( const CSimpleString& );  // no implementation
};

//
// Utility methods
//

void MessageBox(
    HINSTANCE hInstance,
    HWND hwndParent,
    UINT uIDText
    );

void MessageBoxFromWinError(
    DWORD dwError,
    LPCTSTR pszCaption /* = NULL */
    );

void MessageBoxFromGetLastError(
    LPCTSTR pszCaption /* = NULL */
    );

#endif
