// StellaConfig.cpp : implementation file
//

#include "pch.hxx"
#include "Cyberstella.h"
#include "StellaConfig.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// StellaConfig dialog


StellaConfig::StellaConfig(CGlobalData* rGlobalData, CWnd* pParent /*=NULL*/)
	: CDialog(StellaConfig::IDD, pParent)
      ,m_pGlobalData(rGlobalData)
{
    //{{AFX_DATA_INIT(StellaConfig)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void StellaConfig::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(StellaConfig)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(StellaConfig, CDialog)
	//{{AFX_MSG_MAP(StellaConfig)
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_CONTINUE, OnContinue)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// StellaConfig message handlers

BOOL StellaConfig::OnInitDialog() 
{
	CDialog::OnInitDialog();

    int i;

    // Set up PADDLE
    CString paddle = "Paddle%d";
    for (i=0; i<4; i++)
    {
        paddle.Format("Paddle %d",i);
        ((CComboBox*)GetDlgItem(IDC_PADDLE))->AddString(paddle);
    }
    ((CComboBox*)GetDlgItem(IDC_PADDLE))->SetCurSel(m_pGlobalData->iPaddleMode);

    // Set up SOUND
	((CButton*)GetDlgItem(IDC_SOUND))->SetCheck(m_pGlobalData->bNoSound ? BST_CHECKED : BST_UNCHECKED);

    // Set up AutoSelectVideoMode
	((CButton*)GetDlgItem(IDC_AUTO_SELECT_VIDEOMODE))->SetCheck(m_pGlobalData->bAutoSelectVideoMode ? BST_CHECKED : BST_UNCHECKED);

    // Set up JOYSTICK
	((CButton*)GetDlgItem(IDC_JOYSTICK))->SetCheck(m_pGlobalData->bJoystickIsDisabled ? BST_CHECKED : BST_UNCHECKED);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void StellaConfig::OnClose() 
{
    retrieveData();
	CDialog::OnClose();
}

BOOL StellaConfig::DestroyWindow() 
{
    retrieveData();
	return CDialog::DestroyWindow();
}

void StellaConfig::retrieveData() 
{
    // Apply changes
    m_pGlobalData->iPaddleMode = ((CComboBox*)GetDlgItem(IDC_PADDLE))->GetCurSel();
    m_pGlobalData->bNoSound = ((CButton*)GetDlgItem(IDC_SOUND))->GetCheck();
    m_pGlobalData->bAutoSelectVideoMode = ((CButton*)GetDlgItem(IDC_AUTO_SELECT_VIDEOMODE))->GetCheck();
    m_pGlobalData->bJoystickIsDisabled = ((CButton*)GetDlgItem(IDC_JOYSTICK))->GetCheck();

    // Set modify flag
    m_pGlobalData->bIsModified = true;
}

void StellaConfig::OnContinue() 
{
    EndDialog(1);	
}
