//
// StellaX
// Jeff Miller 05/01/2000
//
#ifndef HDRCTL_H
#define HDRCTL_H
#pragma once

#include "Wnd.hxx"

class CHeaderCtrl : public CWnd
{
public:

	CHeaderCtrl();

	void SetSortCol(int nCol, BOOL fAscending);
	int GetSortCol(void) const;
	BOOL GetSortAsc(void) const;

protected:

	LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
		BOOL& rfHandled);

private:

	void OnDrawItem(HWND hwnd, UINT idCtl, LPDRAWITEMSTRUCT lpdis);

	UINT m_nSortCol;
	BOOL m_fSortAsc;

	CHeaderCtrl( const CHeaderCtrl& );  // no implementation
	void operator=( const CHeaderCtrl& );  // no implementation

};

inline int CHeaderCtrl::GetSortCol
(
	void
) const
{
	return m_nSortCol;
}

inline BOOL CHeaderCtrl::GetSortAsc
(
	void
) const
{
	return m_fSortAsc;
}

#endif
