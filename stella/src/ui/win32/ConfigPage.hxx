//
// StellaX
// Jeff Miller 05/07/2000
//

#ifndef CONFIGPG_H
#define CONFIGPG_H
#pragma once

#include "PropertySheet.hxx"
#include "GlobalData.hxx"

class CConfigPage : public CPropertyPage
{
public:

	CConfigPage(CGlobalData* rGlobalData);

protected:

	virtual BOOL OnInitDialog( HWND hwnd );
	virtual void OnDestroy();
	virtual LONG OnApply( LPPSHNOTIFY lppsn );

	virtual BOOL OnCommand( WORD /* wNotifyCode */, WORD /* wID */, HWND /* hwndCtl */ );

private:

    CGlobalData* m_rGlobalData;
    HWND m_hwnd;

	CConfigPage( const CConfigPage& );  // no implementation
	void operator=( const CConfigPage& );  // no implementation

};

#endif
