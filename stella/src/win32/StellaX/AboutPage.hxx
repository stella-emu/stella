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
// $Id: AboutPage.hxx,v 1.3 2004-07-15 03:03:27 stephena Exp $
//============================================================================

#ifndef ABOUT_PAGE_HXX
#define ABOUT_PAGE_HXX

#include "PropertySheet.hxx"
#include "HyperLink.hxx"

class CHelpPage : public CPropertyPage
{
  public:
    CHelpPage();

  protected:
    virtual BOOL OnInitDialog( HWND hwnd );

  private:
    CHyperLink m_hlMail_JSM;
    CHyperLink m_hlWWW_JSM;
    CHyperLink m_hlMail_Stella;
    CHyperLink m_hlWWW_Stella;

    CHelpPage( const CHelpPage& );      // no implementation
    void operator=( const CHelpPage& ); // no implementation
};

#endif
