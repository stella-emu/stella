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
// $Id: AboutPage.cxx,v 1.2 2004-07-15 03:03:26 stephena Exp $
//============================================================================

#include "pch.hxx"
#include "AboutPage.hxx"
#include "resource.h"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
CHelpPage::CHelpPage()
         : CPropertyPage(IDD_ABOUT_PAGE)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
BOOL CHelpPage::OnInitDialog(	HWND hwnd )
{
  m_hlMail_JSM.SubclassDlgItem( hwnd, IDC_EMAIL_MAINTAINER );
  m_hlMail_JSM.SetURL( _T("mailto:sa666_666@hotmail.com?Subject=StellaX") );

  m_hlWWW_JSM.SubclassDlgItem( hwnd, IDC_WEB_MAINTAINER );
  m_hlWWW_JSM.SetURL( _T("http://minbar.org") );

  m_hlMail_Stella.SubclassDlgItem( hwnd, IDC_EMAIL_STELLA );
  m_hlMail_Stella.SetURL( _T("mailto:stella-main@lists.sourceforge.net") );

  m_hlWWW_Stella.SubclassDlgItem( hwnd, IDC_WEB_STELLA );
  m_hlWWW_Stella.SetURL( _T("http://stella.sf.net") );

  // return FALSE if SetFocus is called
  return TRUE;
}
