#if !defined(AFX_ABOUTDLG_H__1E4CE741_3D76_11D6_ACFC_0048546D2F04__INCLUDED_)
#define AFX_ABOUTDLG_H__1E4CE741_3D76_11D6_ACFC_0048546D2F04__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AboutDlg.h : header file
//

#include "HyperLink.h"

/////////////////////////////////////////////////////////////////////////////
// AboutDlg dialog

class AboutDlg : public CDialog
{
// Construction
public:
	AboutDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(AboutDlg)
	enum { IDD = IDD_ABOUTBOX };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(AboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(AboutDlg)
	afx_msg void OnContinue();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	CHyperLink m_hlMail_JSM;
	CHyperLink m_hlWWW_JSM;
	CHyperLink m_hlMail_Stella;
	CHyperLink m_hlWWW_Stella;
	CHyperLink m_hlWWW_Mame;

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ABOUTDLG_H__1E4CE741_3D76_11D6_ACFC_0048546D2F04__INCLUDED_)
