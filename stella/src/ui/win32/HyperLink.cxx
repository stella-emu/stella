//
// StellaX
// Jeff Miller 05/01/2000
//
#include "pch.hxx"
#include "HyperLink.hxx"

#include <shellapi.h>

CHyperLink::CHyperLink(
    ) : \
	m_bOverControl(FALSE),
	m_bVisited(FALSE),
	m_hFont(NULL),
	m_hLinkCursor(NULL)
{
    m_crLinkColor = RGB(  0,   0, 238);   // Blue
    m_crVisitedColor = RGB( 85,  26, 139);   // Purple
    m_crHoverColor = ::GetSysColor(COLOR_HIGHLIGHT);

	SetDefaultCursor();
}

CHyperLink::~CHyperLink(
    )
{
	if (m_hFont)
	{
        ::DeleteObject( m_hFont );
		m_hFont = NULL;
	}
}

void CHyperLink::SetURL(
	LPCTSTR ctszURL
    )
{
	if (ctszURL == NULL)
		return;

	lstrcpy(m_tszURL, ctszURL);

}

void CHyperLink::PreSubclassWindow(
	HWND hwnd
    )
{
	::GetWindowText( hwnd, m_tszText, MAX_HYPERLINK_TEXT_LEN );

	m_hFont = (HFONT)::SendMessage(hwnd, WM_GETFONT, 0, 0);
}

LRESULT CHyperLink::WndProc(
	HWND hwnd,
	UINT msg, 
	WPARAM wParam, 
	LPARAM lParam,
	BOOL& rfHandled
    ) 
{
	switch (msg)
	{
	case WM_PAINT:
		rfHandled = TRUE;
		return OnPaint(hwnd);

	case WM_SETCURSOR:
		rfHandled = TRUE;
		return OnSetCursor(hwnd, LOWORD(lParam), HIWORD(lParam));

	case WM_LBUTTONUP:
		rfHandled = TRUE;
		return OnLButtonUp(hwnd, wParam, LOWORD(lParam), 
			HIWORD(lParam));

	case WM_MOUSEMOVE:
		rfHandled = TRUE;
		return OnMouseMove(hwnd, wParam, LOWORD(lParam), 
			HIWORD(lParam));
	}

	return 0;
}

LRESULT CHyperLink::OnPaint(
	HWND hwnd
    )
{
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hwnd, &ps);
	
	SelectObject(hdc, m_hFont);
	
	if (m_bOverControl)
    {
		SetTextColor(hdc, m_crHoverColor);
    }
	else if (m_bVisited)
    {
		SetTextColor(hdc, m_crVisitedColor);
    }
	else
    {
		SetTextColor(hdc, m_crLinkColor);
    }

    ::SetBkMode( hdc, TRANSPARENT );
	
    ::TextOut( hdc, 0, 0, m_tszText, lstrlen(m_tszText) );
	
	EndPaint(hwnd, &ps);
	return 0;
}

BOOL CHyperLink::OnSetCursor(
	HWND hwnd,
	WORD nHittest,
	WORD wMouseMsg
    )
{
    UNUSED_ALWAYS( hwnd );
    UNUSED_ALWAYS( nHittest );
    UNUSED_ALWAYS( wMouseMsg );

    if (m_hLinkCursor)
    {
        ::SetCursor(m_hLinkCursor);
        return TRUE;
    }

    return FALSE;
}

LRESULT CHyperLink::OnLButtonUp(
	HWND hwnd,
	WPARAM fwKeys,
	WORD xPos,
	WORD yPos
    )
{
    UNUSED_ALWAYS( fwKeys );
    UNUSED_ALWAYS( xPos );
    UNUSED_ALWAYS( yPos );

    int result = (int)GotoURL(m_tszURL, SW_SHOW);
    m_bVisited = (result > HINSTANCE_ERROR);
    if (!m_bVisited) 
	{
        MessageBeep(MB_ICONEXCLAMATION);
        ReportError(result);
    } 
	else 
	{
		m_bVisited = TRUE; 
		InvalidateRect(hwnd, NULL, FALSE); 
	}

	return 0;
}

LRESULT CHyperLink::OnMouseMove(
	HWND hwnd,
	WPARAM fwKeys,
	WORD xPos,
	WORD yPos
    )
{
    UNUSED_ALWAYS( fwKeys );

	// call defwindowproc?

	if (m_bOverControl)
	{
		RECT rc;
		GetClientRect(hwnd, &rc);

		POINT pt = { xPos, yPos };
		if (!PtInRect(&rc, pt))
		{
			m_bOverControl = FALSE;
			ReleaseCapture();
			RedrawWindow(hwnd, NULL, NULL, 
				RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
		}
	}
	else
	{
		m_bOverControl = TRUE;
		RedrawWindow(hwnd, NULL, NULL, 
			RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
		SetCapture(hwnd);
	}

	return 0;
}

// The following appeared in Paul DiLascia's Jan 1998 MSJ articles.
// It loads a "hand" cursor from the winhlp32.exe module

void CHyperLink::SetDefaultCursor(
    void
    )
{
    if (m_hLinkCursor == NULL)                // No cursor handle - load our own
    {
        // Get the windows directory

        TCHAR tszWndDir[MAX_PATH + 1];
        GetWindowsDirectory(tszWndDir, MAX_PATH);

		lstrcat(tszWndDir, _T("\\winhlp32.exe"));

        // This retrieves cursor #106 from winhlp32.exe, which is a hand pointer
        HMODULE hModule = LoadLibrary(tszWndDir);
        if (hModule) {
            HCURSOR hHandCursor = ::LoadCursor(hModule, MAKEINTRESOURCE(106));
            if (hHandCursor)
                m_hLinkCursor = CopyCursor(hHandCursor);
        }
        FreeLibrary(hModule);
    }
}

LONG CHyperLink::GetRegKey(
    HKEY key, 
    LPCTSTR subkey, 
    LPTSTR retdata
    )
{
    HKEY hkey;
    LONG retval = RegOpenKeyEx(key, subkey, 0, KEY_QUERY_VALUE, &hkey);

    if (retval == ERROR_SUCCESS) {
        long datasize = MAX_PATH;
        TCHAR data[MAX_PATH];
        RegQueryValue(hkey, NULL, data, &datasize);
        lstrcpy(retdata,data);
        RegCloseKey(hkey);
    }

    return retval;
}

void CHyperLink::ReportError(
    int nError
    )
{
	TCHAR tsz[MAX_HYPERLINK_TEXT_LEN + 1];
    switch (nError) 
	{
        case 0:
			lstrcpy(tsz, _T("The operating system is out\nof memory or resources."));
			break;

        case SE_ERR_PNF:
			lstrcpy(tsz, _T("The specified path was not found."));
			break;

        case SE_ERR_FNF:
			lstrcpy(tsz, _T("The specified file was not found."));
			break;

        case ERROR_BAD_FORMAT:
			lstrcpy(tsz, _T("The .EXE file is invalid\n(non-Win32 .EXE or error in .EXE image)."));
			break;

        case SE_ERR_ACCESSDENIED:
			lstrcpy(tsz, _T("The operating system denied\naccess to the specified file."));
			break;

        case SE_ERR_ASSOCINCOMPLETE:
			lstrcpy(tsz,_T("The filename association is\nincomplete or invalid."));
			break;

        case SE_ERR_DDEBUSY:
			lstrcpy(tsz, _T("The DDE transaction could not\nbe completed because other DDE transactions\nwere being processed."));
			break;

        case SE_ERR_DDEFAIL:
			lstrcpy(tsz, _T("The DDE transaction failed."));
			break;

        case SE_ERR_DDETIMEOUT:
			lstrcpy(tsz, _T("The DDE transaction could not\nbe completed because the request timed out."));
			break;

        case SE_ERR_DLLNOTFOUND:
			lstrcpy(tsz, _T("The specified dynamic-link library was not found."));
			break;

        case SE_ERR_NOASSOC:
			lstrcpy(tsz, __T("There is no application associated\nwith the given filename extension."));
			break;

        case SE_ERR_OOM:
			lstrcpy(tsz, _T("There was not enough memory to complete the operation."));
			break;

        case SE_ERR_SHARE:
            lstrcpy(tsz, _T("A sharing violation occurred."));
			break;

        default:
			wsprintf(tsz, _T("Unknown error %d occurred."), nError); 
			break;
    }

	TCHAR tszCaption[MAX_HYPERLINK_TEXT_LEN + 1];
	lstrcpy(tszCaption, _T("Unable to open hyperlink"));

    MessageBox(NULL, tsz, tszCaption, MB_ICONEXCLAMATION | MB_OK);
}

HINSTANCE CHyperLink::GotoURL(
    LPCTSTR url, 
    int showcmd
    )
{
    TCHAR key[MAX_PATH + MAX_PATH];

    // First try ShellExecute()
    HINSTANCE result = ShellExecute(NULL, _T("open"), url, NULL,NULL, showcmd);

    // If it failed, get the .htm regkey and lookup the program
    if ((UINT)result <= HINSTANCE_ERROR) {

        if (GetRegKey(HKEY_CLASSES_ROOT, _T(".htm"), key) == ERROR_SUCCESS) {
            lstrcat(key, _T("\\shell\\open\\command"));

            if (GetRegKey(HKEY_CLASSES_ROOT,key,key) == ERROR_SUCCESS) {
                TCHAR *pos;
                pos = _tcsstr(key, _T("\"%1\""));
                if (pos == NULL) {                     // No quotes found
                    pos = strstr(key, _T("%1"));       // Check for %1, without quotes 
                    if (pos == NULL)                   // No parameter at all...
                        pos = key+lstrlen(key)-1;
                    else
                        *pos = '\0';                   // Remove the parameter
                }
                else
                    *pos = '\0';                       // Remove the parameter

                lstrcat(pos, _T(" "));
                lstrcat(pos, url);
                result = (HINSTANCE) WinExec(key,showcmd);
            }
        }
    }

    return result;
}
