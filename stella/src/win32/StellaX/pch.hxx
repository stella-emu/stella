//
// StellaX
// Jeff Miller 05/01/2000
//
#ifndef PCH_H
#define PCH_H

#define WIN32_LEAN_AND_MEAN
#define DIRECTINPUT_VERSION 5

#include <windows.h>
#include <windowsx.h>
#include <tchar.h>

#include <mmsystem.h>
#include <commdlg.h>
#include <commctrl.h>

#include <dinput.h>

#include "debug.hxx"

// ---------------------------------------------------------------------------
// Conditional defines

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
