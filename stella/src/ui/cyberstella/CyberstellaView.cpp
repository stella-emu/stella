// CyberstellaView.cpp : implementation of the CCyberstellaView class
//

#include "pch.hxx"
#include "Cyberstella.h"
#include "MainFrm.h"
#include "CyberstellaDoc.h"
#include "CyberstellaView.h"
#include "StellaConfig.h"
#include "MD5.hxx"
#include "PropsSet.hxx"
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

//#define USE_FS

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

    DWORD dwRet;

    HWND hwnd = *this;

    dwRet = Initialize();
    if ( dwRet != ERROR_SUCCESS )
    {
        MessageBoxFromWinError( dwRet, _T("CStellaX::Initialize") );
        AfxGetMainWnd()->SendMessage(WM_CLOSE, 0, 0);
    }

    const int nMaxString = 256;
    TCHAR psz[nMaxString + 1];

    // LVS_EX_ONECLICKACTIVATE was causing a/vs in kernel32

    ::SendMessage( m_List, 
                   LVM_SETEXTENDEDLISTVIEWSTYLE,
                   0,
                   LVS_EX_FULLROWSELECT );

    RECT rc;
    ::GetClientRect( m_List, &rc );

    LONG lTotalWidth = rc.right-rc.left - GetSystemMetrics(SM_CXVSCROLL);
    int cx = lTotalWidth / CListData::GetColumnCount();

    for (int i = 0; i < CListData::GetColumnCount(); ++i)
    {
        
        LoadString(GetModuleHandle(NULL), 
        CListData::GetColumnNameIdForColumn( i ), 
        psz, nMaxString );

        LV_COLUMN lvc;
        lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
        lvc.fmt = LVCFMT_LEFT;
        lvc.cx = cx;
        lvc.pszText = psz;
        ListView_InsertColumn( m_List, i, &lvc );
    }

    DWORD dwError = PopulateRomList();
    if ( dwError != ERROR_SUCCESS )
    {
        MessageBoxFromWinError( dwError, _T("PopulateRomList") );
        AfxGetMainWnd()->SendMessage(WM_CLOSE, 0, 0);
    }

    // if items added, select first item and enable play button

    int nCount = ListView_GetItemCount( m_List );
    if (nCount != 0)
    {
        m_List.SortItems(ListViewCompareFunc, 0);
        ListView_SetItemState( m_List, 0, LVIS_SELECTED | LVIS_FOCUSED,
        LVIS_SELECTED | LVIS_FOCUSED );
    }
    else
    {
        ::EnableWindow(::GetDlgItem( hwnd, IDC_PLAY), FALSE );
    }

    //
    // Show status text
    //

    TCHAR pszStatus[256 + 1];
    LoadString(GetModuleHandle(NULL), IDS_STATUSTEXT, pszStatus, 256);
    wsprintf( psz, pszStatus, nCount );
    SetDlgItemText(IDC_ROMCOUNT, psz );

    //
    // Show rom path
    //

    SetDlgItemText(IDC_ROMPATH, m_pGlobalData->romDir);

    //
    // Set default button
    //

    ::SendMessage( hwnd, DM_SETDEFID, IDC_PLAY, 0 );
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

////////////////////////////
// Listview compare function
int CALLBACK CCyberstellaView::ListViewCompareFunc(LPARAM lParam1, LPARAM lParam2, 
                                           LPARAM lParamSort)
{
    CCyberstellaView* pThis = reinterpret_cast<CCyberstellaView*>( lParamSort );

    //
    // I assume that the metadata for column 0 is always available,
    // while other column metadata requires a call to ReadRomData
    //

    int nSortCol = lParamSort;

    CListData* pItem1 = reinterpret_cast<CListData*>( lParam1 );
    if ( ! pItem1->IsPopulated() && nSortCol != 0 )
    {
        pThis->ReadRomData( pItem1 );
    }

    CListData* pItem2 = reinterpret_cast<CListData*>( lParam2 );  
    if ( ! pItem2->IsPopulated() && nSortCol != 0 )
    {
        pThis->ReadRomData( pItem2 );
    }

    LPCTSTR pszItem1 = pItem1->GetTextForColumn( nSortCol );
    LPCTSTR pszItem2 = pItem2->GetTextForColumn( nSortCol );

    //
    // put blank items last
    //

    if ( pszItem1 == NULL || pszItem1[0] == _T('\0') )
    {
        return 1;
    }

    if ( pszItem2 == NULL || pszItem2[0] == _T('\0') )
    {
        return -1;
    }
    
    //
    // Compare the specified column. 
    //

    return lstrcmpi( pszItem1, pszItem2 );
}

void CCyberstellaView::OnConfig() 
{
    StellaConfig dlg(m_pGlobalData);
    dlg.DoModal();
}

DWORD CCyberstellaView::PopulateRomList()
{
    DWORD dwRet;

    ClearList();

    TCHAR pszPath[ MAX_PATH ];
    lstrcpy( pszPath, m_pGlobalData->romDir);
    lstrcat( pszPath, _T("\\*.bin") );

    WIN32_FIND_DATA ffd;
    HANDLE hFind = FindFirstFile( pszPath, &ffd );

    ListView_SetItemCount(m_List, 100);

    int iItem = 0;

    BOOL fDone = (hFind == INVALID_HANDLE_VALUE);
    while (!fDone)
    {
        //
        // File metadata will be read in ReadRomData()
        //

        CListData* pListData  = new CListData;
        if( pListData == NULL )
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        dwRet = pListData->Initialize();
        if ( dwRet != ERROR_SUCCESS )
        {
            return dwRet;
        }

        if ( ! pListData->m_strFileName.Set( ffd.cFileName ) )
        {
            return FALSE;
        }

        LV_ITEM lvi;
        lvi.mask = LVIF_TEXT | LVIF_PARAM;
        lvi.iItem = iItem++;
        lvi.iSubItem = 0;
        lvi.pszText = ffd.cFileName;
        lvi.lParam = (LPARAM)pListData;
		m_List.InsertItem (&lvi);

        //TODO: Display the Rest
        /*int nItem = ListView_InsertItem(m_List, &lvi);
        ASSERT( nItem != -1 );
        ListView_SetItemText(m_List, nItem, 
            CListData::FILENAME_COLUMN, LPSTR_TEXTCALLBACK );
        ListView_SetItemText(m_List, nItem, 
            CListData::MANUFACTURER_COLUMN, LPSTR_TEXTCALLBACK);
        ListView_SetItemText(m_List, nItem, 
            CListData::RARITY_COLUMN, LPSTR_TEXTCALLBACK );*/

        // go to the next rom file
        fDone = !FindNextFile(hFind, &ffd);
    }

    if ( hFind != INVALID_HANDLE_VALUE )
    {
        VERIFY( ::FindClose( hFind ) );
    }
    return ERROR_SUCCESS;
}

void CCyberstellaView::ClearList()
{
    int nCount = ListView_GetItemCount(m_List);

    for (int i = 0; i < nCount; ++i)
    {
        ListView_DeleteItem(m_List,0);
    }

    ListView_DeleteAllItems(m_List);
}

DWORD CCyberstellaView::ReadRomData(CListData* pListData) const
{
    // TODO: Move this method to ListData class (?)
    if ( pListData == NULL )
    {
        ASSERT( FALSE );
        return ERROR_BAD_ARGUMENTS;
    }

    // attempt to read the rom file
    TCHAR pszPath[MAX_PATH + 1];
    lstrcpy( pszPath, m_pGlobalData->romDir);
    lstrcat( pszPath, _T("\\") );
    lstrcat( pszPath, pListData->GetTextForColumn( CListData::FILENAME_COLUMN ) );

    HANDLE hFile;
    
    hFile = CreateFile( pszPath, 
                        GENERIC_READ, 
                        FILE_SHARE_READ, 
                        NULL, 
                        OPEN_EXISTING, 
                        FILE_ATTRIBUTE_NORMAL,
                        NULL );
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return GetLastError();
    }

    DWORD dwFileSize = ::GetFileSize( hFile, NULL );

    BYTE* pImage = new BYTE[dwFileSize];
    if ( pImage == NULL )
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    
    DWORD dwRead;

    if ( ::ReadFile( hFile, pImage, dwFileSize, &dwRead, NULL ) )
    {
        // Read the file, now check the md5
        
        std::string md5 = MD5( pImage, dwFileSize );
        
        // search through the properties set for this MD5

        PropertiesSet& propertiesSet = GetPropertiesSet();
        Properties properties;
        propertiesSet.getMD5(md5, properties);

        if ( ! pListData->m_strManufacturer.Set( 
            properties.get("Cartridge.Manufacturer").c_str() ) )
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        if ( ! pListData->m_strName.Set( 
            properties.get("Cartridge.Name").c_str() ) )
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        if (! pListData->m_strRarity.Set( 
            properties.get("Cartridge.Rarity").c_str() ) )
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        if ( ! pListData->m_strNote.Set( 
            properties.get("Cartridge.Note").c_str() ) )
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }
    
    delete[] pImage;
    
    VERIFY( ::CloseHandle( hFile ) );

    pListData->m_fPopulated = TRUE;

    return ERROR_SUCCESS;
}

void CCyberstellaView::OnPlay() 
{
    CListData* pListData;
    int nItem;

    nItem = (int)::SendMessage( m_List,
                                    LVM_GETNEXTITEM, 
                                    (WPARAM)-1,
                                    MAKELPARAM( LVNI_SELECTED, 0 ) );
    ASSERT( nItem != -1 );
    if ( nItem == -1 )
    {
        ::MessageBox( GetModuleHandle(NULL), 
                      *this, 
                      IDS_NO_ITEM_SELECTED );
        return;
    }

    pListData = (CListData*)m_List.GetItemData(nItem);

    TCHAR pszPathName[ MAX_PATH + 1 ];
    lstrcpy( pszPathName, m_pGlobalData->romDir);
    lstrcat( pszPathName, _T("\\") );
    lstrcat( pszPathName, 
             pListData->GetTextForColumn( CListData::FILENAME_COLUMN ) );

    // Play the game!

    ::EnableWindow(*this, FALSE );

    PlayROM( *this, 
                            pszPathName,
                            pListData->GetTextForColumn( CListData::NAME_COLUMN ),
                            m_pGlobalData);

    ::EnableWindow( *this, TRUE );

    // Set focus back to the rom list

    ::SetFocus(m_List);
}

//  Toggles pausing of the emulator
void CCyberstellaView::togglePause()
{
    m_bIsPause = !m_bIsPause;

    //TODO: theConsole->mediaSource().pause(m_bIsPause);
}

DWORD CCyberstellaView::Initialize()
{
    TRACE( "CStellaXMain::SetupProperties" );

    // Create a properties set for us to use

    if ( m_pPropertiesSet )
    {
        return ERROR_SUCCESS;
    }

    m_pPropertiesSet = new PropertiesSet(); 
    if ( m_pPropertiesSet == NULL )
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

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

    return ERROR_SUCCESS;
}

HRESULT CCyberstellaView::PlayROM(HWND hwnd,
    LPCTSTR pszPathName,
    LPCTSTR pszFriendlyName,
    CGlobalData* rGlobalData)
{
    UNUSED_ALWAYS( hwnd );

    HRESULT hr = S_OK;

    TRACE("CStellaXMain::PlayROM");

    //
    // show wait cursor while loading
    //

    HCURSOR hcur = ::SetCursor( ::LoadCursor( NULL, IDC_WAIT ) );

    //
    // setup objects used here
    //

    BYTE* pImage = NULL;
    LPCTSTR pszFileName = NULL;

#ifdef USE_FS
    CDirectXFullScreen* pwnd = NULL;
#else
    CDirectXWindow* pwnd = NULL;
#endif
    Console* pConsole = NULL;
    Sound* pSound = NULL;
    Event rEvent;

    //
    // Load the rom file
    //

    HANDLE hFile;
    DWORD dwImageSize;

    hFile = ::CreateFile( pszPathName, 
                          GENERIC_READ, 
                          FILE_SHARE_READ, 
                          NULL, 
                          OPEN_EXISTING, 
                          FILE_ATTRIBUTE_NORMAL,
                          NULL );
    if (hFile == INVALID_HANDLE_VALUE)
    {
        HINSTANCE hInstance = (HINSTANCE)::GetWindowLong( hwnd, GWL_HINSTANCE );

        DWORD dwLastError = ::GetLastError();

        TCHAR pszCurrentDirectory[ MAX_PATH + 1 ];
        ::GetCurrentDirectory( MAX_PATH, pszCurrentDirectory );

        // ::MessageBoxFromGetLastError( pszPathName );
        TCHAR pszFormat[ 1024 ];
        LoadString( hInstance,
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
                  pszPathName, 
                  dwLastError,
                  pszLastError );

        ::MessageBox( hwnd, 
                      pszError, 
                      _T("Error"),
                      MB_OK | MB_ICONEXCLAMATION );

        ::LocalFree( pszLastError );

        hr = HRESULT_FROM_WIN32( ::GetLastError() ); 
        goto exit;
    }

    dwImageSize = ::GetFileSize( hFile, NULL );

    pImage = new BYTE[dwImageSize + 1];
    if ( pImage == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    DWORD dwActualSize;
    if ( ! ::ReadFile( hFile, pImage, dwImageSize, &dwActualSize, NULL ) )
    {
        VERIFY( ::CloseHandle( hFile ) );

        MessageBoxFromGetLastError( pszPathName );

        hr = HRESULT_FROM_WIN32( ::GetLastError() );
        goto exit;
    }

    VERIFY( ::CloseHandle(hFile) );

    //
    // Create Sound driver object
    // (Will be initialized once we have a window handle below)
    //

    if (rGlobalData->bNoSound)
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
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    //
    // get just the filename
    //

    pszFileName = _tcsrchr( pszPathName, _T('\\') );
    if ( pszFileName )
    {
        ++pszFileName;
    }
    else
    {
        pszFileName = pszPathName;
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
            hr = E_OUTOFMEMORY;
            goto exit;
        }
    }
    catch (...)
    {

        ::MessageBox(rGlobalData->instance,
            NULL, IDS_CANTSTARTCONSOLE);

        goto exit;
    }

#ifdef USE_FS
    pwnd = new CDirectXFullScreen( rGlobalData,
                                   pConsole, 
                                   rEvent );
#else
    pwnd = new CDirectXWindow( rGlobalData,
                               pConsole,
                               rEvent );
#endif
    if ( pwnd == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

#ifdef USE_FS
    if (rGlobalData->bAutoSelectVideoMode)
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
    hr = pwnd->Initialize( hwnd, pszFriendlyName );
#endif
    if ( FAILED(hr) )
    {
        TRACE( "CWindow::Initialize failed, err = %X", hr );
        goto exit;
    }

    if (!rGlobalData->bNoSound)
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

    ::ShowWindow( hwnd, SW_HIDE );

    (void)pwnd->Run();

    ::ShowWindow( hwnd, SW_SHOW );

exit:

    if ( hcur )
    {
        ::SetCursor( hcur );
    }

    delete pwnd;

    delete pConsole;
    delete pSound;
    delete pImage;

    return hr;
}