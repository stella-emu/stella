//
// StellaX
// Jeff Miller 05/12/2000
//
#ifndef DOCPG_H
#define DOCPG_H
#pragma once

#include "PropertySheet.hxx"
#include "ControlHost.hxx"
#include "HyperLink.hxx"

class CPDFControl : public CActiveXControl
{
public:

    CPDFControl() : CActiveXControl()
        {
        }

	REFCLSID GetCLSID() { return clsid; }
	void OnInitialUpdate();

protected:

	static const CLSID clsid;

private:

	CPDFControl( const CPDFControl& );  // no implementation
	void operator=( const CPDFControl& );  // no implementation

};

class CDocPage : public CPropertyPage
{
public:

	CDocPage();

protected:

	virtual BOOL OnInitDialog(HWND hwnd);
	virtual void OnDestroy();
    virtual void OnActivate( UINT state, HWND hwndActDeact, BOOL fMinimized );

private:

	CHyperLink m_hlAdobe;
	CControlHost* m_pHost;

	CDocPage( const CDocPage& );  // no implementation
	void operator=( const CDocPage& );  // no implementation

};

#endif
