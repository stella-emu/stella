//
// StellaX
// Jeff Miller 05/01/2000
//
#include "pch.hxx"
#include "Wnd.hxx"

// REVIEW: Need to reset old wnd proc?

CWnd::CWnd(
    ) : \
	m_pOldWindowProc(NULL),
	m_hwnd(NULL)
{
}

BOOL CWnd::SubclassDlgItem(
	HWND hwndParent,
	UINT nID
    )
{
	if ( nID == 0 || hwndParent == NULL )
    {
		return FALSE;
    }

	// can't subclass twice!

	if ( m_pOldWindowProc != NULL )
    {
		return FALSE;
    }

	m_hwnd = GetDlgItem( hwndParent, nID );
	if (m_hwnd == NULL)
	{
		ASSERT( FALSE );
		return FALSE;
	}

	PreSubclassWindow( m_hwnd );

	m_pOldWindowProc = (WNDPROC)GetWindowLong( m_hwnd, GWL_WNDPROC );
	SetWindowLong( m_hwnd, GWL_USERDATA, (LONG)this ); 
	SetWindowLong( m_hwnd, GWL_WNDPROC, (LONG)StaticWndProc );

	return TRUE;
}

void CWnd::PreSubclassWindow(
    HWND hwnd
    )
{
	// no default behavior
}

LRESULT CALLBACK CWnd::StaticWndProc(
	HWND hWnd, 
	UINT msg, 
	WPARAM wParam, 
	LPARAM lParam
    ) 
{  
	CWnd* pThis = (CWnd*)GetWindowLong(hWnd, GWL_USERDATA);

	BOOL fHandled = FALSE;
	LRESULT lRes = pThis->WndProc( hWnd, msg, wParam, lParam, fHandled );

	if (fHandled)
	{
		return lRes;
	}

	return CallWindowProc( pThis->m_pOldWindowProc, hWnd, msg, wParam, lParam );
}
