//============================================================================
//
//   SSSS    tt          lll  lll       
//  SS  SS   tt           ll   ll        
//  SS     tttttt  eeee   ll   ll   aaaa 
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2000 by Jeff Miller
// Copyright (c) 2004 by Stephen Anthony
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: MainDlg.cxx,v 1.3 2004-07-06 22:51:58 stephena Exp $
//============================================================================ 

#include "pch.hxx"
#include "MainDlg.hxx"
#include "GlobalData.hxx"
#include "PropertySheet.hxx"
#include "AboutPage.hxx"
#include "ConfigPage.hxx"
#include "resource.h"
#include "Settings.hxx"

#define BKGND_BITMAP_TOP     64
#define BKGND_BITMAP_BOTTOM  355

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
inline LPARAM ListView_GetItemData( HWND hwndList, int iItem )
{
  LVITEM lvi;
  lvi.mask = LVIF_PARAM;
  lvi.iItem = iItem;
  lvi.iSubItem = 0;

  ListView_GetItem(hwndList, &lvi);

  return lvi.lParam;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
CMainDlg::CMainDlg( CGlobalData& rGlobalData, HINSTANCE hInstance )
        : myGlobalData(rGlobalData),
          m_hInstance(hInstance)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
void CMainDlg::ClearList( void )
{
  int nCount = ListView_GetItemCount( myHwndList );

  for (int i = 0; i < nCount; ++i)
    delete (CListData*)ListView_GetItemData( myHwndList, i );

  ListView_DeleteAllItems( myHwndList );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
int CMainDlg::DoModal( HWND hwndParent )
{
  return DialogBoxParam( m_hInstance, 
                         MAKEINTRESOURCE(IDD),
                         hwndParent, 
                         StaticDialogFunc, 
                         (LPARAM)this );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
BOOL CALLBACK
CMainDlg::StaticDialogFunc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
  CMainDlg* pDlg;

  switch ( uMsg )
  {
    case WM_INITDIALOG:
      pDlg = reinterpret_cast<CMainDlg*>( lParam );
      pDlg->myHwnd = hDlg;
      (void)::SetWindowLong( hDlg, DWL_USER, reinterpret_cast<LONG>( pDlg ) );
      break;

    default:
      pDlg = reinterpret_cast<CMainDlg*>( ::GetWindowLong( hDlg, DWL_USER ) );
      if ( pDlg == NULL )
        return FALSE;
      break;
  }

  return pDlg->DialogFunc( uMsg, wParam, lParam );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
BOOL CALLBACK
CMainDlg::DialogFunc( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
  BOOL b;

  switch (uMsg)
  {
    case WM_COMMAND:
      return OnCommand( LOWORD(wParam), (HWND)lParam, HIWORD(wParam) );

    case WM_CTLCOLORSTATIC:
      b = (BOOL)OnCtlColorStatic( (HDC)wParam, (HWND)lParam );
      if (b)
        return b;
      break;

    case WM_ERASEBKGND:
      if ( OnEraseBkgnd( (HDC)wParam ) )
        return TRUE;
      break;

    case WM_INITDIALOG:
      return OnInitDialog( );

    case WM_NOTIFY:
      return OnNotify( (int)wParam, (LPNMHDR)lParam );

    case WM_PALETTECHANGED:
      TRACE( "WM_PALETTECHANGED from maindlg" );
      return FALSE;

    case WM_QUERYNEWPALETTE:
      TRACE( "WM_QUERYNEWPALETTE from maindlg" );
      return FALSE;

    // Cool caption handlers
    case WM_DESTROY:
      OnDestroy( );
      break;

    case WM_DRAWITEM:
      // Forward this onto the control
      ::SendMessage( ((LPDRAWITEMSTRUCT)lParam)->hwndItem, WM_DRAWITEM, wParam, lParam );
      return TRUE;

    case WM_NCPAINT:
      // DefWindowProc(hDlg, uMsg, wParam, lParam);
      OnNcPaint( (HRGN)wParam );
      return TRUE;

    case WM_NCACTIVATE:
      OnNcActivate( (BOOL)wParam );
      // When the fActive parameter is FALSE, an application should return 
      // TRUE to indicate that the system should proceed with the default 
      // processing
      SetWindowLong( myHwnd, DWL_MSGRESULT, TRUE );
      return TRUE;

    case WM_NCLBUTTONDOWN:
      return OnNcLButtonDown( (INT)wParam, MAKEPOINTS(lParam) );

    case WM_SYSCOMMAND:
      // Allow Alt-F4 to close the window
      if ( wParam == SC_CLOSE )
        ::EndDialog( myHwnd, IDCANCEL );
      break;
  }

  // Message not handled
  return FALSE;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
BOOL CMainDlg::OnInitDialog( void  )
{
  DWORD dwRet;

  HWND hwnd = *this;

  // Set dialog icon
  HICON hicon = ::LoadIcon(m_hInstance, MAKEINTRESOURCE(IDI_APP));
  ::SendMessage( hwnd, WM_SETICON, ICON_BIG, (LPARAM)hicon );
  ::SendMessage( hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hicon );

  // Make the Rom note have bold text
  HWND hwndCtrl;

  hwndCtrl = ::GetDlgItem( hwnd, IDC_ROMNOTE );

  HFONT hfont = (HFONT)::SendMessage( hwndCtrl, WM_GETFONT, 0, 0 );

  LOGFONT lf;
  ::GetObject( hfont, sizeof(LOGFONT), &lf );
  lf.lfWeight = FW_BOLD;

  m_hfontRomNote = ::CreateFontIndirect( &lf );
  if ( m_hfontRomNote )
    ::SendMessage( hwndCtrl, WM_SETFONT, (WPARAM)m_hfontRomNote, 0 );

  // Do subclassing
  m_CoolCaption.OnInitDialog( hwnd );
  myHeader.SubclassDlgItem( hwnd, IDC_ROMLIST );
  myAppTitle.SubclassDlgItem( hwnd, IDC_TITLE );
  myPlayButton.SubclassDlgItem( hwnd, IDC_PLAY );
  myReloadButton.SubclassDlgItem( hwnd, IDC_RELOAD );
  myHelpButton.SubclassDlgItem( hwnd, IDC_ABOUT );
  myConfigButton.SubclassDlgItem( hwnd, IDC_CONFIG );
  myExitButton.SubclassDlgItem( hwnd, IDC_EXIT );

  const int nMaxString = 256;
  TCHAR psz[nMaxString + 1];

  // Initialize the list view
  myHwndList = ::GetDlgItem( hwnd, IDC_ROMLIST );
  ASSERT( myHwndList );

  // LVS_EX_ONECLICKACTIVATE was causing a/vs in kernel32
  ::SendMessage( myHwndList, LVM_SETEXTENDEDLISTVIEWSTYLE,
                 0, LVS_EX_FULLROWSELECT );

  RECT rc;
  ::GetClientRect( myHwndList, &rc );

  LONG lTotalWidth = rc.right-rc.left - GetSystemMetrics(SM_CXVSCROLL);
  int cx = lTotalWidth / CListData::GetColumnCount();

  for (int i = 0; i < CListData::GetColumnCount(); ++i)
  {
    ::LoadString( m_hInstance, CListData::GetColumnNameIdForColumn( i ), 
                  psz, nMaxString );

    LV_COLUMN lvc;
    lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
    lvc.fmt = LVCFMT_LEFT;
    lvc.cx = cx;
    lvc.pszText = psz;
    ListView_InsertColumn( myHwndList, i, &lvc );
  }

  // Update the ROM game list
  UpdateRomList();

  // Set default button
  ::SendMessage( hwnd, DM_SETDEFID, IDC_PLAY, 0 );

  // return FALSE if SetFocus is called
  return TRUE;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
BOOL CMainDlg::OnCommand( int id, HWND hwndCtl, UINT codeNotify )
{
  UNUSED_ALWAYS( hwndCtl );
  UNUSED_ALWAYS( codeNotify );

  HWND hwnd = *this;
  CListData* pListData;

  int nItem;

  switch (id)
  {
    case IDC_PLAY:
      nItem = (int)::SendMessage( myHwndList, LVM_GETNEXTITEM, 
                                  (WPARAM)-1, MAKELPARAM( LVNI_SELECTED, 0 ) );
      ASSERT( nItem != -1 );
      if ( nItem == -1 )
      {
        ::MessageBox( m_hInstance, hwnd, IDS_NO_ITEM_SELECTED );
        return TRUE;
      }

      pListData = (CListData*)ListView_GetItemData( myHwndList, nItem );

      TCHAR pszPathName[ MAX_PATH + 1 ];
      lstrcpy( pszPathName, myGlobalData.settings().getString("romdir").c_str() );
      lstrcat( pszPathName, _T("\\") );
      lstrcat( pszPathName, pListData->GetTextForColumn( CListData::FILENAME_COLUMN ) );

      (void)m_stella.PlayROM( pszPathName, myGlobalData );

      // Set focus back to the rom list
      ::SetFocus( myHwndList );

      return TRUE;
      break; // case IDC_PLAY

    case IDC_EXIT:
      ClearList();
      ::EndDialog( hwnd, IDCANCEL );
      return TRUE;
      break; // case IDC_EXIT

    case IDC_CONFIG:
    {
      CPropertySheet ps( _T("Configure"), hwnd );

      CConfigPage pgConfig( myGlobalData );
      ps.AddPage( &pgConfig );

      (void)ps.DoModal();

      return TRUE;
      break; // case IDC_CONFIG
    }

    case IDC_ABOUT:
    {
      CPropertySheet ps(_T("Help"), hwnd);

      CHelpPage pgAbout;
      ps.AddPage(&pgAbout);

      ps.DoModal();
      return TRUE;
      break; // case IDC_ABOUT
    }

    case IDC_RELOAD:
    {
      UpdateRomList();

      return TRUE;
      break; // case IDC_RELOAD
    }
  }

  return FALSE;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
BOOL CMainDlg::OnNotify( int idCtrl, LPNMHDR pnmh )
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
static void ScreenToClient( HWND hwnd, LPRECT lpRect )
{
  ::ScreenToClient(hwnd, (LPPOINT)lpRect);
  ::ScreenToClient(hwnd, ((LPPOINT)lpRect)+1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
static void FillSolidRect( HDC hdc, LPCRECT lpRect, COLORREF clr )
{
  COLORREF crOld = ::SetBkColor(hdc, clr);
  ::ExtTextOut(hdc, 0, 0, ETO_OPAQUE, lpRect, NULL, 0, NULL);
  ::SetBkColor(hdc, crOld);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
BOOL CMainDlg::OnEraseBkgnd( HDC hdc )
{
  // don't do this in 256 color

  if (GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE)
    return FALSE;

  RECT rcWindow;
  ::GetWindowRect( *this, &rcWindow );
  ::ScreenToClient( *this, &rcWindow );

  FillSolidRect( hdc, &rcWindow, ::GetSysColor( COLOR_3DFACE ) );

  RECT rc;
  ::SetRect( &rc, rcWindow.left, BKGND_BITMAP_TOP, rcWindow.right, BKGND_BITMAP_BOTTOM );

  long lWidth = rc.right - rc.left;
  long lHeight = rc.bottom - rc.top;

  HDC hdcMem = CreateCompatibleDC(hdc);

  HBITMAP hbmpTile = LoadBitmap( m_hInstance, MAKEINTRESOURCE(IDB_TILE) );

  BITMAP bm;
  GetObject(hbmpTile, sizeof(bm), &bm);

  HBITMAP hbmpOld = (HBITMAP)SelectObject(hdcMem, hbmpTile);

  for (long x = 0; x < lWidth; x += bm.bmWidth)
  {
    for (long y = 0; y < lHeight; y += bm.bmHeight)
    {
      ::BitBlt( hdc, 
                rc.left + x, rc.top + y,
                ( (rc.left + x + bm.bmWidth) > rc.right ) ? rc.right-(rc.left+x) : bm.bmWidth,
                ( (rc.top + y + bm.bmHeight) > rc.bottom ) ? rc.bottom-(rc.top+y) : bm.bmHeight,
                hdcMem, 
                0, 0, SRCCOPY );
    }
  }

  SelectObject(hdcMem, hbmpOld);
  DeleteObject(hbmpTile);
  DeleteDC(hdcMem);

  return TRUE;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
HBRUSH CMainDlg::OnCtlColorStatic( HDC hdcStatic, HWND hwndStatic )
{
  // don't do this in 256 color

  if (GetDeviceCaps(hdcStatic, RASTERCAPS) & RC_PALETTE)
    return FALSE;

  if ((GetWindowLong(hwndStatic, GWL_STYLE) & SS_ICON) == SS_ICON)
    return NULL;

  SetBkMode(hdcStatic, TRANSPARENT);

  return (HBRUSH)GetStockObject(NULL_BRUSH);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
void CMainDlg::UpdateRomList( void )
{
  HWND hwndText;
  RECT rc;

  DWORD dwError = PopulateRomList();
  if ( dwError != ERROR_SUCCESS )
    MessageBoxFromWinError( dwError, _T("PopulateRomList") );

  // if items added, select first item and enable play button
  int nCount = ListView_GetItemCount( myHwndList );
  if (nCount != 0)
  {
    myHeader.SetSortCol( 0, TRUE );
    ListView_SortItems( myHwndList, ListViewCompareFunc, (LPARAM)this ); 
    ListView_SetItemState( myHwndList, 0, LVIS_SELECTED | LVIS_FOCUSED,
                           LVIS_SELECTED | LVIS_FOCUSED );
  }
  else
  {
    ::EnableWindow(::GetDlgItem( *this, IDC_PLAY), FALSE );
  }

  // Show status text
  TCHAR psz[256 + 1];
  TCHAR pszStatus[256 + 1];
  LoadString(m_hInstance, IDS_STATUSTEXT, pszStatus, 256);
  wsprintf( psz, pszStatus, nCount );
  hwndText = GetDlgItem( *this, IDC_ROMCOUNT );
  GetWindowRect(hwndText, &rc);
  ScreenToClient( *this, (LPPOINT)&rc );
  ScreenToClient( *this, ((LPPOINT)&rc)+1 );
  SetWindowText( hwndText, psz );
  InvalidateRect( *this, &rc, TRUE );

  // Show rom path
  hwndText = GetDlgItem( *this, IDC_ROMPATH );
  GetWindowRect(hwndText, &rc);
  ScreenToClient( *this, (LPPOINT)&rc );
  ScreenToClient( *this, ((LPPOINT)&rc)+1 );
  SetWindowText( hwndText, myGlobalData.settings().getString("romdir").c_str() );
  InvalidateRect( *this, &rc, TRUE );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
DWORD CMainDlg::PopulateRomList( void )
{
  DWORD dwRet;
  ClearList();

  TCHAR pszPath[ MAX_PATH ];

  lstrcpy( pszPath, myGlobalData.settings().getString("romdir").c_str() );
  lstrcat( pszPath, _T("\\*.bin") );

  WIN32_FIND_DATA ffd;
  HANDLE hFind = FindFirstFile( pszPath, &ffd );

  ListView_SetItemCount( myHwndList, 100 );

  int iItem = 0;
  BOOL fDone = (hFind == INVALID_HANDLE_VALUE);
  while(!fDone)
  {
    // File metadata will be read in ReadRomData()
    CListData* pListData  = new CListData;
    if( pListData == NULL )
      return ERROR_NOT_ENOUGH_MEMORY;

    dwRet = pListData->Initialize();
    if( dwRet != ERROR_SUCCESS )
      return dwRet;

    if ( !pListData->m_strFileName.Set( ffd.cFileName ) )
      return FALSE;

    LV_ITEM lvi;
    lvi.mask = LVIF_TEXT | LVIF_PARAM;
    lvi.iItem = iItem++;
    lvi.iSubItem = 0;
    lvi.pszText = LPSTR_TEXTCALLBACK;
    lvi.lParam = (LPARAM)pListData;
    int nItem = ListView_InsertItem( myHwndList, &lvi );

    ASSERT( nItem != -1 );

    ListView_SetItemText( myHwndList, nItem, 
        CListData::FILENAME_COLUMN, LPSTR_TEXTCALLBACK );
    ListView_SetItemText(myHwndList, nItem, 
        CListData::MANUFACTURER_COLUMN, LPSTR_TEXTCALLBACK);
    ListView_SetItemText( myHwndList, nItem, 
        CListData::RARITY_COLUMN, LPSTR_TEXTCALLBACK );

    // go to the next rom file
    fDone = !FindNextFile(hFind, &ffd);
  }

  if( hFind != INVALID_HANDLE_VALUE )
    VERIFY( ::FindClose( hFind ) );

  return ERROR_SUCCESS;
}

DWORD CMainDlg::ReadRomData(CListData* pListData) const
{
/*
  // TODO: Move this method to ListData class (?)
  if( pListData == NULL )
  {
    ASSERT( FALSE );
    return ERROR_BAD_ARGUMENTS;
  }

  // attempt to read the rom file
  TCHAR pszPath[MAX_PATH + 1];
  lstrcpy( pszPath, myGlobalData.settings().getString("romdir").c_str() );
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
  if(hFile == INVALID_HANDLE_VALUE)
    return GetLastError();

  DWORD dwFileSize = ::GetFileSize( hFile, NULL );
  BYTE* pImage = new BYTE[dwFileSize];
  if( pImage == NULL )
    return ERROR_NOT_ENOUGH_MEMORY;
    
  DWORD dwRead;
  if( ::ReadFile( hFile, pImage, dwFileSize, &dwRead, NULL ) )
  {
    // Read the file, now check the md5
    std::string md5 = MD5( pImage, dwFileSize );
        
    // search through the properties set for this MD5
    PropertiesSet& propertiesSet = m_stella.GetPropertiesSet();

        uInt32 setSize = propertiesSet.size();
        
        for (uInt32 i = 0; i < setSize; ++i)
        {
            if (propertiesSet.get(i).get("Cartridge.MD5") == md5)
            {
                // got it!
                break;
            }
        }
        
        if (i != setSize)
        {
            const Properties& properties = propertiesSet.get(i);
            
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
        else
        {
            //
            // Any output here should be appended to the emucore\stella.pro file
            //

            TRACE( "\"Cartridge.MD5\" \"%s\"\n\"Cartridge.Name\" \"%s\"\n\"\"\n", 
                   md5.c_str(), 
                   pListData->GetTextForColumn( CListData::FILENAME_COLUMN ) );
        }
    }
    
    delete[] pImage;
    
    VERIFY( ::CloseHandle( hFile ) );

    pListData->m_fPopulated = TRUE;
*/
    return ERROR_SUCCESS;
}

void CMainDlg::OnColumnClick( LPNMLISTVIEW pnmv )
{
    HCURSOR hcur = ::SetCursor(::LoadCursor(NULL, IDC_WAIT));

    myHeader.SetSortCol(pnmv->iSubItem, TRUE);
    ListView_SortItems(pnmv->hdr.hwndFrom, ListViewCompareFunc, (LPARAM)this);

    // ensure the selected item is visible

    int nItem = ListView_GetNextItem( myHwndList, -1, LVNI_SELECTED );
    if (nItem != -1)
    {
        ListView_EnsureVisible( myHwndList, nItem, TRUE );
    }

    ::SetCursor(hcur);
}

void CMainDlg::OnItemChanged( LPNMLISTVIEW pnmv )
{
  HWND hwnd = *this;

  HWND hwndNote = ::GetDlgItem( hwnd, IDC_ROMNOTE );

  RECT rc;
  GetWindowRect(hwndNote, &rc);
  ScreenToClient( hwnd, (LPPOINT)&rc );
  ScreenToClient( hwnd, ((LPPOINT)&rc)+1 );

  int iItem = ListView_GetNextItem(pnmv->hdr.hwndFrom, -1, LVNI_SELECTED);
  if (iItem == -1)
  {
    SetWindowText( hwndNote, _T("") );
    EnableWindow( ::GetDlgItem( hwnd, IDC_PLAY ), FALSE );
    InvalidateRect( hwnd, &rc, TRUE );
    return;
  }

  CListData* pListData = (CListData*)ListView_GetItemData( 
    pnmv->hdr.hwndFrom, 
    pnmv->iItem );

  SetWindowText( hwndNote, pListData->GetNote() );
  InvalidateRect( hwnd, &rc, TRUE );
  EnableWindow( ::GetDlgItem( hwnd, IDC_PLAY ), TRUE );
}

// ---------------------------------------------------------------------------
// LPSTR_TEXTCALLBACK message handlers
void CMainDlg::OnGetDispInfo(NMLVDISPINFO* pnmv)
{
  // Provide the item or subitem's text, if requested. 
  if( !(pnmv->item.mask & LVIF_TEXT ) )
    return;

  CListData* pListData = (CListData*)
             ListView_GetItemData( pnmv->hdr.hwndFrom, pnmv->item.iItem );
  ASSERT( pListData );
  if( pListData == NULL )
    return;

  if( !pListData->IsPopulated() )
    ReadRomData( pListData );

  pnmv->item.pszText = const_cast<LPTSTR>( pListData->GetTextForColumn(pnmv->item.iSubItem) );
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

    int nSortCol = pThis->myHeader.GetSortCol();

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

// ---------------------------------------------------------------------------
// Cool caption message handlers

void CMainDlg::OnDestroy(
    void
    )
{
    m_CoolCaption.OnDestroy();

    if ( m_hfontRomNote )
    {
        ::DeleteObject( m_hfontRomNote );
        m_hfontRomNote = NULL;
    }
}

void CMainDlg::OnNcPaint(
    HRGN hrgn
    )
{
    m_CoolCaption.OnNcPaint( hrgn );
}

void CMainDlg::OnNcActivate(
    BOOL fActive
    )
{
    m_CoolCaption.OnNcActivate( fActive );
}

BOOL CMainDlg::OnNcLButtonDown(
    INT nHitTest,
    POINTS pts
    )
{
    return m_CoolCaption.OnNCLButtonDown( nHitTest, pts );
}
