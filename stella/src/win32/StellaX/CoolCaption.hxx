//
// StellaX
// Jeff Miller 05/01/2000
//

#ifndef COOLCAP_H
#define COOLCAP_H
#pragma once

class CCoolCaption
{
public:
	CCoolCaption();

	void OnInitDialog(HWND hDlg);
	void OnDestroy();

	void OnNcPaint(HRGN);
	void OnNcActivate(BOOL);
	BOOL OnNCLButtonDown(INT, POINTS);

protected:

	HWND m_hDlg;

private:

	void CalculateNCArea(void);
	void FillSolidRect(HDC hdc, int x, int y, int cx, int cy, COLORREF clr);
	void Draw3dRect(HDC hdc, int x, int y, int cx, int cy, 
		COLORREF clrTopLeft, COLORREF clrBottomRight);

	BOOL m_fIsActive;
	RECT m_rcWindow;
	int m_cxWindow;
	int m_cyWindow;
	int m_cxFrame;
	int m_cyFrame;
	int m_cxButtonSize;
	RECT m_rcClose;
	RECT m_rcMin;
	RECT m_rcCaption;
	RECT m_rcTextArea;
	HFONT m_hfont;
	LPTSTR m_tszCaption;

	CCoolCaption( const CCoolCaption& );  // no implementation
	void operator=( const CCoolCaption& );  // no implementation
};

#endif
