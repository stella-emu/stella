//
// StellaX
// Jeff Miller 05/01/2000
//
#ifndef TXTBTN_H
#define TXTBTN_H
#pragma once

// derived from Roger Onslow's code

#include "Wnd.hxx"

class CTextButton3d : public CWnd
{
public:

	CTextButton3d();

protected:

	LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
		BOOL& rfHandled);

private:

	void OnPaint(HWND, HDC);
	BOOL OnEraseBkgnd(HWND, HDC);

	CTextButton3d( const CTextButton3d& );  // no implementation
	void operator=( const CTextButton3d& );  // no implementation
};

#endif
