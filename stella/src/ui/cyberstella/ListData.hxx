//
// StellaX
// Jeff Miller 05/10/2000
//
#ifndef MAINDLG_H
#define MAINDLG_H
#pragma once

#include "resource.h"
#include "pch.hxx"
#include "StellaXMain.hxx"

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

#endif
