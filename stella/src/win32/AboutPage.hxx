//
// StellaX
// Jeff Miller 05/01/2000
//
#ifndef ABOUTPG_H
#define ABOUTPG_H
#pragma once

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
	CHyperLink m_hlWWW_Mame;

	CHelpPage( const CHelpPage& );  // no implementation
	void operator=( const CHelpPage& );  // no implementation
};

#endif
