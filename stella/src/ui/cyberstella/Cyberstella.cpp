// Cyberstella.cpp : Defines the class behaviors for the application.
//

#include "pch.hxx"
#include "Cyberstella.h"

#include "MainFrm.h"
#include "CyberstellaDoc.h"
#include "CyberstellaView.h"
#include "AboutDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCyberstellaApp

BEGIN_MESSAGE_MAP(CCyberstellaApp, CWinApp)
	//{{AFX_MSG_MAP(CCyberstellaApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCyberstellaApp construction

CCyberstellaApp::CCyberstellaApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CCyberstellaApp object

CCyberstellaApp theApp;

// see debug.cpp
LPCTSTR g_ctszDebugLog = _T("stella.log");

/////////////////////////////////////////////////////////////////////////////
// CCyberstellaApp initialization

BOOL CCyberstellaApp::InitInstance()
{
	// Delete previous Debug Log
    (void)::DeleteFile(g_ctszDebugLog);

	// Avoid Second instance
	CreateMutex(NULL,TRUE,_T("StellaXMutex"));

    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        MessageBox(NULL, NULL, IDS_ALREADYRUNNING);
        return false;
    }
    
    // Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	// Change the registry key under which our settings are stored.
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization.
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));

	LoadStdProfileSettings(10);  // Load standard INI file options (including MRU)

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.

	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CCyberstellaDoc),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		RUNTIME_CLASS(CCyberstellaView));
	AddDocTemplate(pDocTemplate);

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	// Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	// The one and only window has been initialized, so show and update it.
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();

	return TRUE;
}

// App command to run the about dialog
void CCyberstellaApp::OnAppAbout()
{
	AboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CCyberstellaApp message handlers


void CCyberstellaApp::OnViewToolbar() 
{
	// TODO: Add your command handler code here
	
}
