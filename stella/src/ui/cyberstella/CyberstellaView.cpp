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
	ON_COMMAND(IDG_GUNFIGHT, OnGunfight)
	ON_COMMAND(IDG_JAMMED, OnJammed)
	ON_COMMAND(IDG_QB, OnQb)
	ON_COMMAND(IDG_THRUST, OnThrust)
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
    StellaConfig dlg(m_pGlobalData);
    dlg.DoModal();
}

void CCyberstellaView::OnPlay() 
{
    playRom();
}

//  Toggles pausing of the emulator
void CCyberstellaView::togglePause()
{
    m_bIsPause = !m_bIsPause;

    //TODO: theConsole->mediaSource().pause(m_bIsPause);
}

LRESULT CCyberstellaView::initialize(WPARAM wParam, LPARAM lParam)
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
    string filename("stella.pro");
    
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
        MessageBox("stella.pro not found in working directory!", "Warning!", MB_OK|MB_ICONEXCLAMATION);
    }

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

void CCyberstellaView::OnGunfight()
{
    MessageBox("To avoid probable GPL violations by including non-GPL games into this project, this function is currently disabled. We're working on a GPL conform solution though, so check back soon.", "Sorry, currently not available!", MB_OK);
    //playRom(IDG_GUNFIGHT);
    //MessageBox("If you'd like to play Gunfight on a real VCS, you can order a cartridge for only $16\nfrom http://webpages.charter.net/hozervideo!", "Commercial Break", MB_OK);
}    

void CCyberstellaView::OnJammed() 
{
    MessageBox("To avoid probable GPL violations by including non-GPL games into this project, this function is currently disabled. We're working on a GPL conform solution though, so check back soon.", "Sorry, currently not available!", MB_OK);
    //playRom(IDG_JAMMED);
    //MessageBox("If you'd like to play Jammed on a real VCS, you can order a cartridge for only $16\nfrom http://webpages.charter.net/hozervideo!", "Commercial Break", MB_OK);
}

void CCyberstellaView::OnQb() 
{
    MessageBox("To avoid probable GPL violations by including non-GPL games into this project, this function is currently disabled. We're working on a GPL conform solution though, so check back soon.", "Sorry, currently not available!", MB_OK);
    //playRom(IDG_QB);
    //MessageBox("If you'd like to play Qb on a real VCS, you can order a cartridge for only $16\nfrom http://webpages.charter.net/hozervideo!", "Commercial Break", MB_OK);
}

void CCyberstellaView::OnThrust() 
{
    MessageBox("To avoid probable GPL violations by including non-GPL games into this project, this function is currently disabled. We're working on a GPL conform solution though, so check back soon.", "Sorry, currently not available!", MB_OK);
    //playRom(IDG_THRUST);
    //MessageBox("If you'd like to play Thrust on a real VCS, you can order a cartridge for only $25\nfrom http://webpages.charter.net/hozervideo!", "Commercial Break", MB_OK);
}

void CCyberstellaView::playRom(LONG gameID)
{
    
    EnableWindow(FALSE);

#ifdef USE_FS
    CDirectXFullScreen* pwnd = NULL;
#else
    CDirectXWindow* pwnd = NULL;
#endif

    CString fileName;
    BYTE* pImage = NULL;
    LPCTSTR pszFileName = NULL;
    Console* pConsole = NULL;
    Sound* pSound = NULL;
    DWORD dwImageSize;
    DWORD dwActualSize;
    Event rEvent;

    // Create Sound driver object
    // (Will be initialized once we have a window handle below)
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

    // Special handling for special games
    switch(gameID)
    {
        case IDG_GUNFIGHT:
        {
            /*pszFileName = "Gunfight";
            dwActualSize = sizeof gunfight;
            pImage = new BYTE[dwActualSize];
            for(int i=0; i<dwActualSize; i++)
            {
				pImage[i] = gunfight[i]^(pszFileName[i%strlen(pszFileName)]);
            }*/
            break;
        }
        case IDG_JAMMED:
        {
            /*pszFileName = "Jammed";
            dwActualSize = sizeof jammed;
            pImage = new BYTE[dwActualSize];
            for(int i=0; i<dwActualSize; i++)
            {
				pImage[i] = jammed[i]^(pszFileName[i%strlen(pszFileName)]);
            }*/
            break;
        }
        case IDG_QB:
        {
            /*pszFileName = "Qb";
            dwActualSize = sizeof qb;
            pImage = new BYTE[dwActualSize];
            for(int i=0; i<dwActualSize; i++)
            {
				pImage[i] = qb[i]^(pszFileName[i%strlen(pszFileName)]);
            }*/
            break;
        }
        case IDG_THRUST:
        {
            /*pszFileName = "Thrust";
            dwActualSize = sizeof thrust;
            pImage = new BYTE[dwActualSize];
            for(int i=0; i<dwActualSize; i++)
            {
				pImage[i] = thrust[i]^(pszFileName[i%strlen(pszFileName)]);
            }*/
            break;
        }
        default:
        {
            fileName = m_List.getCurrentFile();

            // Safety Bail Out
            if(fileName.GetLength() <= 0)   return;

            // Load the rom file
            HANDLE hFile;
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

            if ( ! ::ReadFile( hFile, pImage, dwImageSize, &dwActualSize, NULL ) )
            {
                VERIFY( ::CloseHandle( hFile ) );

                MessageBoxFromGetLastError(fileName);

                goto exit;
            }

            VERIFY( ::CloseHandle(hFile) );
        }
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

    ::ShowWindow( *this, SW_HIDE );

    (void)pwnd->Run();

    ::ShowWindow( *this, SW_SHOW );

exit:
    delete pwnd;
    delete pConsole;
    delete pSound;
    if (pImage) delete pImage;

    EnableWindow(TRUE);

    // Set focus back to the rom list
    m_List.SetFocus();
}
