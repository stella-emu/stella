//
// StellaX
// Jeff Miller 05/12/2000
//
#include "pch.hxx"
#include "MainDlg.hxx"

#include "PropertySheet.hxx"
#include "ConfigPage.hxx"
#include "AboutPage.hxx"

#include "MD5.hxx"
#include "PropsSet.hxx"

#include "resource.h"

// NOTE: LVS_OWNERDATA doesn't support LVM_SORTITEMS!

inline LPARAM ListView_GetItemData(
    HWND hwndList, 
    int iItem
    )
{
    LVITEM lvi;
    lvi.mask = LVIF_PARAM;
    lvi.iItem = iItem;
    lvi.iSubItem = 0;
    ListView_GetItem(hwndList, &lvi);
    return lvi.lParam;
}

CMainDlg::CMainDlg(HINSTANCE hInstance)
        : m_hInstance(hInstance)
{
    m_pGlobalData = new CGlobalData(hInstance);
}

CMainDlg::~CMainDlg()
{
    delete m_pGlobalData;
}

void CMainDlg::ClearList(
    void
    )
{
    int nCount = ListView_GetItemCount( m_hwndList );

    for (int i = 0; i < nCount; ++i)
    {
        delete (CListData*)ListView_GetItemData( m_hwndList, i );
    }

    ListView_DeleteAllItems( m_hwndList );
}

int CMainDlg::DoModal(
    HWND hwndParent
    )
{
    return DialogBoxParam( m_hInstance, 
                           MAKEINTRESOURCE(IDD),
                           hwndParent, 
                           StaticDialogFunc, 
                           (LPARAM)this );
}

BOOL CALLBACK CMainDlg::StaticDialogFunc(
    HWND hDlg, 
    UINT uMsg,
    WPARAM wParam, 
    LPARAM lParam
    )
{
    CMainDlg* pDlg;

    switch ( uMsg )
    {
    case WM_INITDIALOG:
        pDlg = reinterpret_cast<CMainDlg*>( lParam );
        pDlg->m_hwnd = hDlg;
        (void)::SetWindowLong( hDlg, 
                               DWL_USER,
                               reinterpret_cast<LONG>( pDlg ) );
        break;

    default:
        pDlg = reinterpret_cast<CMainDlg*>( 
            ::GetWindowLong( hDlg, DWL_USER ) );
        if ( pDlg == NULL )
        {
            return FALSE;
        }
        break;
    }

    return pDlg->DialogFunc( uMsg, wParam, lParam );
}

BOOL CALLBACK CMainDlg::DialogFunc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_COMMAND:
            return OnCommand( LOWORD(wParam), (HWND)lParam, HIWORD(wParam) );

        case WM_INITDIALOG:
            return OnInitDialog( );

        case WM_NOTIFY:
            return OnNotify( (int)wParam, (LPNMHDR)lParam );

        case WM_SYSCOMMAND:
            // Allow Alt-F4 to close the window
            if ( wParam == SC_CLOSE )
            {
                ::EndDialog( m_hwnd, IDCANCEL );
            }
            break;
    }

    return FALSE;
}

BOOL CMainDlg::OnInitDialog(void)
{
    DWORD dwRet;

    HWND hwnd = *this;

    dwRet = m_stella.Initialize();
    if ( dwRet != ERROR_SUCCESS )
    {
        MessageBoxFromWinError( dwRet, _T("CStellaX::Initialize") );
        SendMessage( hwnd, WM_CLOSE, 0, 0 );
        return FALSE;
    }

    // Set dialog icon

    HICON hicon = ::LoadIcon(m_hInstance, MAKEINTRESOURCE(IDI_APP));
    ::SendMessage( hwnd, WM_SETICON, ICON_BIG, (LPARAM)hicon );
    ::SendMessage( hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hicon );

    //
    // Make the Rom note have bold text
    //

    HWND hwndCtrl;

    hwndCtrl = ::GetDlgItem( hwnd, IDC_ROMNOTE );

    HFONT hfont = (HFONT)::SendMessage( hwndCtrl, WM_GETFONT, 0, 0 );

    LOGFONT lf;
    ::GetObject( hfont, sizeof(LOGFONT), &lf );
    lf.lfWeight = FW_BOLD;

    m_hfontRomNote = ::CreateFontIndirect( &lf );
    if ( m_hfontRomNote )
    {
        ::SendMessage( hwndCtrl, WM_SETFONT, (WPARAM)m_hfontRomNote, 0 );
    }

    // Do subclassing
    m_header.SubclassDlgItem( hwnd, IDC_ROMLIST );

    const int nMaxString = 256;
    TCHAR psz[nMaxString + 1];

    // Initialize the list view

    m_hwndList = ::GetDlgItem( hwnd, IDC_ROMLIST );
    ASSERT( m_hwndList );

    // LVS_EX_ONECLICKACTIVATE was causing a/vs in kernel32

    ::SendMessage( m_hwndList, 
                   LVM_SETEXTENDEDLISTVIEWSTYLE,
                   0,
                   LVS_EX_FULLROWSELECT );

    RECT rc;
    ::GetClientRect( m_hwndList, &rc );

    LONG lTotalWidth = rc.right-rc.left - GetSystemMetrics(SM_CXVSCROLL);
    int cx = lTotalWidth / CListData::GetColumnCount();

    for (int i = 0; i < CListData::GetColumnCount(); ++i)
    {
        ::LoadString( m_hInstance, 
                      CListData::GetColumnNameIdForColumn( i ), 
                      psz, nMaxString );

        LV_COLUMN lvc;
        lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
        lvc.fmt = LVCFMT_LEFT;
        lvc.cx = cx;
        lvc.pszText = psz;
        ListView_InsertColumn( m_hwndList, i, &lvc );
    }

    DWORD dwError = PopulateRomList();
    if ( dwError != ERROR_SUCCESS )
    {
        MessageBoxFromWinError( dwError, _T("PopulateRomList") );
        return FALSE;
    }

    // if items added, select first item and enable play button

    int nCount = ListView_GetItemCount( m_hwndList );
    if (nCount != 0)
    {
#ifdef _DEBUG
            DWORD dwStartTick = ::GetTickCount();
#endif

            m_header.SetSortCol( 0, TRUE );
            ListView_SortItems( m_hwndList, 
                                ListViewCompareFunc, 
                                (LPARAM)this ); 

#ifdef _DEBUG
            TRACE("\tElapsed ticks for ListView_SortItems = %ld", ::GetTickCount()-dwStartTick);
#endif

        ListView_SetItemState( m_hwndList, 0, LVIS_SELECTED | LVIS_FOCUSED,
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
    LoadString(m_hInstance, IDS_STATUSTEXT, pszStatus, 256);
    wsprintf( psz, pszStatus, nCount );
    SetDlgItemText( hwnd, IDC_ROMCOUNT, psz );

    //
    // Show rom path
    //

    SetDlgItemText( hwnd, IDC_ROMPATH, m_pGlobalData->RomDir() );

    //
    // Set default button
    //

    ::SendMessage( hwnd, DM_SETDEFID, IDC_PLAY, 0 );

    // return FALSE if SetFocus is called
    return TRUE;
}

BOOL CMainDlg::OnCommand(
    int id, 
    HWND hwndCtl, 
    UINT codeNotify
    )
{
    UNUSED_ALWAYS( hwndCtl );
    UNUSED_ALWAYS( codeNotify );

    HWND hwnd = *this;
    CListData* pListData;

    int nItem;

    switch (id)
    {
    case IDC_PLAY:

        nItem = (int)::SendMessage( m_hwndList,
                                    LVM_GETNEXTITEM, 
                                    (WPARAM)-1,
                                    MAKELPARAM( LVNI_SELECTED, 0 ) );
        ASSERT( nItem != -1 );
        if ( nItem == -1 )
        {
            ::MessageBox( m_hInstance, 
                          hwnd, 
                          IDS_NO_ITEM_SELECTED );
            return TRUE;
        }

#if 0
        TCHAR pszFile[MAX_PATH + 1];

        // BUGBUG: On Win95b pszFile is coming back empty!
        LVITEM lvi;
        lvi.mask = LVIF_TEXT;
        lvi.iItem = nItem;
        lvi.iSubItem = CListData::FILENAME_COLUMN;
        lvi.pszText = pszFile;
        lvi.cchTextMax = MAX_PATH;
        ::SendMessage( m_hwndList, LVM_GETITEM, 0, (LPARAM)&lvi );
#endif

        pListData = (CListData*)ListView_GetItemData( m_hwndList, nItem );

        TCHAR pszPathName[ MAX_PATH + 1 ];
        lstrcpy( pszPathName, m_pGlobalData->RomDir() );
        lstrcat( pszPathName, _T("\\") );
        lstrcat( pszPathName, 
                 pListData->GetTextForColumn( CListData::FILENAME_COLUMN ) );

        // Play the game!

        ::EnableWindow( hwnd, FALSE );

        (void)m_stella.PlayROM( hwnd, 
                                pszPathName,
                                pListData->GetTextForColumn( CListData::NAME_COLUMN ),
                                m_pGlobalData);

        ::EnableWindow( hwnd, TRUE );

        // Set focus back to the rom list

        ::SetFocus( m_hwndList );

        return TRUE;

    case IDC_EXIT:
        ClearList();
        ::EndDialog( hwnd, IDCANCEL );
        return TRUE;

    case IDC_CONFIG:
        {
            CPropertySheet ps( _T("Configure"), hwnd );

            CConfigPage pgConfig(m_pGlobalData);
            ps.AddPage( &pgConfig );

            (void)ps.DoModal();
        }
        return TRUE;

        case IDC_ABOUT:
        {
            CPropertySheet ps( _T("Help"), hwnd );

            CHelpPage pgAbout;
            ps.AddPage( &pgAbout );

            (void)ps.DoModal();
        }
    }

    return FALSE;
}

BOOL CMainDlg::OnNotify(
    int idCtrl,
    LPNMHDR pnmh
    )
{
    UNUSED_ALWAYS( idCtrl );

    switch ( pnmh->code )
    {
    case LVN_GETDISPINFO:
        OnGetDispInfo( (NMLVDISPINFO*)pnmh );
        return TRUE;

    case LVN_COLUMNCLICK:
        OnColumnClick( (LPNMLISTVIEW)pnmh );
        return TRUE;

    case LVN_ITEMCHANGED:
        OnItemChanged( (LPNMLISTVIEW)pnmh );
        return TRUE;

    case NM_DBLCLK:
        // send out an ok click to play
        ::SendDlgItemMessage( *this, IDC_PLAY, BM_CLICK, 0, 0 );
        return TRUE;
    }

    // not handled

    return FALSE;
}

static void ScreenToClient(
    HWND hwnd,
    LPRECT lpRect
    )
{
    ::ScreenToClient(hwnd, (LPPOINT)lpRect);
    ::ScreenToClient(hwnd, ((LPPOINT)lpRect)+1);
}

static void FillSolidRect(
    HDC hdc,
    LPCRECT lpRect, 
    COLORREF clr
    )
{
    COLORREF crOld = ::SetBkColor(hdc, clr);
    ::ExtTextOut(hdc, 0, 0, ETO_OPAQUE, lpRect, NULL, 0, NULL);
    ::SetBkColor(hdc, crOld);
}

// ---------------------------------------------------------------------------

DWORD CMainDlg::PopulateRomList(
    void
    )
{

    TRACE("CMainDlg::PopulateRomList");

    DWORD dwRet;

#ifdef _DEBUG
    DWORD dwStartTick = ::GetTickCount();
#endif

    ClearList();

    // REVIEW: Support .zip files?

    TCHAR pszPath[ MAX_PATH ];

    lstrcpy( pszPath, m_pGlobalData->RomDir() );
    lstrcat( pszPath, _T("\\*.bin") );

    WIN32_FIND_DATA ffd;
    HANDLE hFind = FindFirstFile( pszPath, &ffd );

    ListView_SetItemCount( m_hwndList, 100 );

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
        lvi.pszText = LPSTR_TEXTCALLBACK;
        lvi.lParam = (LPARAM)pListData;
        int nItem = ListView_InsertItem( m_hwndList, &lvi );

        ASSERT( nItem != -1 );

        ListView_SetItemText( m_hwndList, nItem, 
            CListData::FILENAME_COLUMN, LPSTR_TEXTCALLBACK );
        ListView_SetItemText(m_hwndList, nItem, 
            CListData::MANUFACTURER_COLUMN, LPSTR_TEXTCALLBACK);
        ListView_SetItemText( m_hwndList, nItem, 
            CListData::RARITY_COLUMN, LPSTR_TEXTCALLBACK );

        // go to the next rom file

        fDone = !FindNextFile(hFind, &ffd);
    }

    if ( hFind != INVALID_HANDLE_VALUE )
    {
        VERIFY( ::FindClose( hFind ) );
    }

#ifdef _DEBUG
    TRACE("\tElapsed ticks for PopulateRomList = %ld", ::GetTickCount()-dwStartTick);
#endif

    return ERROR_SUCCESS;
}

DWORD CMainDlg::ReadRomData(
    CListData* pListData
    ) const
{
    // TODO: Move this method to ListData class (?)

    if ( pListData == NULL )
    {
        ASSERT( FALSE );
        return ERROR_BAD_ARGUMENTS;
    }

    // attempt to read the rom file

    TCHAR pszPath[MAX_PATH + 1];

    lstrcpy( pszPath, m_pGlobalData->RomDir() );
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

        PropertiesSet& propertiesSet = m_stella.GetPropertiesSet();
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

void CMainDlg::OnColumnClick(
    LPNMLISTVIEW pnmv
    )
{
    HCURSOR hcur = ::SetCursor(::LoadCursor(NULL, IDC_WAIT));

    m_header.SetSortCol(pnmv->iSubItem, TRUE);
    ListView_SortItems(pnmv->hdr.hwndFrom, ListViewCompareFunc, (LPARAM)this);

    // ensure the selected item is visible

    int nItem = ListView_GetNextItem( m_hwndList, -1, LVNI_SELECTED );
    if (nItem != -1)
    {
        ListView_EnsureVisible( m_hwndList, nItem, TRUE );
    }

    ::SetCursor(hcur);
}

void CMainDlg::OnItemChanged(
    LPNMLISTVIEW pnmv
    )
{
    HWND hwnd = *this;

    HWND hwndNote = ::GetDlgItem( hwnd, IDC_ROMNOTE );

    RECT rc;
    ::GetWindowRect(hwndNote, &rc);
    ::ScreenToClient( hwnd, (LPPOINT)&rc );
    ::ScreenToClient( hwnd, ((LPPOINT)&rc)+1 );

    int iItem = ListView_GetNextItem(pnmv->hdr.hwndFrom, -1, LVNI_SELECTED);
    if (iItem == -1)
    {
        ::SetWindowText( hwndNote, _T("") );
        ::EnableWindow( ::GetDlgItem( hwnd, IDC_PLAY ), FALSE );
        ::InvalidateRect( hwnd, &rc, TRUE );
        return;
    }

    CListData* pListData = (CListData*)ListView_GetItemData( 
        pnmv->hdr.hwndFrom, 
        pnmv->iItem );

    ::SetWindowText( hwndNote, pListData->GetNote() );
    ::InvalidateRect( hwnd, &rc, TRUE );
    ::EnableWindow( ::GetDlgItem( hwnd, IDC_PLAY ), TRUE );
}

// ---------------------------------------------------------------------------
// LPSTR_TEXTCALLBACK message handlers

void CMainDlg::OnGetDispInfo(
    NMLVDISPINFO* pnmv
    )
{
    // Provide the item or subitem's text, if requested. 

    if ( ! (pnmv->item.mask & LVIF_TEXT ) )
    {
        return;
    }

    CListData* pListData = (CListData*)ListView_GetItemData(
        pnmv->hdr.hwndFrom, 
        pnmv->item.iItem );
    ASSERT( pListData );
    if ( pListData == NULL )
    {
        return;
    }

    if ( ! pListData->IsPopulated() )
    {
        ReadRomData( pListData );
    }

    pnmv->item.pszText = const_cast<LPTSTR>( pListData->GetTextForColumn(pnmv->item.iSubItem) );
    // ASSERT( pnmv->item.pszText );
} 

int CALLBACK CMainDlg::ListViewCompareFunc(
    LPARAM lParam1, 
    LPARAM lParam2, 
    LPARAM lParamSort
    )
{
    CMainDlg* pThis = reinterpret_cast<CMainDlg*>( lParamSort );

    //
    // I assume that the metadata for column 0 is always available,
    // while other column metadata requires a call to ReadRomData
    //

    int nSortCol = pThis->m_header.GetSortCol();

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