//
// StellaX
// Jeff Miller 05/01/2000
//
#include "pch.hxx"
#include <stdio.h>
#include <stdarg.h>
#include "resource.h"

// This will force in the DirectX library

#pragma comment( lib, "dxguid" )

// Bring in DirectInput library (for c_dfDIMouse, etc)

#pragma comment( lib, "dinput" )

// Bring in the common control library

#pragma comment( lib, "comctl32" )

// Bring in multimedia timers

#pragma comment( lib, "winmm" )

void MessageBox(
    HINSTANCE hInstance,
    HWND hwndParent,
    UINT uIDText
    )
{
    const int nMaxStrLen = 1024;
    TCHAR tszCaption[nMaxStrLen + 1] = { 0 };
    TCHAR tszText[nMaxStrLen + 1] = { 0 };

    // Caption is always "StellaX"

    LoadString(hInstance, IDS_STELLA, tszCaption, nMaxStrLen);

    LoadString(hInstance, uIDText, tszText, nMaxStrLen);

    if (hwndParent == NULL)
    {
        hwndParent = ::GetForegroundWindow();
    }

    ::MessageBox(hwndParent, tszText, tszCaption, MB_ICONWARNING | MB_OK);
}

void MessageBoxFromWinError(
    DWORD dwError,
    LPCTSTR pszCaption /* = NULL */
    )
{
    const int nMaxStrLen = 1024;
    TCHAR pszCaptionStellaX[nMaxStrLen + 1];

    if ( pszCaption == NULL )
    {
        // LoadString(hInstance, IDS_STELLA, tszCaption, nMaxStrLen);
        lstrcpy( pszCaptionStellaX, _T("StellaX") );
    }

    LPTSTR pszText = NULL;

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, 
        dwError, 
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&pszText, 
        0, 
        NULL);

    ::MessageBox(::GetForegroundWindow(), pszText, 
        pszCaption ? pszCaption : pszCaptionStellaX, MB_ICONWARNING | MB_OK);

    ::LocalFree( pszText );
}

void MessageBoxFromGetLastError(
    LPCTSTR pszCaption /* = NULL */
    )
{
    MessageBoxFromWinError( GetLastError(), pszCaption );
}

