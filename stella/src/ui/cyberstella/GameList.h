#if !defined(AFX_GAMELIST_H__1EF8C3E1_4D6A_11D6_ACFC_0048546D2F04__INCLUDED_)
#define AFX_GAMELIST_H__1EF8C3E1_4D6A_11D6_ACFC_0048546D2F04__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// GameList.h : header file
//

#include "CRegBinding.h"
#include "PropsSet.hxx"

/////////////////////////////////////////////////////////////////////////////
// GameList window

class GameList : public CListCtrl
{
private:
    // memebers saved in registry
    CString path;
    PropertiesSet* m_pPropertiesSet;

    // Regbinding
    CRegBinding   rs;

    // methods
    void displayDrives();
    void displayPath();
    Properties* readRomData(CString binFile);

    // Construction
public:
	GameList();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(GameList)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~GameList();
    void insertColumns();
    void populateRomList();
    void setPropertiesSet(PropertiesSet* newPropertiesSet) 
         {m_pPropertiesSet = newPropertiesSet;}
    void deleteItemsAndProperties();
    CString getCurrentFile();
    CString getCurrentName();

	// Generated message map functions
protected:
	//{{AFX_MSG(GameList)
	afx_msg void OnColumnclick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemActivate(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GAMELIST_H__1EF8C3E1_4D6A_11D6_ACFC_0048546D2F04__INCLUDED_)
