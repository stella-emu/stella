// CyberstellaView.cpp : implementation of the CCyberstellaView class
//

#include "pch.hxx"
#include "Cyberstella.h"
#include "MainFrm.h"
#include "CyberstellaDoc.h"
#include "CyberstellaView.h"
#include "StellaConfig.h"
#include "Console.hxx"
#include "MainWin32.hxx"
#include "SoundWin32.hxx"
#include "SettingsWin32.hxx"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCyberstellaView

IMPLEMENT_DYNCREATE(CCyberstellaView, CFormView)

BEGIN_MESSAGE_MAP(CCyberstellaView, CFormView)
	//{{AFX_MSG_MAP(CCyberstellaView)
	ON_BN_CLICKED(IDC_CONFIG, OnConfig)
	ON_BN_CLICKED(IDC_PLAY, OnPlay)
	ON_WM_DESTROY()
    ON_MESSAGE(MSG_GAMELIST_UPDATE, updateListInfos)
    ON_MESSAGE(MSG_GAMELIST_DISPLAYNOTE, displayNote)
    ON_MESSAGE(MSG_VIEW_INITIALIZE, initialize)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCyberstellaView construction/destruction

CCyberstellaView::CCyberstellaView()
	: CFormView(CCyberstellaView::IDD)
{
	//{{AFX_DATA_INIT(CCyberstellaView)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

  // Create SettingsWin32 object
  // This should be done before any other xxxWin32 objects are created
  theSettings = new SettingsWin32();
  theSettings->loadConfig();

  // Create a properties set for us to use
  thePropertiesSet = new PropertiesSet();

  // Try to load the file stella.pro file
  string filename(theSettings->userPropertiesFilename());
    
  // See if we can open the file and load properties from it
  ifstream stream(filename.c_str()); 
  if(stream)
  {
    // File was opened so load properties from it
    stream.close();
    thePropertiesSet->load(filename, &Console::defaultProperties());
  }
  else
  {
    thePropertiesSet->load("", &Console::defaultProperties());
    MessageBox("stella.pro not found in working directory!", "Warning!", MB_OK|MB_ICONEXCLAMATION);
  }
}

CCyberstellaView::~CCyberstellaView()
{
  if(thePropertiesSet)
    delete thePropertiesSet;

  if(theSettings)
  {
    theSettings->saveConfig();
    delete theSettings;
  }
}

void CCyberstellaView::DoDataExchange(CDataExchange* pDX)
{
    CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCyberstellaView)
	DDX_Control(pDX, IDC_ROMLIST, m_List);
	//}}AFX_DATA_MAP
}

BOOL CCyberstellaView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CFormView::PreCreateWindow(cs);
}

void CCyberstellaView::OnInitialUpdate()
{
  CFormView::OnInitialUpdate();
  GetParentFrame()->RecalcLayout();
  ResizeParentToFit();

    // Init ListControl, parse stella.pro
    PostMessage(MSG_VIEW_INITIALIZE);
}

/////////////////////////////////////////////////////////////////////////////
// CCyberstellaView diagnostics

#ifdef _DEBUG
void CCyberstellaView::AssertValid() const
{
	CFormView::AssertValid();
}

void CCyberstellaView::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}

CCyberstellaDoc* CCyberstellaView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CCyberstellaDoc)));
	return (CCyberstellaDoc*)m_pDocument;
}
#endif //_DEBUG

void CCyberstellaView::OnConfig() 
{
  StellaConfig dlg(theSettings);
  dlg.DoModal();
}

void CCyberstellaView::OnPlay() 
{
    playRom();
}

LRESULT CCyberstellaView::initialize(WPARAM wParam, LPARAM lParam)
{
  // Set up the image list.
  HICON hFolder, hAtari;

  m_imglist.Create( 16, 16, ILC_COLOR16 | ILC_MASK, 4, 1 );

  hFolder = reinterpret_cast<HICON>(
              ::LoadImage ( AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_FOLDER),
                            IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR ));
  hAtari = reinterpret_cast<HICON>(
              ::LoadImage ( AfxGetResourceHandle(), MAKEINTRESOURCE(IDR_MAINFRAME),
                            IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR ));

  m_imglist.Add (hFolder);
  m_imglist.Add (hAtari);

  m_List.SetImageList (&m_imglist, LVSIL_SMALL);

  // Init ListCtrl
  m_List.init(thePropertiesSet, theSettings, this);
  m_List.populateRomList();

  return 0;
}

void CCyberstellaView::OnDestroy() 
{
  CFormView::OnDestroy();
  m_List.deleteItemsAndProperties();
}

LRESULT CCyberstellaView::updateListInfos(WPARAM wParam, LPARAM lParam)
{
  // Show status text
  CString status;
  status.Format(IDS_STATUSTEXT, m_List.getRomCount());
  SetDlgItemText(IDC_ROMCOUNT,status);

  // Show rom path
  SetDlgItemText(IDC_ROMPATH, m_List.getPath());

  return 0;
}

LRESULT CCyberstellaView::displayNote(WPARAM wParam, LPARAM lParam)
{
  // Show rom path
  CString note;
  note.Format(IDS_NOTETEXT, m_List.getCurrentNote());
  ((CMainFrame*)AfxGetMainWnd())->setStatusText(note);

  return 0;    
}

void CCyberstellaView::playRom(LONG gameID)
{
  EnableWindow(FALSE);

  CString fileName;
  BYTE* pImage = NULL;
  LPCTSTR pszFileName = NULL;
  DWORD dwImageSize;
  DWORD dwActualSize;

  fileName = m_List.getCurrentFile();
  if(fileName.GetLength() <= 0)
    return;

  // Load the rom file
  HANDLE hFile;
  hFile = ::CreateFile( fileName, GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );

  if(hFile == INVALID_HANDLE_VALUE)
  {
    DWORD dwLastError = ::GetLastError();

    TCHAR pszCurrentDirectory[ MAX_PATH + 1 ];
    ::GetCurrentDirectory( MAX_PATH, pszCurrentDirectory );

    // ::MessageBoxFromGetLastError( pszPathName );
    TCHAR pszFormat[ 1024 ];
    LoadString(GetModuleHandle(NULL), IDS_ROM_LOAD_FAILED, pszFormat, 1023 );

    LPTSTR pszLastError = NULL;

    FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL, 
            dwLastError, 
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR)&pszLastError, 
            0, 
            NULL);

    TCHAR pszError[ 1024 ];
    wsprintf( pszError, 
              pszFormat, 
              pszCurrentDirectory,
              fileName, 
              dwLastError,
              pszLastError );

    ::MessageBox( *this, 
                  pszError, 
                  _T("Error"),
                  MB_OK | MB_ICONEXCLAMATION );

    ::LocalFree( pszLastError );
    return;
  }

  dwImageSize = ::GetFileSize( hFile, NULL );

  pImage = new BYTE[dwImageSize + 1];
  if(pImage == NULL)
    return;

  if ( ! ::ReadFile( hFile, pImage, dwImageSize, &dwActualSize, NULL ) )
  {
    VERIFY( ::CloseHandle( hFile ) );
    MessageBoxFromGetLastError(fileName);
    delete pImage;
    return;
  }

  VERIFY( ::CloseHandle(hFile) );

  // get just the filename
  pszFileName = _tcsrchr( fileName, _T('\\') );
  if ( pszFileName )
  {
    ++pszFileName;
  }
  else
  {
    pszFileName = fileName;
  }

  ::ShowWindow( *this, SW_HIDE );

  // Create a new main instance for this cartridge
  MainWin32* mainWin32 = new MainWin32(pImage, dwActualSize, pszFileName,
                          *theSettings, *thePropertiesSet);
  // And start the main emulation loop
  mainWin32->run();

  ::ShowWindow( *this, SW_SHOW );
  ShowCursor(TRUE);
  
  delete pImage;
  delete mainWin32;

  EnableWindow(TRUE);

  // Set focus back to the rom list
  m_List.SetFocus();
}
