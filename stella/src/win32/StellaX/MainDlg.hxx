//
// StellaX
// Jeff Miller 05/10/2000
//
#ifndef MAINDLG_H
#define MAINDLG_H
#pragma once

#include "resource.h"

class CGlobalData;

#include "StellaXMain.hxx"
#include "CoolCaption.hxx"
#include "TextButton3d.hxx"
#include "HeaderCtrl.hxx"
#include "RoundButton.hxx"

class CMainDlg;

class CListData
{
    friend CMainDlg;

public:

    CListData() :
        m_fPopulated( FALSE )
        {
        }

    DWORD Initialize()
        {
            //
            // ListView control doesn't like NULLs returned, so initialize all
            //

            if ( ! m_strName.Set( _T("") ) )
            {
                return ERROR_NOT_ENOUGH_MEMORY;
            }

            if ( ! m_strManufacturer.Set( _T("") ) )
            {
                return ERROR_NOT_ENOUGH_MEMORY;
            }

            if ( ! m_strRarity.Set( _T("") ) )
            {
                return ERROR_NOT_ENOUGH_MEMORY;
            }

            if ( ! m_strFileName.Set( _T("") ) )
            {
                return ERROR_NOT_ENOUGH_MEMORY;
            }

            if ( ! m_strNote.Set( _T("") ) )
            {
                return ERROR_NOT_ENOUGH_MEMORY;
            }

            return ERROR_SUCCESS;
        }

    //
    // MetaData
    //

    static int GetColumnCount( void )
        {
            return 4;
        }

    enum ColumnIndex
        {
            FILENAME_COLUMN,
            NAME_COLUMN,
            MANUFACTURER_COLUMN,
            RARITY_COLUMN,
        };

    static UINT GetColumnNameIdForColumn( int nCol )
        {
            UINT uID = 0;

            switch ( nCol )
            {
            case NAME_COLUMN:
                uID = IDS_NAME;
                break;
                
            case MANUFACTURER_COLUMN:
                uID = IDS_MANUFACTURER;
                break;
                
            case RARITY_COLUMN:
                uID = IDS_RARITY;
                break;
                
            case FILENAME_COLUMN:
                uID = IDS_FILENAME;
                break;
                
            default:
                ASSERT(FALSE);
                break;
            }

            return uID;
        }

    LPCTSTR GetTextForColumn( int nCol ) const
        {
            LPCTSTR pszText = NULL;
            
            switch (nCol)
            {
            case NAME_COLUMN:
                pszText = m_strName.Get();
                break;
                
            case MANUFACTURER_COLUMN:
                pszText = m_strManufacturer.Get();
                break;
                
            case RARITY_COLUMN:
                pszText = m_strRarity.Get();
                break;
                
            case FILENAME_COLUMN:
                pszText = m_strFileName.Get();
                break;
                
            default:
                ASSERT( FALSE );
                break;
            }

            return pszText;
        }

    LPCTSTR GetNote( void ) const
        {
            return m_strNote.Get();
        }

    BOOL IsPopulated( void ) const
        {
            return m_fPopulated;
        }

private:

    CSimpleString m_strName;
    CSimpleString m_strManufacturer;
    CSimpleString m_strRarity;
    CSimpleString m_strFileName;
    CSimpleString m_strNote;
    BOOL m_fPopulated;

private:

CListData( const CListData& );  // no implementation
void operator=( const CListData& );  // no implementation

};

// ---------------------------------------------------------------------------

class CMainDlg
{
public:

    enum { IDD = IDD_MAIN };

    CMainDlg( CGlobalData& rGlobalData, HINSTANCE hInstance );

    virtual int DoModal( HWND hwndParent );

    operator HWND( void ) const
        {
            return myHwnd;
        }

private:

    HWND myHwnd;

    CCoolCaption  m_CoolCaption;
    CTextButton3d myAppTitle;
    CHeaderCtrl   myHeader;
    CRoundButton  myPlayButton;
    CRoundButton  myHelpButton;
    CRoundButton  myReloadButton;
    CRoundButton  myConfigButton;
    CRoundButton  myExitButton;

    //
    // Message handlers
    //

    BOOL OnInitDialog( void );
    BOOL OnCommand( int id, HWND hwndCtl, UINT codeNotify );
    BOOL OnNotify( int idCtrl, LPNMHDR pnmh );
    BOOL OnEraseBkgnd( HDC hdc );
    HBRUSH OnCtlColorStatic( HDC hdcStatic, HWND hwndStatic );

    //
    // wm_notify handlers
    //

    void OnGetDispInfo( NMLVDISPINFO* pnmh );

    void OnColumnClick( LPNMLISTVIEW pnmv );
    void OnItemChanged( LPNMLISTVIEW pnmv );

    //
    // cool caption handlers
    //

    void OnDestroy( void );
    void OnNcPaint( HRGN hrgn );
    void OnNcActivate( BOOL fActive );
    BOOL OnNcLButtonDown( INT nHitTest, POINTS pts );

    //
    // callback methods
    //

    BOOL CALLBACK DialogFunc( UINT uMsg, WPARAM wParam, LPARAM lParam );

    static BOOL CALLBACK StaticDialogFunc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
    static int CALLBACK ListViewCompareFunc( LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort );

    // internal data

    DWORD PopulateRomList();
    void UpdateRomList();
    DWORD ReadRomData( CListData* ) const;

    HINSTANCE m_hInstance;

    // stuff in list

    HWND myHwndList;
    void ClearList();

    HFONT m_hfontRomNote;

    // Stella stuff

    CGlobalData&    myGlobalData;
    CStellaXMain    m_stella;

CMainDlg( const CMainDlg& );  // no implementation
void operator=( const CMainDlg& );  // no implementation

};

#endif
