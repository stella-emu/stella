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
// Copyright (c) 1995-2002 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: StellaConfig.cpp,v 1.3 2003-11-24 01:14:38 stephena Exp $
//============================================================================

#include "pch.hxx"
#include "bspf.hxx"
#include "SettingsWin32.hxx"
#include "Cyberstella.h"
#include "StellaConfig.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StellaConfig::StellaConfig(SettingsWin32* settings, CWnd* pParent)
  : CDialog(StellaConfig::IDD, pParent),
    mySettings(settings)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StellaConfig::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BEGIN_MESSAGE_MAP(StellaConfig, CDialog)
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_CONTINUE, OnContinue)
END_MESSAGE_MAP()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BOOL StellaConfig::OnInitDialog() 
{
  CDialog::OnInitDialog();

  // Set up PADDLE
  CString paddle = "Paddle%d";
  for(uInt32 i = 0; i < 4; i++)
  {
    paddle.Format("Paddle %d",i);
    ((CComboBox*)GetDlgItem(IDC_PADDLE))->AddString(paddle);
  }

  uInt32 paddlemode = mySettings->getInt("paddle");
  if(paddlemode < 0 || paddlemode > 4)
    paddlemode = 0;
  ((CComboBox*)GetDlgItem(IDC_PADDLE))->SetCurSel(paddlemode);

  // Set up SOUND
  string sound = mySettings->getString("sound");
	((CButton*) GetDlgItem(IDC_SOUND))->SetCheck(
    sound == "0" ? BST_CHECKED : BST_UNCHECKED);

  // Set up AutoSelectVideoMode
  bool autoselect = mySettings->getBool("autoselect_video");
  ((CButton*)GetDlgItem(IDC_AUTO_SELECT_VIDEOMODE))->SetCheck
    (autoselect ? BST_CHECKED : BST_UNCHECKED);

  // Set up JOYSTICK
  bool joystick = mySettings->getBool("disable_joystick");
  ((CButton*)GetDlgItem(IDC_JOYSTICK))->SetCheck
    (joystick ? BST_CHECKED : BST_UNCHECKED);

  return TRUE;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StellaConfig::OnClose() 
{
  retrieveData();
  CDialog::OnClose();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BOOL StellaConfig::DestroyWindow() 
{
  retrieveData();
  return CDialog::DestroyWindow();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StellaConfig::retrieveData() 
{
  string tmp;

  // Apply changes
  mySettings->setInt("paddle", (uInt32) ((CComboBox*)GetDlgItem(IDC_PADDLE))->GetCurSel());

  if(((CButton*)GetDlgItem(IDC_SOUND))->GetCheck())
    mySettings->setString("sound", "0");
  else
    mySettings->setString("sound", "win32");

  mySettings->setBool("autoselect_video",
    ((CButton*)GetDlgItem(IDC_AUTO_SELECT_VIDEOMODE))->GetCheck());

  mySettings->setBool("joystick_disabled",
    ((CButton*)GetDlgItem(IDC_JOYSTICK))->GetCheck());

  // Save any settings that were changed
  mySettings->saveConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StellaConfig::OnContinue() 
{
  EndDialog(1);	
}
