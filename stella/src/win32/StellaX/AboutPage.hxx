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
// Copyright (c) 1995-2000 by Jeff Miller
// Copyright (c) 2004 by Stephen Anthony
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: AboutPage.hxx,v 1.2 2004-07-04 20:16:03 stephena Exp $
//============================================================================ 

#ifndef ABOUTPG_H
#define ABOUTPG_H

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
