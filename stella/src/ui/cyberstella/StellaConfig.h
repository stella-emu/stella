#if !defined(AFX_STELLACONFIG_H__EECE0DA1_3FFF_11D6_ACFC_0048546D2F04__INCLUDED_)
#define AFX_STELLACONFIG_H__EECE0DA1_3FFF_11D6_ACFC_0048546D2F04__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "GlobalData.hxx"

/////////////////////////////////////////////////////////////////////////////
// StellaConfig dialog

class StellaConfig : public CDialog
{
// Construction
public:
	StellaConfig(CGlobalData* rGlobalData, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(StellaConfig)
	enum { IDD = IDD_CONFIG_PAGE };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(StellaConfig)
	public:
	virtual BOOL DestroyWindow();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(StellaConfig)
	virtual BOOL OnInitDialog();
	afx_msg void OnClose();
	afx_msg void OnBrowse();
	afx_msg void OnContinue();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
    // member
    CGlobalData* m_pGlobalData;
    //method
    void retrieveData();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STELLACONFIG_H__EECE0DA1_3FFF_11D6_ACFC_0048546D2F04__INCLUDED_)
