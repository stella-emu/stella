//============================================================================
//
//   SSSS    tt          lll  lll          XX     XX
//  SS  SS   tt           ll   ll           XX   XX
//  SS     tttttt  eeee   ll   ll   aaaa     XX XX
//   SSSS    tt   ee  ee  ll   ll      aa     XXX
//      SS   tt   eeeeee  ll   ll   aaaaa    XX XX
//  SS  SS   tt   ee      ll   ll  aa  aa   XX   XX
//   SSSS     ttt  eeeee llll llll  aaaaa  XX     XX
//
// Copyright (c) 1995-2000 by Jeff Miller
// Copyright (c) 2004 by Stephen Anthony
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: MainDlg.cxx,v 1.10 2004-09-14 19:10:29 stephena Exp $
//============================================================================

#include "pch.hxx"
#include "MainDlg.hxx"
#include "Game.hxx"
#include "GlobalData.hxx"
#include "PropertySheet.hxx"
#include "AboutPage.hxx"
#include "ConfigPage.hxx"
#include "resource.h"
#include "Settings.hxx"
#include "PropsSet.hxx"
#include "MD5.hxx"

#define BKGND_BITMAP_TOP     64
#define BKGND_BITMAP_BOTTOM  355

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
MainDlg::MainDlg( CGlobalData& rGlobalData, HINSTANCE hInstance )
        : myGlobalData(rGlobalData),
          myHInstance(hInstance)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
MainDlg::~MainDlg( void )
{
  // Just to be safe, make sure we don't have a memory leak
  ListView_Clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
int MainDlg::DoModal( HWND hwndParent )
{
  return DialogBoxParam( myHInstance, 
                         MAKEINTRESOURCE(IDD),
                         hwndParent, 
                         StaticDialogFunc, 
                         (LPARAM)this );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
BOOL CALLBACK
MainDlg::StaticDialogFunc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
  MainDlg* pDlg;

  switch ( uMsg )
  {
    case WM_INITDIALOG:
      pDlg = reinterpret_cast<MainDlg*>( lParam );
      pDlg->myHwnd = hDlg;
      (void)::SetWindowLong( hDlg, DWL_USER, reinterpret_cast<LONG>( pDlg ) );
      break;

    default:
      pDlg = reinterpret_cast<MainDlg*>( ::GetWindowLong( hDlg, DWL_USER ) );
      if ( pDlg == NULL )
        return FALSE;
      break;
  }

  return pDlg->DialogFunc( uMsg, wParam, lParam );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
BOOL CALLBACK
MainDlg::DialogFunc( UINT uMsg, WPARAM wParam, LPARAM lParam )
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
      return OnInitDialog();

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
      OnDestroy();
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

    case WM_CLOSE:
      Quit();
      break;
  }

  // Message not handled
  return FALSE;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
BOOL MainDlg::OnInitDialog( void  )
{
  DWORD dwRet;

  HWND hwnd = *this;

  // Set dialog icon
  HICON hicon = ::LoadIcon(myHInstance, MAKEINTRESOURCE(IDI_APP));
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
  myCoolCaption.OnInitDialog( hwnd );
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

  // Declare the column headings
  int columnHeader[] = { IDS_NAME, IDS_MANUFACTURER, IDS_RARITY };

  // Set the column widths
  int columnWidth[3];
  columnWidth[0] = myGlobalData.settings().getInt("namecolwidth");
  columnWidth[1] = myGlobalData.settings().getInt("manufacturercolwidth");
  columnWidth[2] = myGlobalData.settings().getInt("raritycolwidth");

  // Make sure there are sane values for the column widths
  if (columnWidth[0] <= 0 || columnWidth[1] <= 0 || columnWidth[2] <= 0)
  {
    LONG lTotalWidth = rc.right-rc.left - GetSystemMetrics(SM_CXVSCROLL);
    columnWidth[0] = (int) (0.58 * lTotalWidth);
    columnWidth[1] = (int) (0.25 * lTotalWidth);
    columnWidth[2] = lTotalWidth - columnWidth[0] - columnWidth[1];
  }

  // Set up the column headings
  for (int i = 0; i < 3; ++i)
  {
    LoadString( myHInstance, columnHeader[i], psz, nMaxString );

    LV_COLUMN lvc;
    lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
    lvc.fmt = LVCFMT_LEFT;
    lvc.cx = columnWidth[i];
    lvc.pszText = psz;
    ListView_InsertColumn( myHwndList, i, &lvc );
  }

  // Update the ROM game list
  UpdateRomList();

  // Set default button
  SendMessage( hwnd, DM_SETDEFID, IDC_PLAY, 0 );

  // return FALSE if SetFocus is called
  return TRUE;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
BOOL MainDlg::OnCommand( int id, HWND hwndCtl, UINT codeNotify )
{
  UNUSED_ALWAYS( hwndCtl );
  UNUSED_ALWAYS( codeNotify );

  HWND hwnd = *this;
  Game* g;
  string romfile;
  int nItem;

  switch (id)
  {
    case IDC_PLAY:
      nItem = (int)::SendMessage( myHwndList, LVM_GETNEXTITEM, 
                                  (WPARAM)-1, MAKELPARAM( LVNI_SELECTED, 0 ) );
      ASSERT( nItem != -1 );
      if ( nItem == -1 )
      {
        MessageBox( myHInstance, hwnd, IDS_NO_ITEM_SELECTED );
        return TRUE;
      }

      g = (Game*) ListView_GetItemData( myHwndList, nItem );
      romfile = myGlobalData.settings().getString("romdir") + "\\" + g->rom(); 
      (void)m_stella.PlayROM( romfile, myGlobalData );

      // Set focus back to the rom list
      SetFocus( myHwndList );

      return TRUE;
      break; // case IDC_PLAY

    case IDC_EXIT:
      Quit();
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
      UpdateRomList( true );
      return TRUE;
      break; // case IDC_RELOAD
    }
  }

  return FALSE;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
BOOL MainDlg::OnNotify( int idCtrl, LPNMHDR pnmh )
{
  UNUSED_ALWAYS( idCtrl );

  switch ( pnmh->code )
  {
    case LVN_ITEMCHANGED:
      OnItemChanged( (LPNMLISTVIEW)pnmh );
      return TRUE;

    case LVN_COLUMNCLICK:
      OnColumnClick( (LPNMLISTVIEW)pnmh );
      return TRUE;

    case NM_DBLCLK:
      // send out an ok click to play
      SendDlgItemMessage( *this, IDC_PLAY, BM_CLICK, 0, 0 );
      return TRUE;
  }

  // not handled
  return FALSE;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
void MainDlg::OnItemChanged( LPNMLISTVIEW pnmv )
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
    EnableWindow( GetDlgItem( hwnd, IDC_PLAY ), FALSE );
    InvalidateRect( hwnd, &rc, TRUE );
    return;
  }

  Game* g = (Game*)ListView_GetItemData( pnmv->hdr.hwndFrom, pnmv->iItem );
  SetWindowText( hwndNote, g->note().c_str() );
  InvalidateRect( hwnd, &rc, TRUE );
  EnableWindow( GetDlgItem( hwnd, IDC_PLAY ), TRUE );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
void MainDlg::OnColumnClick( LPNMLISTVIEW pnmv )
{
  ListView_SortByColumn( pnmv->hdr.hwndFrom, pnmv->iSubItem );
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
BOOL MainDlg::OnEraseBkgnd( HDC hdc )
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

  HBITMAP hbmpTile = LoadBitmap( myHInstance, MAKEINTRESOURCE(IDB_TILE) );

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
HBRUSH MainDlg::OnCtlColorStatic( HDC hdcStatic, HWND hwndStatic )
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
void MainDlg::Quit()
{
  // OK, reload the settings to make sure we have the most current ones
  myGlobalData.settings().loadConfig();

  // Save the current sort column
  int sortcol = myHeader.GetSortCol();
  myGlobalData.settings().setInt("sortcol", sortcol);

  // Save the column widths
  myGlobalData.settings().setInt("namecolwidth",
    ListView_GetColWidth( myHwnd, 0 ));
  myGlobalData.settings().setInt("manufacturercolwidth",
    ListView_GetColWidth( myHwnd, 1 ));
  myGlobalData.settings().setInt("raritycolwidth",
    ListView_GetColWidth( myHwnd, 2 ));

  // Now, save the settings
  myGlobalData.settings().saveConfig();

  ListView_Clear();
  EndDialog( myHwnd, IDCANCEL );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
void MainDlg::UpdateRomList( bool forceReload )
{
  HWND hwndText;
  RECT rc;
  TCHAR psz[256 + 1];
  TCHAR pszStatus[256 + 1];

  // Erase status text
  LoadString(myHInstance, IDS_STATUSTEXT, pszStatus, 256);
  wsprintf( psz, pszStatus, 0 );
  hwndText = GetDlgItem( *this, IDC_ROMCOUNT );
  GetWindowRect(hwndText, &rc);
  ScreenToClient( *this, (LPPOINT)&rc );
  ScreenToClient( *this, ((LPPOINT)&rc)+1 );
  SetWindowText( hwndText, psz );
  InvalidateRect( *this, &rc, TRUE );

  // Erase rom path
  hwndText = GetDlgItem( *this, IDC_ROMPATH );
  GetWindowRect(hwndText, &rc);
  ScreenToClient( *this, (LPPOINT)&rc );
  ScreenToClient( *this, ((LPPOINT)&rc)+1 );
  SetWindowText( hwndText, "" );
  InvalidateRect( *this, &rc, TRUE );

  // Get the ROM gamelist, either from disk or cache
  PopulateRomList( forceReload );

  // if items added, select first item and enable play button
  int nCount = ListView_GetItemCount( myHwndList );
  if (nCount == 0)
    EnableWindow(GetDlgItem( *this, IDC_PLAY), FALSE );

  // Show status text
  LoadString(myHInstance, IDS_STATUSTEXT, pszStatus, 256);
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
bool MainDlg::PopulateRomList( bool forceReload )
{
  bool result = false;
  bool cacheFileExists = myGlobalData.settings().fileExists("stellax.cache");
  bool cacheIsStale = false; // FIXME - add romdir status checking

  if (forceReload)
      result = LoadRomListFromDisk();
  else if (cacheFileExists && !cacheIsStale)
  {
    result = LoadRomListFromCache();
    if (!result)
    {
      MessageBox( myHInstance, myHwnd, IDS_CORRUPT_CACHE_FILE );
      result = LoadRomListFromDisk();
    }
  }
  else
  {
    if (!cacheFileExists)
      MessageBox( myHInstance, myHwnd, IDS_NO_CACHE_FILE );
    else if (cacheIsStale)
      MessageBox( myHInstance, myHwnd, IDS_ROMDIR_CHANGED );

    result = LoadRomListFromDisk();
  }

  ListView_SortByColumn( myHwndList, myGlobalData.settings().getInt("sortcol") );
  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
bool MainDlg::LoadRomListFromDisk()
{
  ListView_Clear();

  // Get the location of the romfiles (*.bin)
  string romdir = myGlobalData.settings().getString("romdir");
  romdir += "\\";
  string romfiles = romdir + "*";

  WIN32_FIND_DATA ffd;
  HANDLE hFind = FindFirstFile( romfiles.c_str(), &ffd );

  ListView_SetItemCount( myHwndList, 100 );

  int iItem = 0;
  bool done = (hFind == INVALID_HANDLE_VALUE);
  while(!done)
  {
    // Make sure we're only looking at files
    if( !(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
    {
      Game* g = new Game();
      if( g == NULL )
        return false;

      LV_ITEM lvi;
      lvi.mask = LVIF_TEXT | LVIF_PARAM;
      lvi.iItem = iItem++;
      lvi.iSubItem = 0;
      lvi.pszText = "";
      lvi.lParam = (LPARAM) g;
      int nItem = ListView_InsertItem( myHwndList, &lvi );

      g->setAvailable( true );
      g->setRom( ffd.cFileName );

      // Set the name (shown onscreen) to be the rom
      // It will be overwritten later if a name is found in the properties set
      g->setName( ffd.cFileName );
    }

    // go to the next rom file
    done = !FindNextFile(hFind, &ffd);
  }

  if( hFind != INVALID_HANDLE_VALUE )
    VERIFY( ::FindClose( hFind ) );

  // Create a propertiesset object and load the properties
  // We don't create the propsSet until we need it, since it's
  // a time-consuming process
  PropertiesSet propsSet;
  string theProperties = myGlobalData.settings().propertiesInputFilename();
  if(theProperties != "")
    propsSet.load(theProperties, true);
  else
    propsSet.load("", false);

  // Now, rescan the items in the listview and update the onscreen
  // text and game properties
  // Also generate a cache file so the program will start faster next time
  ofstream cache("stellax.cache");
  int count = ListView_GetItemCount( myHwndList );
  cache << count << endl;
  for (int i = 0; i < count; ++i)
  {
    Game* g = (Game*) ListView_GetItemData(myHwndList, i);

    // Get the MD5sum for this rom
    // Use it to lookup the rom in the properties set
    string rom = romdir + g->rom();
    ifstream in(rom.c_str(), ios_base::binary);
    if(!in)
      continue;
    uInt8* image = new uInt8[512 * 1024];
    in.read((char*)image, 512 * 1024);
    uInt32 size = in.gcount();
    in.close(); 
    string md5 = MD5( image, size );
    delete[] image; 

    // Get the properties for this rom
    Properties props;
    propsSet.getMD5( md5, props );

    // For some braindead reason, the ListView_SetItemText won't
    // allow std::string::c_str(), so we create C-strings instead
    char name[256], manufacturer[256], rarity[256];
    strncpy(name, props.get("Cartridge.Name").c_str(), 255);
    strncpy(manufacturer, props.get("Cartridge.Manufacturer").c_str(), 255);
    strncpy(rarity, props.get("Cartridge.Rarity").c_str(), 255);

    // Make sure the onscreen 'Name' field isn't blank
    if(props.get("Cartridge.Name") == "Untitled")
      strncpy(name, g->name().c_str(), 255);

    // Update the current game
    g->setMd5( md5 );
    g->setName( name );
    g->setRarity( rarity );
    g->setManufacturer( manufacturer );
    g->setNote( props.get("Cartridge.Note") );

    // Update the cachefile with this game
    cache << g->rom() << endl
          << md5 << endl
          << name << endl
          << rarity << endl
          << manufacturer << endl
          << g->note() << endl;

    // Finally, update the listview itself
    ListView_SetItemText( myHwndList, i, 0, name );
    ListView_SetItemText( myHwndList, i, 1, manufacturer );
    ListView_SetItemText( myHwndList, i, 2, rarity );
  }
  cache.close();
  SetFocus( myHwndList );

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
bool MainDlg::LoadRomListFromCache()
{
  ListView_Clear();
  char count[10], rom[256], md5[256], name[256], rarity[256],
       manufacturer[256], note[256];

  // We don't scan the roms at all; just load directly from the cache file
  ifstream in("stellax.cache");
  if (!in)
    return false;

  in.getline(count, 9);
  int numRoms = atoi(count);
  ListView_SetItemCount( myHwndList, 100 );

  int iItem = 0;
  for(int i = 0; i < numRoms; i++)
  {
    string line;
    Game* g = new Game();
    if( g == NULL )
      return false;

    // Get the data from the cache file
    in.getline(rom, 255);
    in.getline(md5, 255);
    in.getline(name, 255);
    in.getline(rarity, 255);
    in.getline(manufacturer, 255);
    in.getline(note, 255);

    // And save it to this game object
    g->setAvailable( true );
    g->setRom( rom );
    g->setMd5( md5 );
    g->setName( name );
    g->setRarity( rarity );
    g->setManufacturer( manufacturer );
    g->setNote( note );

    LV_ITEM lvi;
    lvi.mask = LVIF_TEXT | LVIF_PARAM;
    lvi.iItem = iItem++;
    lvi.iSubItem = 0;
    lvi.pszText = "";
    lvi.lParam = (LPARAM) g;
    int nItem = ListView_InsertItem( myHwndList, &lvi );

    ASSERT( nItem != -1 );

    ListView_SetItemText( myHwndList, nItem, 0, name );
    ListView_SetItemText( myHwndList, nItem, 1, manufacturer );
    ListView_SetItemText( myHwndList, nItem, 2, rarity );
  }
  SetFocus( myHwndList );

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
// Cool caption message handlers
void MainDlg::OnDestroy( void )
{
  myCoolCaption.OnDestroy();

  if ( m_hfontRomNote )
  {
    DeleteObject( m_hfontRomNote );
    m_hfontRomNote = NULL;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
void MainDlg::OnNcPaint( HRGN hrgn )
{
  myCoolCaption.OnNcPaint( hrgn );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
void MainDlg::OnNcActivate( BOOL fActive )
{
  myCoolCaption.OnNcActivate( fActive );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
BOOL MainDlg::OnNcLButtonDown( INT nHitTest, POINTS pts )
{
  return myCoolCaption.OnNCLButtonDown( nHitTest, pts );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
LPARAM MainDlg::ListView_GetItemData( HWND hwndList, int iItem )
{
  LVITEM lvi;
  lvi.mask = LVIF_PARAM;
  lvi.iItem = iItem;
  lvi.iSubItem = 0;

  ListView_GetItem(hwndList, &lvi);

  return lvi.lParam;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
void MainDlg::ListView_SortByColumn( HWND hwndList, int col )
{
  HCURSOR hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));

  int nCount = ListView_GetItemCount( hwndList );
  if (nCount != 0)
  {
    myHeader.SetSortCol( col, TRUE );
    ListView_SortItems( hwndList, ListViewCompareFunc, (LPARAM)this ); 
    ListView_SetItemState( hwndList, 0, LVIS_SELECTED | LVIS_FOCUSED,
                           LVIS_SELECTED | LVIS_FOCUSED );
  }

  // ensure the selected item is visible
  int nItem = ListView_GetNextItem( myHwndList, -1, LVNI_SELECTED );
  if (nItem != -1)
    ListView_EnsureVisible( myHwndList, nItem, TRUE );

  SetCursor(hcur);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
int MainDlg::ListView_GetColWidth( HWND hwndList, int col )
{
  // Although there seems to be a similar function in the Win32 API
  // to do this, I couldn't get it to work, so it's quicker to
  // write this one and use it ...
  LV_COLUMN lvc;
  lvc.mask = LVCF_WIDTH;
  if (ListView_GetColumn( myHwndList, col, &lvc ) == TRUE)
    return lvc.cx;
  else
    return 0;  // the next time StellaX starts, it will recreate a sane value
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
void MainDlg::ListView_Clear( void )
{
  int nCount = ListView_GetItemCount( myHwndList );

  for (int i = 0; i < nCount; ++i)
    delete (Game*) ListView_GetItemData( myHwndList, i );

  ListView_DeleteAllItems( myHwndList );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
int CALLBACK
MainDlg::ListViewCompareFunc( LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort )
{
  MainDlg* dlg = reinterpret_cast<MainDlg*>( lParamSort );

  int sortCol = dlg->myHeader.GetSortCol();

  Game* g1 = reinterpret_cast<Game*>( lParam1 );
  Game* g2 = reinterpret_cast<Game*>( lParam2 );

  string s1 = "", s2 = "";
  switch (sortCol)
  {
    case 0:
      s1 = g1->name();
      s2 = g2->name();
      break;

    case 1:
      s1 = g1->manufacturer();
      s2 = g2->manufacturer();
      break;

    case 2:
      s1 = g1->rarity();
      s2 = g2->rarity();
      break;
  }

  if (s1 > s2)
    return 1;
  else if (s1 < s2)
    return -1;
  else
    return 0;
}
