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
// Copyright (c) 1995-1999 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: GameList.cpp,v 1.6 2003-11-24 23:56:10 stephena Exp $
//============================================================================

#include "pch.hxx"
#include "Cyberstella.h"
#include "GameList.h"
#include "MD5.hxx"
#include "SettingsWin32.hxx"

class CyberstellaView;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GameList::GameList()
  : myRomPath(""),
    myRomCount(0)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GameList::~GameList()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BEGIN_MESSAGE_MAP(GameList, CListCtrl)
	// {{AFX_MSG_MAP(GameList)
	ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, OnColumnclick)
	ON_NOTIFY_REFLECT(LVN_ITEMACTIVATE, OnItemActivate)
	ON_NOTIFY_REFLECT(LVN_ITEMCHANGED, OnItemchanged)
	// }}AFX_MSG_MAP
END_MESSAGE_MAP()

// Sort the item in reverse alphabetical order.
static int CALLBACK
MyCompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    CListCtrl* pListCtrl = (CListCtrl*) lParamSort;
    Properties* props1 = (Properties*) lParam1;
    Properties* props2 = (Properties*) lParam2;

    // Dots on top
    if(strcmpi(props1->get("Cartridge.Type").c_str(), "Dots") == 0)
        return 0;
    if(strcmpi(props2->get("Cartridge.Type").c_str(), "Dots") == 0)
        return 1;

    // Directories on top
    if((strcmpi(props1->get("Cartridge.Type").c_str(), "Directory") == 0)
      &&(strcmpi(props2->get("Cartridge.Type").c_str(), "Directory") != 0))
        return 0;
    if((strcmpi(props1->get("Cartridge.Type").c_str(), "Directory") != 0)
      &&(strcmpi(props2->get("Cartridge.Type").c_str(), "Directory") == 0))
        return 1;

    // Else compare by name
    return strcmpi(props1->get("Cartridge.Name").c_str(),
                   props2->get("Cartridge.Name").c_str());
}

void GameList::deleteItemsAndProperties() 
{
    // First delete all Properties
    for(int i = 0; i < GetItemCount(); i++)
    {
        Properties* props = (Properties*)GetItemData(i);
        if(props)   delete props;
    }

    // Second delete all items
    DeleteAllItems();
}

void GameList::OnColumnclick(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here

	*pResult = 0;
}

void GameList::insertColumns() 
{
	CRect rect;
	int width;
    CString str;

	GetClientRect(&rect);
	width = (rect.Width()-20)/4;

	// Insert Columns
    str.LoadString(IDS_FILE);
	InsertColumn (0, str, LVCFMT_LEFT, width);
    str.LoadString(IDS_NAME);
	InsertColumn (1, str, LVCFMT_LEFT, width);
    str.LoadString(IDS_MANUFACTURER);
	InsertColumn (2, str, LVCFMT_LEFT, width);
    str.LoadString(IDS_RARITY);
	InsertColumn (3, str, LVCFMT_LEFT, width);
}

void GameList::populateRomList()
{
  CWaitCursor c;

  // Remove previous content
  deleteItemsAndProperties();

  // Add new content
  if(myRomPath.GetLength() > 0)
    displayPath();
  else
    displayDrives();

  // Sort content
  SortItems(MyCompareProc, (LPARAM)(CListCtrl*)this);

  // Select first item
  SetItemState(0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);

  if(myParent)
    myParent->SendMessage(MSG_GAMELIST_UPDATE);

  // Save the current path
  mySettings->setString("rompath", (const char*) myRomPath);
}

void GameList::displayPath()
{
	CFileFind find;
	CString name, searchpath;
	BOOL bFind;
	LVITEM lvi;
    int itemCounter = 0;
	BOOL first = true;

	// Do pathname
	if (myRomPath.GetAt(myRomPath.GetLength()-1) == '\\')
		searchpath = myRomPath + "*.*";
	else
		searchpath = myRomPath + "\\*.*";
  
	bFind = find.FindFile(searchpath);

    // Init Rom count
    myRomCount = 0;

	while (bFind)
	{
		bFind = find.FindNextFile();
		if (find.IsDirectory())
		{
			if (!find.IsDots())
			{
                name = find.GetFileName();
                lvi.mask = LVIF_IMAGE | LVIF_TEXT | LVIF_PARAM;
                lvi.iItem = itemCounter++;
                lvi.iSubItem = 0;
			    lvi.pszText = name.GetBuffer(name.GetLength());

                // set fake properties
                Properties* props = new Properties();
                props->set("Cartridge.Type", "Directory");
                props->set("Cartridge.Name", name.GetBuffer(name.GetLength()));
                lvi.lParam = (LPARAM)props;

                //Set folder icon
                lvi.iImage = 0;

			    InsertItem(&lvi);
            }
        }
		else 
		{
            name = find.GetFileName();
            
            // Bail out if if this is not a *.BIN file
            CString right = name.Right(4);
            if (strcmpi(right, ".bin") != 0) continue;
            // Bail out if we don't get properties
            Properties* props = readRomData(find.GetFilePath());
            if(props == NULL) continue;

            lvi.mask = LVIF_IMAGE | LVIF_TEXT | LVIF_PARAM;
            lvi.iItem = itemCounter++;
            lvi.iSubItem = 0;

            lvi.pszText = name.GetBuffer(name.GetLength());
            lvi.lParam = (LPARAM)props;

            // Set atari icon
            lvi.iImage = 1;

			InsertItem(&lvi);

            lvi.mask = LVIF_TEXT;

            lvi.iSubItem = 1;
            name = props->get("Cartridge.Name").c_str();

            // If entry is not found in stella.pro, set filename
            if(strcmpi(name,"untitled") == 0)
            {
                props->set("Cartridge.Name", find.GetFileName().GetBuffer(name.GetLength()));
            }

            lvi.pszText = name.GetBuffer(name.GetLength());
			SetItem(&lvi);

            lvi.iSubItem = 2;
            name = props->get("Cartridge.Manufacturer").c_str();
            lvi.pszText = name.GetBuffer(name.GetLength());
			SetItem(&lvi);
			
			lvi.iSubItem = 3;
            name = props->get("Cartridge.Rarity").c_str();
            lvi.pszText = name.GetBuffer(name.GetLength());
			SetItem(&lvi);

            myRomCount++;
        }
	}

    lvi.mask = LVIF_IMAGE | LVIF_TEXT | LVIF_PARAM;
    lvi.iItem = itemCounter++;
    lvi.iSubItem = 0;
	lvi.pszText = "..";
    
    // set fake properties
    Properties* props = new Properties();
    props->set("Cartridge.Type", "Dots");
    lvi.lParam = (LPARAM)props;

    //Set folder icon
    lvi.iImage = 0;

    InsertItem(&lvi);
}

void GameList::displayDrives()
{
    DWORD dwDrives, dwMask;
	LVITEM lvi;
    bool first;
    int itemCounter;

    // Clear path
    myRomPath = "";

    //Enumerate drive letters and add them to list
    dwDrives = GetLogicalDrives();
    dwMask = 1;

    first = true;
    itemCounter = 0;

    for (int i=0; i<32; i++)
    {
        if (dwDrives & dwMask)
        {
            CString sDrive;

            sDrive.Format(_T("%c:\\"), i + _T('A'));

			lvi.mask = LVIF_IMAGE | LVIF_TEXT | LVIF_PARAM;
            lvi.iItem = itemCounter++;
            lvi.iSubItem = 0;
			lvi.pszText = sDrive.GetBuffer(sDrive.GetLength());

            // set fake properties
            Properties* props = new Properties();
            props->set("Cartridge.Type", "Directory");
            props->set("Cartridge.Name", sDrive.GetBuffer(sDrive.GetLength()));
            lvi.lParam = (LPARAM)props;

            //Set folder icon
            lvi.iImage = 0;

			InsertItem(&lvi);
       }
        dwMask <<= 1;
    }
}

void GameList::OnItemActivate(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	int iItem = pNMListView->iItem;

    if(iItem >= 0)
	{
        Properties* props = (Properties*)GetItemData(iItem);
        CString dir = GetItemText(iItem,0);

        if(strcmpi(props->get("Cartridge.Type").c_str(), "Dots") == 0)
        {
            int cutPos = myRomPath.ReverseFind('\\');
            myRomPath = myRomPath.Left(cutPos);
            populateRomList();
        }
        else if(strcmpi(props->get("Cartridge.Type").c_str(), "Directory") == 0)
        {
            // Do pathname
            if (myRomPath.GetLength() <= 0)
            {
                myRomPath = dir;
            }
            else if (myRomPath.GetAt(myRomPath.GetLength()-1) != '\\')
            {
	            myRomPath += "\\";
                myRomPath += dir;
            }
            else
            {
                myRomPath += dir;
            }
            populateRomList();
        }
        else
        {
            // Notify parent to play the current game by 
            // sending a faked 'Play Button Pressed' message.
            if (myParent)  myParent->PostMessage(WM_COMMAND, BN_CLICKED | IDC_PLAY);
        }
    }
    *pResult = 0;
}

Properties* GameList::readRomData(CString binFile)
{
    HANDLE hFile;
    
    hFile = CreateFile(binFile, GENERIC_READ, FILE_SHARE_READ, NULL, 
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        return NULL;
    }

    DWORD dwFileSize = ::GetFileSize(hFile, NULL);

    BYTE* pImage = new BYTE[dwFileSize];
    if (pImage == NULL)
    {
        return NULL;
    }
    
    DWORD dwRead;

    if (::ReadFile(hFile, pImage, dwFileSize, &dwRead, NULL))
    {
        // Read the file, now check the md5
        std::string md5 = MD5(pImage, dwFileSize);
        // search through the properties set for this MD5
        Properties* props = new Properties();
        myPropertiesSet->getMD5(md5, *props);
        // Return properties
        delete[] pImage;
        VERIFY(::CloseHandle(hFile));
        return props;
    }
    
    delete[] pImage;
    VERIFY(::CloseHandle(hFile));
    return NULL;
}

CString GameList::getCurrentFile()
{
    CString filename = "";

    int curSel = GetSelectionMark();
    if(curSel >= 0)
    {
        if (myRomPath.GetAt(myRomPath.GetLength()-1) != '\\')
	        myRomPath += "\\";

        filename = myRomPath + GetItemText(curSel,0);
    }

    return filename;
}

CString GameList::getCurrentName()
{
    int curSel = GetSelectionMark();
    if(curSel >= 0)
    {
        return GetItemText(curSel,1);
    }
    return "";
}

CString GameList::getCurrentNote()
{
    int curSel = GetSelectionMark();
    if(curSel >= 0)
    {
        Properties* props = (Properties*)GetItemData(curSel);
        return props->get("Cartridge.Note").c_str();
    }
    return "";
}

void GameList::init(PropertiesSet* newPropertiesSet,
                    SettingsWin32* settings, CWnd* newParent)
{
  myParent = newParent;
  myPropertiesSet = newPropertiesSet;
  mySettings = settings;

  myRomPath = mySettings->getString("rompath").c_str();
  SetExtendedStyle(LVS_EX_FULLROWSELECT);
  insertColumns();
}


void GameList::OnItemchanged(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

    if(myParent)   myParent->SendMessage(MSG_GAMELIST_DISPLAYNOTE);

    *pResult = 0;
}

BOOL GameList::PreTranslateMessage(MSG* pMsg) 
{
	if( pMsg->message == WM_KEYDOWN )
	{
		if(pMsg->wParam == VK_RETURN)
        {
            ::TranslateMessage(pMsg);
			::DispatchMessage(pMsg);
			return TRUE;		    	// DO NOT process further
		}
	}

	return CListCtrl::PreTranslateMessage(pMsg);
}