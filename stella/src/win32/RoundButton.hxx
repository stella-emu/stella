//
// StellaX
// Jeff Miller 05/01/2000
//
#ifndef RNDBUT_H
#define RNDBUT_H

#include "Wnd.hxx"

class CRoundButton : public CWnd
{
public:

    CRoundButton();
    ~CRoundButton();

protected:

    LRESULT WndProc( HWND hwnd, UINT uMsg, 
        WPARAM wParam, LPARAM lParam, BOOL& fHandled );
    virtual void PreSubclassWindow(HWND hwnd);

private:

    void OnDrawItem(HWND hwnd, UINT idCtl, LPDRAWITEMSTRUCT lpdis);

    HRGN m_hrgn;
    POINT m_ptCenter;
    POINT m_ptLeft;
    POINT m_ptRight;
    int m_nRadius;
    BOOL m_fDrawDashedFocusCircle;
    BOOL m_fStretch;

	CRoundButton( const CRoundButton& );  // no implementation
	void operator=( const CRoundButton& );  // no implementation
};

#endif
