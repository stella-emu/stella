/*
    StellaX
    Win32 DirectX port of Stella

    Originally written by Jeff Miller
    Continued by Manuel Polik

    Stella core developed by Bradford W. Mott
*/

#include "pch.hxx"
#include "resource.h"
#include "MainDlg.hxx"

// see debug.cpp
LPCTSTR g_ctszDebugLog = _T("stella.log");

int WINAPI _tWinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance
					 ,LPTSTR lpCmdLine,int nCmdShow)
{
	// Delete previous Debug Log
    (void)::DeleteFile(g_ctszDebugLog);

	// Avoid Second instance
	CreateMutex(NULL,TRUE,_T("StellaXMutex"));

    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        MessageBox( hInstance, NULL, IDS_ALREADYRUNNING );
        return 1;
    }

    HRESULT hrCoInit = ::CoInitialize( NULL );
    if ( FAILED(hrCoInit) )
    {
        MessageBox( hInstance, NULL, IDS_COINIT_FAILED );
    }

    ::InitCommonControls();

    CMainDlg dlg(hInstance);
    dlg.DoModal(NULL);

    if ( hrCoInit == S_OK )
    {
        ::CoUninitialize();
    }

    return 0;
}
