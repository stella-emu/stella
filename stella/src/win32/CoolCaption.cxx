//
// StellaX
// Jeff Miller 05/01/2000
//

#include "pch.hxx"
#include "CoolCaption.hxx"

// MAKE SURE STYLE IS JUST "TITLE BAR" !

CCoolCaption::CCoolCaption(
    ) : \
	m_hfont( NULL ),
	m_tszCaption( NULL ),
	m_fIsActive( FALSE ),
	m_hDlg( NULL )
{
}

void CCoolCaption::OnInitDialog(
	HWND hDlg
    )
{
	m_hDlg = hDlg;

	CalculateNCArea();
}

void CCoolCaption::OnDestroy(
	void
    )
{
	if ( m_hDlg == NULL )
    {
		return;
    }

	if ( m_hfont )
    {
        ::DeleteObject( m_hfont );
    }

	delete[] m_tszCaption;
    m_tszCaption = NULL;
}

void CCoolCaption::CalculateNCArea(
    )
{
	if ( m_hDlg == NULL )
    {
		return;
    }

    ::GetWindowRect(m_hDlg, &m_rcWindow);
	m_cxWindow = (m_rcWindow.right - m_rcWindow.left);
	m_cyWindow = (m_rcWindow.bottom - m_rcWindow.top);

    m_cxFrame = ::GetSystemMetrics(SM_CXFRAME);
    m_cyFrame = ::GetSystemMetrics(SM_CYFRAME);

    m_cxButtonSize = ::GetSystemMetrics(SM_CXSIZE);

    const int cyCaption = ::GetSystemMetrics(SM_CYCAPTION);

	// This is the size of the caption

    ::SetRect(&m_rcCaption, 0, 0, m_cxWindow, cyCaption + m_cyFrame);

	// calculate position of buttons

	SetRect(&m_rcClose,
		m_cxWindow - m_cxFrame - m_cxButtonSize + 1, 
		m_rcCaption.top + m_cyFrame + 1, 
		m_cxWindow - m_cxFrame - 1, 
		m_rcCaption.bottom - m_cyFrame 
	);

	CopyRect(&m_rcMin, &m_rcClose);
	OffsetRect(&m_rcMin, -(m_cxButtonSize-2), 0);

	// figure out union of all buttons

	RECT rcButtons;
	UnionRect(&rcButtons, &m_rcClose, &m_rcMin);

	SetRect(&m_rcTextArea, m_cxFrame, m_rcCaption.top+m_cyFrame, 
		rcButtons.left-m_cxFrame, m_rcCaption.bottom-m_cyFrame);

	// Make bold caption font

	m_hfont = (HFONT)SendMessage(m_hDlg, WM_GETFONT, 0, 0);
	LOGFONT lf;
	GetObject(m_hfont, sizeof(lf), &lf);
	lf.lfWeight = FW_BOLD;
	m_hfont = CreateFontIndirect(&lf);

	// Get caption text

	int nCaptionLength = GetWindowTextLength(m_hDlg);
	m_tszCaption = new TCHAR[nCaptionLength + 2];
	GetWindowText(m_hDlg, m_tszCaption, nCaptionLength+1);
}


void CCoolCaption::OnNcPaint(
	HRGN hrgn
    )
{
    UNUSED_ALWAYS( hrgn );

	if ( m_hDlg == NULL )
    {
		return;
    }

	HDC hdc = GetWindowDC(m_hDlg);

	// Fill in the NC area with the proper color

	HPEN hpen, hpenOld;

	hpen = CreatePen(PS_SOLID, 1, m_fIsActive ? 
		GetSysColor(COLOR_ACTIVECAPTION) : \
		GetSysColor(COLOR_INACTIVECAPTION));
	hpenOld = (HPEN)SelectObject(hdc, hpen);

	for (long x = 0; x < m_cxWindow; ++x) 
	{
		if (x < m_cxFrame-1 || x > m_cxWindow-m_cxFrame)
		{
			// left, right must go from top to bottom

			MoveToEx(hdc, x, 0, NULL);
			LineTo(hdc, x, m_rcWindow.bottom);
		}
		else
		{
			// otherwise just do top and bottom

			MoveToEx(hdc, x, 0, NULL);
			LineTo(hdc, x, m_rcCaption.bottom);

			MoveToEx(hdc, x, m_cyWindow-m_cyFrame, NULL); 
			LineTo(hdc, x, m_cyWindow); 
		}
	}

	SelectObject(hdc, hpenOld);
	DeleteObject(hpen);

	// Draw the close button

	DrawFrameControl(hdc, &m_rcClose, DFC_CAPTION, DFCS_CAPTIONCLOSE);

	// draw the minimize button

	DrawFrameControl(hdc, &m_rcMin, DFC_CAPTION, DFCS_CAPTIONMIN);

	// Draw the caption text

	SetTextColor(hdc, m_fIsActive ? GetSysColor(COLOR_CAPTIONTEXT) : \
		GetSysColor(COLOR_INACTIVECAPTIONTEXT));
	SetBkMode(hdc, TRANSPARENT);

	HFONT hfontOld = (HFONT)SelectObject(hdc, m_hfont);

	// calculate width / height of text

	SIZE sizeText;
	GetTextExtentPoint(hdc, m_tszCaption, lstrlen(m_tszCaption), &sizeText);

	DrawText(hdc, m_tszCaption, -1, &m_rcTextArea,
		DT_END_ELLIPSIS | DT_EXTERNALLEADING |
		DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER);

	SelectObject(hdc, hfontOld);

	// Draw the docking grippers 

	const int nTextRight = m_rcTextArea.left + sizeText.cx + m_cxFrame;

	// right top

	COLORREF cr3dHilight = GetSysColor(COLOR_3DHILIGHT);
	COLORREF cr3dShadow = GetSysColor(COLOR_3DSHADOW);

	Draw3dRect(hdc, nTextRight+m_cxFrame, 8,
		m_rcTextArea.right-(nTextRight+m_cxFrame), 3,
		cr3dHilight, cr3dShadow);

	// right bottom

	Draw3dRect(hdc, nTextRight+m_cxFrame, 12, 
		m_rcTextArea.right-(nTextRight+m_cxFrame), 3,
		cr3dHilight, cr3dShadow);

	ReleaseDC(m_hDlg, hdc);
}

void CCoolCaption::OnNcActivate(
	BOOL fActive
    )
{
	if (m_hDlg == NULL)
		return;

	m_fIsActive = fActive;

	// This fixes a problem where alt-tab wont repaint the NC area

	RedrawWindow(m_hDlg, NULL, NULL, 
		RDW_INVALIDATE | RDW_FRAME | RDW_UPDATENOW | RDW_ERASE);
}

BOOL CCoolCaption::OnNCLButtonDown(
	INT nHitTest,
	POINTS pts
    )
{
    UNUSED_ALWAYS( nHitTest );

	if ( m_hDlg == NULL )
    {
		return FALSE;
    }

	// Get the point in client units

	RECT rcWindow;
	GetWindowRect(m_hDlg, &rcWindow);

	POINT pt = { pts.x, pts.y };
	pt.x -= rcWindow.left;
	pt.y -= rcWindow.top;

	if (PtInRect(&m_rcClose, pt))
	{
		// This isn't closing it!
		// SendMessage(m_hDlg, WM_CLOSE, 0, 0);
		EndDialog(m_hDlg, IDCANCEL);
		return TRUE;
	}
	
	if (PtInRect(&m_rcMin, pt))
	{
		ShowWindow(m_hDlg, SW_MINIMIZE);
		return TRUE;
	}

	return FALSE;
}

void CCoolCaption::FillSolidRect(
	HDC hdc,
	int x, 
	int y, 
	int cx, 
	int cy, 
	COLORREF clr
    )
{
	COLORREF cr = SetBkColor(hdc, clr);
	RECT rect = { x, y, x+cx, y+cy };
	ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);
	SetBkColor(hdc, cr);
}

void CCoolCaption::Draw3dRect(
	HDC hdc,
	int x, 
	int y, 
	int cx, 
	int cy,
	COLORREF clrTopLeft, 
	COLORREF clrBottomRight
    )
{
	FillSolidRect(hdc, x, y, cx - 1, 1, clrTopLeft);
	FillSolidRect(hdc, x, y, 1, cy - 1, clrTopLeft);
	FillSolidRect(hdc, x + cx, y, -1, cy, clrBottomRight);
	FillSolidRect(hdc, x, y + cy, cx, -1, clrBottomRight);
}

