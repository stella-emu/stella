// AboutDlg.cpp : implementation file
//

#include "pch.hxx"
#include "Cyberstella.h"
#include "AboutDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// AboutDlg dialog


AboutDlg::AboutDlg(CWnd* pParent /*=NULL*/)
	: CDialog(AboutDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(AboutDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void AboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(AboutDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(AboutDlg, CDialog)
	//{{AFX_MSG_MAP(AboutDlg)
	ON_BN_CLICKED(IDC_CONTINUE, OnContinue)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// AboutDlg message handlers

void AboutDlg::OnContinue() 
{
    EndDialog(1);	
}

BOOL AboutDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
  m_hlMail_JSM.SubclassDlgItem(IDC_EMAIL_MAINTAINER, this);
	m_hlMail_JSM.SetURL( _T("mailto:stephena@users.sourceforge.net?Subject=Cyberstella") );

	m_hlWWW_JSM.SubclassDlgItem(IDC_WEB_MAINTAINER, this);
	m_hlWWW_JSM.SetURL( _T("http://minbar.org") );

	m_hlMail_Stella.SubclassDlgItem(IDC_EMAIL_STELLA, this);
	m_hlMail_Stella.SetURL( _T("mailto:stella-main@lists.sourceforge.net") );

	m_hlWWW_Stella.SubclassDlgItem(IDC_WEB_STELLA, this);
	m_hlWWW_Stella.SetURL( _T("http://stella.sourceforge.net") );

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
