//
// StellaX
// Jeff Miller 05/01/2000
//
#include "pch.hxx"
#include "RoundButton.hxx"
#include <math.h>
#include <limits.h>

// prototypes
COLORREF GetColour(double dAngle, COLORREF crBright, COLORREF crDark);

void DrawCircle( HDC hdc, const POINT& p, LONG lRadius, 
                COLORREF crColour, BOOL bDashed = FALSE);

void DrawCircleLeft( HDC hdc, const POINT& p, LONG lRadius, 
                    COLORREF crBright, COLORREF crDark);

void DrawCircleRight( HDC hdc, const POINT& p, LONG lRadius, 
                     COLORREF crBright, COLORREF crDark);


// Calculate colour for a point at the given angle by performing a linear
// interpolation between the colours crBright and crDark based on the cosine
// of the angle between the light source and the point.
//
// Angles are measured from the +ve x-axis (i.e. (1,0) = 0 degrees, (0,1) = 90 degrees )
// But remember: +y points down!

COLORREF GetColour(
    double dAngle, 
    COLORREF crBright, 
    COLORREF crDark
    )
{
#define Rad2Deg	180.0/3.1415 

// For better light-continuity along the edge of a stretched button: 
//	LIGHT_SOURCE_ANGLE == -1.88
	
//#define LIGHT_SOURCE_ANGLE	-2.356		// -2.356 radians = -135 degrees, i.e. From top left
#define LIGHT_SOURCE_ANGLE	-1.88

	ASSERT(dAngle > -3.1416 && dAngle < 3.1416);
	double dAngleDifference = LIGHT_SOURCE_ANGLE - dAngle;

	if (dAngleDifference < -3.1415) dAngleDifference = 6.293 + dAngleDifference;
	else if (dAngleDifference > 3.1415) dAngleDifference = 6.293 - dAngleDifference;

	double Weight = 0.5*(cos(dAngleDifference)+1.0);

	BYTE Red   = (BYTE) (Weight*GetRValue(crBright) + (1.0-Weight)*GetRValue(crDark));
	BYTE Green = (BYTE) (Weight*GetGValue(crBright) + (1.0-Weight)*GetGValue(crDark));
	BYTE Blue  = (BYTE) (Weight*GetBValue(crBright) + (1.0-Weight)*GetBValue(crDark));

	//TRACE("LightAngle = %0.0f, Angle = %3.0f, Diff = %3.0f, Weight = %0.2f, RGB %3d,%3d,%3d\n", 
	//	  LIGHT_SOURCE_ANGLE*Rad2Deg, dAngle*Rad2Deg, dAngleDifference*Rad2Deg, Weight,Red,Green,Blue);

	return RGB(Red, Green, Blue);
}

void DrawCircle(
    HDC hdc, 
    const POINT& p, 
    LONG lRadius, 
    COLORREF crColour, 
    BOOL bDashed
    )
{
	const int nDashLength = 1;
	LONG lError, lXoffset, lYoffset;
	int  nDash = 0;
	BOOL bDashOn = TRUE;

	//Check to see that the coordinates are valid
	ASSERT( (p.x + lRadius <= LONG_MAX) && (p.y + lRadius <= LONG_MAX) );
	ASSERT( (p.x - lRadius >= LONG_MIN) && (p.y - lRadius >= LONG_MIN) );

	//Set starting values
	lXoffset = lRadius;
	lYoffset = 0;
	lError   = -lRadius;

	do 
    {
		if (bDashOn) 
        {
            ::SetPixelV(hdc, p.x + lXoffset, p.y + lYoffset, crColour);
            ::SetPixelV(hdc, p.x + lXoffset, p.y - lYoffset, crColour);
            ::SetPixelV(hdc, p.x + lYoffset, p.y + lXoffset, crColour);
            ::SetPixelV(hdc, p.x + lYoffset, p.y - lXoffset, crColour);
            ::SetPixelV(hdc, p.x - lYoffset, p.y + lXoffset, crColour);
            ::SetPixelV(hdc, p.x - lYoffset, p.y - lXoffset, crColour);
            ::SetPixelV(hdc, p.x - lXoffset, p.y + lYoffset, crColour);
            ::SetPixelV(hdc, p.x - lXoffset, p.y - lYoffset, crColour);
		}

		//Advance the error term and the constant X axis step
		lError += lYoffset++;

		//Check to see if error term has overflowed
		if ((lError += lYoffset) >= 0)
			lError -= --lXoffset * 2;

		if (bDashed && (++nDash == nDashLength)) 
        {
			nDash = 0;
			bDashOn = !bDashOn;
		}

	} while (lYoffset <= lXoffset);	//Continue until halfway point
} 

// The original Drawcircle function is split up into DrawCircleRight and DrawCircleLeft
// to make stretched buttons

void DrawCircleRight(
    HDC hdc,
    const POINT& p,
    LONG lRadius, 
    COLORREF crBright, 
    COLORREF crDark
    )
{
	LONG lError, lXoffset, lYoffset;

	//Check to see that the coordinates are valid
	ASSERT( (p.x + lRadius <= LONG_MAX) && (p.y + lRadius <= LONG_MAX) );
	ASSERT( (p.x - lRadius >= LONG_MIN) && (p.y - lRadius >= LONG_MIN) );

	//Set starting values
	lXoffset = lRadius;
	lYoffset = 0;
	lError   = -lRadius;

	do 
    {
		const double Pi = 3.141592654, 
					 Pi_on_2 = Pi * 0.5;
		COLORREF crColour;
		long double dAngle = atan2((long double)lYoffset, (long double)lXoffset);

		//Draw the current pixel, reflected across all four arcs

		crColour = GetColour(dAngle, crBright, crDark);
        ::SetPixelV(hdc, p.x + lXoffset, p.y + lYoffset, crColour);

		crColour = GetColour(Pi_on_2 - dAngle, crBright, crDark);
        ::SetPixelV(hdc, p.x + lYoffset, p.y + lXoffset, crColour);

		crColour = GetColour(-Pi_on_2 + dAngle, crBright, crDark);
        ::SetPixelV(hdc, p.x + lYoffset, p.y - lXoffset, crColour);

		crColour = GetColour(-dAngle, crBright, crDark);
        ::SetPixelV(hdc, p.x + lXoffset, p.y - lYoffset, crColour);

		//Advance the error term and the constant X axis step
		lError += lYoffset++;

		//Check to see if error term has overflowed
		if ((lError += lYoffset) >= 0)
			lError -= --lXoffset * 2;

	} while (lYoffset <= lXoffset);	//Continue until halfway point
} 

// The original Drawcircle function is split up into DrawCircleRight and DrawCircleLeft
// to make stretched buttons

void DrawCircleLeft(
    HDC hdc,
    const POINT& p,
    LONG lRadius, 
    COLORREF crBright, 
    COLORREF crDark
    )
{
	LONG lError, lXoffset, lYoffset;

	//Check to see that the coordinates are valid
	ASSERT( (p.x + lRadius <= LONG_MAX) && (p.y + lRadius <= LONG_MAX) );
	ASSERT( (p.x - lRadius >= LONG_MIN) && (p.y - lRadius >= LONG_MIN) );

	//Set starting values
	lXoffset = lRadius;
	lYoffset = 0;
	lError   = -lRadius;

	do 
    {
		const double Pi = 3.141592654, 
					 Pi_on_2 = Pi * 0.5;
		COLORREF crColour;
		long double   dAngle = atan2((long double)lYoffset, (long double)lXoffset);

		//Draw the current pixel, reflected across all eight arcs

		crColour = GetColour(Pi_on_2 + dAngle, crBright, crDark);
        ::SetPixelV(hdc, p.x - lYoffset, p.y + lXoffset, crColour);

		crColour = GetColour(Pi - dAngle, crBright, crDark);
        ::SetPixelV(hdc, p.x - lXoffset, p.y + lYoffset, crColour);

		crColour = GetColour(-Pi + dAngle, crBright, crDark);
        ::SetPixelV(hdc, p.x - lXoffset, p.y - lYoffset, crColour);

		crColour = GetColour(-Pi_on_2 - dAngle, crBright, crDark);
        ::SetPixelV(hdc, p.x - lYoffset, p.y - lXoffset, crColour);

		//Advance the error term and the constant X axis step
		lError += lYoffset++;

		//Check to see if error term has overflowed
		if ((lError += lYoffset) >= 0)
			lError -= --lXoffset * 2;

	} while (lYoffset <= lXoffset);	//Continue until halfway point
} 

/////////////////////////////////////////////////////////////////////////////

static void ClientToScreen(
    HWND hwnd, 
    LPRECT lpRect
    )
{
    ::ClientToScreen(hwnd, (LPPOINT)lpRect);
    ::ClientToScreen(hwnd, ((LPPOINT)lpRect)+1);
}

static void ScreenToClient(
    HWND hwnd,
    LPRECT lpRect
    )
{
    ::ScreenToClient(hwnd, (LPPOINT)lpRect);
    ::ScreenToClient(hwnd, ((LPPOINT)lpRect)+1);
}

static void FillSolidRect(
    HDC hdc,
    LPCRECT lpRect, 
    COLORREF clr
    )
{
    COLORREF crOld = ::SetBkColor(hdc, clr);
    ::ExtTextOut(hdc, 0, 0, ETO_OPAQUE, lpRect, NULL, 0, NULL);
    ::SetBkColor(hdc, crOld);
}

//////////////////////////////////////////////////////////////////////////////

CRoundButton::CRoundButton(
    ) : \
    m_hrgn( NULL ),
    m_fDrawDashedFocusCircle( TRUE ),
    m_fStretch( FALSE )
{
}

CRoundButton::~CRoundButton(
    )
{
    if ( m_hrgn )
    {
        ::DeleteObject( m_hrgn );
    }
}

void CRoundButton::PreSubclassWindow(
	HWND hwnd
    )
{
    RECT rect;
    ::GetClientRect( hwnd, &rect );
    
    m_fStretch = (rect.right-rect.left) > (rect.bottom-rect.top);
    
    if ( ! m_fStretch )
    {
        rect.bottom = rect.right = min ( rect.bottom, rect.right );
    }
    
    m_ptCenter.x = m_ptLeft.x = m_ptRight.x = ((rect.right-rect.left)/2);
    m_ptCenter.y = m_ptLeft.y = m_ptRight.y = ((rect.bottom-rect.top)/2);
    
    m_nRadius = rect.bottom/2 - 1;
    
    m_ptLeft.x = m_nRadius;
    m_ptRight.x = rect.right - m_nRadius - 1;
    
    ::SetWindowRgn( hwnd, NULL, FALSE );
    m_hrgn = ::CreateEllipticRgnIndirect( &rect );
    ::SetWindowRgn( hwnd, m_hrgn, TRUE );
    
    ::ClientToScreen( hwnd, &rect );
    
    HWND hwndParent = ::GetParent( hwnd );
    if ( hwndParent )
    {
        ::ScreenToClient( hwndParent, &rect );
    }
    
    if ( ! m_fStretch )
    {
        ::MoveWindow( hwnd, rect.left, rect.top, 
            rect.right-rect.left, rect.bottom-rect.top, TRUE );
    }
}

LRESULT CRoundButton::WndProc(
	HWND hWnd, 
    UINT msg, 
    WPARAM wParam, 
    LPARAM lParam,
    BOOL& rfHandled
    ) 
{  
    switch (msg)
    {
    case WM_DRAWITEM:
        rfHandled = TRUE;
        OnDrawItem(hWnd, (UINT)wParam, (LPDRAWITEMSTRUCT)lParam);
        return TRUE;

	case WM_ERASEBKGND:
        // don't do erasing
        return TRUE;
    }
    
    return 0;
}

void CRoundButton::OnDrawItem(
	HWND hwnd,
    UINT idCtl, 
    LPDRAWITEMSTRUCT lpdis
    )
{
    UNUSED_ALWAYS( idCtl );

    HDC hdc = lpdis->hDC;
    RECT& rect = lpdis->rcItem;
    UINT state = lpdis->itemState;
    UINT nStyle = GetWindowLong(hwnd, GWL_STYLE);
    int nRadius = m_nRadius;
    
    int nSavedDC = ::SaveDC( hdc );
    
    ::SelectObject( hdc, ::GetStockObject( NULL_BRUSH ) );
    ::FillSolidRect( hdc, &rect, ::GetSysColor(COLOR_BTNFACE) );
    
    // Draw the focus circle around the button for non-stretched buttons
    
    if ( (state & ODS_FOCUS) && m_fDrawDashedFocusCircle && !m_fStretch )
    {
        DrawCircle( hdc, m_ptCenter, nRadius--, RGB( 0, 0, 0) );
    }
    
    // Draw the raised/sunken edges of the button (unless flat)
    
    if ( nStyle & BS_FLAT )
    {
        if ( m_fStretch )
        {
            // for stretched buttons: draw left and right arcs and connect the with lines

            HPEN hpenOld;
            
            RECT rectLeftBound;
            ::SetRect( &rectLeftBound, 0, 0, nRadius*2, nRadius*2 );
            
            RECT rectRightBound;
            ::SetRect( &rectRightBound, m_ptRight.x-nRadius, 0, m_ptRight.x+nRadius, nRadius*2 );
            
            hpenOld = (HPEN)::SelectObject( hdc, 
                ::CreatePen( PS_SOLID, 1, ::GetSysColor(COLOR_3DDKSHADOW) ) );
            
            ::Arc( hdc, 
                rectLeftBound.left, rectLeftBound.top, rectLeftBound.right, rectLeftBound.bottom, 
                m_ptLeft.x, 0,
                m_ptLeft.x, nRadius*2 );
            
            ::Arc( hdc, 
                rectRightBound.left, rectRightBound.top, rectRightBound.right, rectRightBound.bottom,
                m_ptRight.x, nRadius*2,
                m_ptRight.x, 0 );
            
            ::MoveToEx( hdc, m_ptLeft.x, 0, NULL );
            ::LineTo( hdc, m_ptRight.x, 0 );
            
            ::MoveToEx( hdc, m_ptLeft.x, nRadius*2-1, NULL );
            ::LineTo( hdc, m_ptRight.x, nRadius*2-1 );

            nRadius--;

            ::InflateRect( &rectLeftBound, -1, -1 );
            ::InflateRect( &rectRightBound, -1, -1 );

            ::DeleteObject( ::SelectObject( hdc, 
                ::CreatePen( PS_SOLID, 1, ::GetSysColor( COLOR_3DHIGHLIGHT ) ) ) );

            ::Arc( hdc, 
                rectLeftBound.left, rectLeftBound.top, rectLeftBound.right, rectLeftBound.bottom, 
                m_ptLeft.x, 1,
                m_ptLeft.x, nRadius*2 );

            ::Arc( hdc,
                rectRightBound.left, rectRightBound.top, rectRightBound.right, rectRightBound.bottom,
                m_ptRight.x, nRadius*2,
                m_ptRight.x, 0 );

            ::MoveToEx( hdc, m_ptLeft.x, 1, NULL );
            ::LineTo( hdc, m_ptRight.x, 1 );

            ::MoveToEx( hdc, m_ptLeft.x, nRadius*2, NULL );
            ::LineTo( hdc, m_ptRight.x, nRadius*2 );

            ::DeleteObject( ::SelectObject( hdc, hpenOld ) );
        }
        else
        {
            // for non-stretched buttons: draw two circles

            DrawCircle( hdc, m_ptCenter, nRadius--, ::GetSysColor(COLOR_3DDKSHADOW) );
            DrawCircle( hdc, m_ptCenter, nRadius--, ::GetSysColor(COLOR_3DHIGHLIGHT) );
        }
    }
    else
    {
        if (state & ODS_SELECTED)
        {
            // draw the circular segments for stretched AND non-stretched buttons

            DrawCircleLeft( hdc, m_ptLeft, nRadius,
                ::GetSysColor(COLOR_3DDKSHADOW), ::GetSysColor(COLOR_3DHIGHLIGHT) );
            DrawCircleRight( hdc, m_ptRight, nRadius,
                ::GetSysColor(COLOR_3DDKSHADOW), ::GetSysColor(COLOR_3DHIGHLIGHT) );
            DrawCircleLeft( hdc, m_ptLeft, nRadius-1,
                ::GetSysColor(COLOR_3DSHADOW), ::GetSysColor(COLOR_3DLIGHT) );
            DrawCircleRight( hdc, m_ptRight, nRadius-1, 
                ::GetSysColor(COLOR_3DSHADOW), ::GetSysColor(COLOR_3DLIGHT) );
            
            if ( m_fStretch )
            {
                // draw connecting lines for stretched buttons only

                HPEN hpenOld;
                
                hpenOld = (HPEN)::SelectObject( hdc, 
                    (HPEN)::CreatePen( PS_SOLID, 1, ::GetSysColor(COLOR_3DDKSHADOW) ) );
                
                ::MoveToEx( hdc, m_ptLeft.x, 1, NULL );
                ::LineTo( hdc, m_ptRight.x, 1 );
                
                ::DeleteObject( ::SelectObject( hdc,
                    ::CreatePen( PS_SOLID, 1, ::GetSysColor(COLOR_3DSHADOW) ) ) );
                
                ::MoveToEx( hdc, m_ptLeft.x, 2, NULL );
                ::LineTo( hdc, m_ptRight.x, 2 );
                
                ::DeleteObject( ::SelectObject( hdc,
                    ::CreatePen( PS_SOLID, 1, ::GetSysColor( COLOR_3DLIGHT ) ) ) );
                
                ::MoveToEx( hdc, m_ptLeft.x, m_ptLeft.y + nRadius-1, NULL );
                ::LineTo( hdc, m_ptRight.x, m_ptLeft.y + nRadius-1 );
                
                ::DeleteObject( ::SelectObject( hdc,
                    ::CreatePen( PS_SOLID, 1, ::GetSysColor( COLOR_3DHIGHLIGHT ) ) ) );
                
                ::MoveToEx( hdc, m_ptLeft.x, m_ptLeft.y + nRadius, NULL );
                ::LineTo( hdc, m_ptRight.x, m_ptLeft.y + nRadius );
                
                ::DeleteObject( ::SelectObject( hdc, hpenOld ) );
            }
        }
        else
        {
            // draw the circular segments for stretched AND non-stretched buttons
            
            DrawCircleLeft( hdc, m_ptLeft, nRadius,
                ::GetSysColor(COLOR_3DHIGHLIGHT), ::GetSysColor(COLOR_3DDKSHADOW) );
            
            DrawCircleRight( hdc, m_ptRight, nRadius,
                ::GetSysColor(COLOR_3DHIGHLIGHT), ::GetSysColor(COLOR_3DDKSHADOW) );
            
            DrawCircleLeft( hdc, m_ptLeft, nRadius - 1,
                ::GetSysColor(COLOR_3DLIGHT), ::GetSysColor(COLOR_3DSHADOW) );
            
            DrawCircleRight( hdc, m_ptRight, nRadius - 1,
                ::GetSysColor(COLOR_3DLIGHT), ::GetSysColor(COLOR_3DSHADOW) );
            
            // draw connecting lines for stretch buttons
            
            if ( m_fStretch )
            {
                HPEN hpenOld;
                
                hpenOld = (HPEN)::SelectObject( hdc,
                    ::CreatePen( PS_SOLID, 1, 
                    ::GetPixel( hdc, m_ptLeft.x, 1 ) ) );

                ::MoveToEx( hdc, m_ptLeft.x, 1, NULL );
                ::LineTo( hdc, m_ptRight.x , 1);

                ::DeleteObject( ::SelectObject( hdc,
                    ::CreatePen( PS_SOLID, 1, 
                    ::GetPixel( hdc, m_ptLeft.x, 2 ) ) ) );

                ::MoveToEx( hdc, m_ptLeft.x, 2, NULL );
                ::LineTo( hdc, m_ptRight.x, 2 );

                ::DeleteObject( ::SelectObject( hdc,
                    ::CreatePen( PS_SOLID, 1, 
                    ::GetPixel( hdc, m_ptLeft.x, m_ptLeft.y+nRadius ) ) ) );

                ::MoveToEx( hdc, m_ptLeft.x, m_ptLeft.y + nRadius, NULL );
                ::LineTo( hdc, m_ptRight.x, m_ptLeft.y + nRadius );

                ::DeleteObject( ::SelectObject( hdc,
                    ::CreatePen( PS_SOLID, 1,
                    ::GetPixel( hdc, m_ptLeft.x, m_ptLeft.y+nRadius-1 ) ) ) );

                ::MoveToEx( hdc, m_ptLeft.x, m_ptLeft.y + nRadius - 1, NULL );
                ::LineTo( hdc, m_ptRight.x, m_ptLeft.y + nRadius - 1 );

                ::DeleteObject( ::SelectObject( hdc, hpenOld ) );
            }
        }
    }
    
    // Draw the text if there is any
    
    TCHAR pszText[ 256 ];
    int cch = ::GetWindowText( hwnd, pszText, 255 );
    
    if ( cch != 0 )
    {
        HRGN hrgn;
        
        if ( m_fStretch )
        {
            hrgn = ::CreateRectRgn( m_ptLeft.x - nRadius / 2, m_ptCenter.y - nRadius, 
                m_ptRight.x + nRadius / 2, m_ptCenter.y + nRadius );
        }
        else
        {
            hrgn = CreateEllipticRgn( m_ptCenter.x - nRadius, m_ptCenter.y - nRadius, 
                m_ptCenter.x + nRadius, m_ptCenter.y + nRadius );
        }
        
        ::SelectClipRgn( hdc, hrgn );
        
        SIZE size;
        ::GetTextExtentPoint32( hdc, pszText, cch, &size );
        
        POINT pt = { m_ptCenter.x - size.cx / 2, m_ptCenter.y - size.cy / 2 - 1 };
        POINT pt2 = { m_ptCenter.x + size.cx/2, m_ptCenter.y + size.cy/2 + 1 };
        
        if ( state & ODS_SELECTED )
        {
            ++( pt.x );
            ++( pt.y );
        }
        
        ::SetBkMode( hdc, TRANSPARENT );
        
#if 0
        if ( state & ODS_DISABLED )
        {
            ::DrawState( hdc, NULL, NULL, (LPARAM)pszText, (WPARAM)cch,
                pt.x, pt.y, size.cx, size.cy, DSS_DISABLED );
        }
        else
#endif
        {
            // give text a 3d-look
            
            RECT pos;
            ::SetRect( &pos, pt.x, pt.y, pt2.x, pt2.y );

            COLORREF crOld = ::SetTextColor( hdc, 
                ( state & ODS_DISABLED ) ?
                    ::GetSysColor( COLOR_GRAYTEXT ) :
                    ::GetSysColor( COLOR_WINDOWTEXT ) );
            
            if ( state & ODS_FOCUS )
            {
                LOGFONT lf;
                ::GetObject( (HFONT)::SendMessage( hwnd, WM_GETFONT, 0, 0 ),
                    sizeof(LOGFONT), &lf );

                lf.lfWeight = FW_BOLD;

                HFONT hfontOld = (HFONT)::SelectObject( hdc, ::CreateFontIndirect( &lf ) );

                ::DrawText( hdc, pszText, -1, &pos, DT_SINGLELINE | DT_CENTER );

                ::DeleteObject( ::SelectObject( hdc, hfontOld) );

            }
            else
            {
                ::DrawText( hdc, pszText, -1, &pos, DT_SINGLELINE | DT_CENTER );
            }

            ::SetTextColor( hdc, crOld );
        }
        
        ::SelectClipRgn( hdc, NULL );
        
        DeleteObject( hrgn );
    }
    
#if 0
    // draw the focus circle on the inside
    
    if ( (state & ODS_FOCUS) && m_fDrawDashedFocusCircle && !m_fStretch )
    {
        DrawCircle( hdc, m_ptCenter, nRadius-2, RGB(0, 0, 0), TRUE );
    }
#endif

    ::RestoreDC( hdc, nSavedDC );
}
