// CyberstellaView.cpp : implementation of the CCyberstellaView class
//

#include "pch.hxx"
#include "Cyberstella.h"
#include "MainFrm.h"
#include "CyberstellaDoc.h"
#include "CyberstellaView.h"
#include "StellaConfig.h"
#include "Console.hxx"
#include "SoundWin32.hxx"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Undefining USE_FS will use the (untested) windowed mode
//

#define USE_FS

#ifdef USE_FS
#include "DirectXFullScreen.hxx"
#else
#include "DirectXWindow.hxx"
#endif

#define FORCED_VIDEO_CX 640
#define FORCED_VIDEO_CY 480

/////////////////////////////////////////////////////////////////////////////
// CCyberstellaView

IMPLEMENT_DYNCREATE(CCyberstellaView, CFormView)

BEGIN_MESSAGE_MAP(CCyberstellaView, CFormView)
	//{{AFX_MSG_MAP(CCyberstellaView)
	ON_BN_CLICKED(IDC_CONFIG, OnConfig)
	ON_BN_CLICKED(IDC_PLAY, OnPlay)
	ON_WM_DESTROY()
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
    m_pGlobalData = new CGlobalData(GetModuleHandle(NULL));
    m_bIsPause = false;
    m_pPropertiesSet = NULL;
}

CCyberstellaView::~CCyberstellaView()
{
    if(m_pGlobalData) delete m_pGlobalData;
    if(m_pPropertiesSet) delete m_pPropertiesSet;
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
    Initialize();

    // Show status text
    CString status;
    status.Format(IDS_STATUSTEXT, m_List.GetItemCount());
    SetDlgItemText(IDC_ROMCOUNT,status);

    // Show rom path
    //ToDo: SetDlgItemText(IDC_ROMPATH, m_pGlobalData->romDir);
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
    StellaConfig dlg(m_pGlobalData);
    dlg.DoModal();
}

void CCyberstellaView::OnPlay() 
{
    EnableWindow(FALSE);

    CString fileName = m_List.getCurrentFile();

    // Safety Bail Out
    if(fileName.GetLength() <= 0)   return;

#ifdef USE_FS
    CDirectXFullScreen* pwnd = NULL;
#else
    CDirectXWindow* pwnd = NULL;
#endif

    BYTE* pImage = NULL;
    LPCTSTR pszFileName = NULL;
    Console* pConsole = NULL;
    Sound* pSound = NULL;
    Event rEvent;

    // show wait cursor while loading
    HCURSOR hcur = ::SetCursor(::LoadCursor(NULL, IDC_WAIT));

    // Load the rom file
    HANDLE hFile;
    DWORD dwImageSize;
    hFile = ::CreateFile( fileName, 
                          GENERIC_READ, 
                          FILE_SHARE_READ, 
                          NULL, 
                          OPEN_EXISTING, 
                          FILE_ATTRIBUTE_NORMAL,
                          NULL );
    if (hFile == INVALID_HANDLE_VALUE)
    {
        DWORD dwLastError = ::GetLastError();

        TCHAR pszCurrentDirectory[ MAX_PATH + 1 ];
        ::GetCurrentDirectory( MAX_PATH, pszCurrentDirectory );

        // ::MessageBoxFromGetLastError( pszPathName );
        TCHAR pszFormat[ 1024 ];
        LoadString(GetModuleHandle(NULL),
                    IDS_ROM_LOAD_FAILED,
                    pszFormat, 1023 );

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

        goto exit;
    }

    dwImageSize = ::GetFileSize( hFile, NULL );

    pImage = new BYTE[dwImageSize + 1];
    if ( pImage == NULL )
    {
        goto exit;
    }

    DWORD dwActualSize;
    if ( ! ::ReadFile( hFile, pImage, dwImageSize, &dwActualSize, NULL ) )
    {
        VERIFY( ::CloseHandle( hFile ) );

        MessageBoxFromGetLastError(fileName);

        goto exit;
    }

    VERIFY( ::CloseHandle(hFile) );

    //
    // Create Sound driver object
    // (Will be initialized once we have a window handle below)
    //

    if (m_pGlobalData->bNoSound)
    {
        TRACE("Creating Sound driver");
        pSound = new Sound;
    }
    else
    {
        TRACE("Creating SoundWin32 driver");
        pSound = new SoundWin32;
    }
    if ( pSound == NULL )
    {
        goto exit;
    }

    //
    // get just the filename
    //

    pszFileName = _tcsrchr( fileName, _T('\\') );
    if ( pszFileName )
    {
        ++pszFileName;
    }
    else
    {
        pszFileName = fileName;
    }

    try
    {
        // If this throws an exception, then it's probably a bad cartridge

        pConsole = new Console( pImage, 
                                dwActualSize,
                                pszFileName, 
                                rEvent, 
                                *m_pPropertiesSet, 
                                *pSound );
        if ( pConsole == NULL )
        {
            goto exit;
        }
    }
    catch (...)
    {

        ::MessageBox(GetModuleHandle(NULL),
            NULL, IDS_CANTSTARTCONSOLE);

        goto exit;
    }

#ifdef USE_FS
    pwnd = new CDirectXFullScreen( m_pGlobalData,
                                   pConsole, 
                                   rEvent );
#else
    pwnd = new CDirectXWindow( m_pGlobalData,
                               pConsole,
                               rEvent );
#endif
    if ( pwnd == NULL )
    {
        goto exit;
    }

    HRESULT hr;

#ifdef USE_FS
    if (m_pGlobalData->bAutoSelectVideoMode)
    {
        hr = pwnd->Initialize( );
    }
    else
    {
        //
        // Initialize with 640 x 480
        //

        hr = pwnd->Initialize( FORCED_VIDEO_CX, FORCED_VIDEO_CY );
    }
#else
    hr = pwnd->Initialize(*this, m_List.getCurrentName());
#endif
    if ( FAILED(hr) )
    {
        TRACE( "CWindow::Initialize failed, err = %X", hr );
        goto exit;
    }

    if (!m_pGlobalData->bNoSound)
    {
        //
        // 060499: Pass pwnd->GetHWND() in instead of hwnd as some systems
        // will not play sound if this isn't set to the active window
        //

        SoundWin32* pSoundWin32 = static_cast<SoundWin32*>( pSound );

        hr = pSoundWin32->Initialize( *pwnd );
        if ( FAILED(hr) )
        {
            TRACE( "Sndwin32 Initialize failed, err = %X", hr );
            goto exit;
        }
    }

    // restore cursor

    ::SetCursor( hcur );
    hcur = NULL;

    ::ShowWindow( *this, SW_HIDE );

    (void)pwnd->Run();

    ::ShowWindow( *this, SW_SHOW );

exit:

    if ( hcur )
    {
        ::SetCursor( hcur );
    }

    delete pwnd;

    delete pConsole;
    delete pSound;
    delete pImage;

    EnableWindow(TRUE);

    // Set focus back to the rom list
    m_List.SetFocus();
}

//  Toggles pausing of the emulator
void CCyberstellaView::togglePause()
{
    m_bIsPause = !m_bIsPause;

    //TODO: theConsole->mediaSource().pause(m_bIsPause);
}

void CCyberstellaView::Initialize()
{
    // Create a properties set for us to use
    m_pPropertiesSet = new PropertiesSet(); 

	// Set up the image list.
    HICON hFolder, hAtari;

    m_imglist.Create ( 16, 16, ILC_COLOR16 | ILC_MASK, 4, 1 );

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
    m_List.init(m_pPropertiesSet,this);

    // Try to load the file stella.pro file
    string filename( "stella.pro" );
    
    // See if we can open the file and load properties from it
    ifstream stream(filename.c_str()); 
    if(stream)
    {
        // File was opened so load properties from it
        stream.close();
        m_pPropertiesSet->load(filename, &Console::defaultProperties());
    }
    else
    {
        m_pPropertiesSet->load("", &Console::defaultProperties());
    }

    // Fill our game list
    m_List.populateRomList();
}

void CCyberstellaView::OnDestroy() 
{
	CFormView::OnDestroy();
    m_List.deleteItemsAndProperties();
}
