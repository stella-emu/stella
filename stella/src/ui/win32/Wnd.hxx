//
// StellaX
// Jeff Miller 05/01/2000
//
#ifndef WND_H
#define WND_H

class CWnd
{
public:

	CWnd();

	BOOL SubclassDlgItem( HWND hwndParent, UINT nID );

    operator HWND() const
        {
            return m_hwnd;
        }

protected:

	virtual void PreSubclassWindow( HWND hwnd );
	virtual LRESULT WndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
		BOOL& rfHandled ) = 0;

private:

	static LRESULT CALLBACK StaticWndProc( HWND hWnd, UINT msg, 
		WPARAM wParam, LPARAM lParam );

	WNDPROC m_pOldWindowProc;
	HWND m_hwnd;

	CWnd( const CWnd& );  // no implementation
	void operator=( const CWnd& );  // no implementation
};

#endif
