//
// StellaX
// Jeff Miller 05/10/2000
//

#include "pch.hxx"
#include "DirectDraw.hxx"
#include "resource.h"

CDirectDraw::CDirectDraw(
	const SIZE& sizeSurface
    ) :\
    m_fInitialized( FALSE ),
	m_piDD(NULL),
	m_piDDSurface(NULL),
	m_piDDSurfaceBack(NULL),
	m_piDDPalette(NULL)
{
	m_sizeScreen.cx = 0;
	m_sizeScreen.cy = 0;

	m_sizeSurface.cx = sizeSurface.cx;
	m_sizeSurface.cy = sizeSurface.cy;

    // TRACE( "m_sizeSurface = %d x %d", m_sizeSurface.cx, m_sizeSurface.cy );
}

CDirectDraw::~CDirectDraw(
    )
{
	TRACE("CDirectDraw::~CDirectDraw");

	Cleanup();
}

HRESULT CDirectDraw::Initialize(
	HWND hwnd,
    const ULONG* pulPalette,
    int cx /* = 0 */,
    int cy /* = 0 */
    )
{
	HRESULT hr = S_OK;
    UINT uMsg = 0; // Message to show if FAILED(hr)
    
    m_hwnd = hwnd;

    hr = ::CoCreateInstance( CLSID_DirectDraw, 
                             NULL, 
		                     CLSCTX_SERVER, 
                             IID_IDirectDraw, 
                             (void**)&m_piDD );
	if ( FAILED(hr) )
	{
		TRACE( "CCI on DirectDraw failed, hr=%x", hr );
        uMsg = IDS_NODIRECTDRAW;
        goto cleanup;
	}

    //
	// Initialize it
	// This method takes the driver GUID parameter that the DirectDrawCreate 
	// function typically uses (NULL is active display driver)
    //

	hr = m_piDD->Initialize( NULL );
	if ( FAILED(hr) )
	{
        TRACE( "DDraw::Initialize failed, hr=%x", hr );
        uMsg = IDS_DD_INIT_FAILED;
        goto cleanup;
	}


    //
	// Get the best video mode for game width
    //

    m_sizeScreen.cx = cx;
    m_sizeScreen.cy = cy;

    if ( cx == 0 || cy == 0 )
    {
    	hr = m_piDD->EnumDisplayModes( 0, NULL, this, EnumModesCallback );
        if ( FAILED(hr) )
        {
            TRACE( "EnumDisplayModes failed" );
            uMsg = IDS_DD_ENUMMODES_FAILED;
            goto cleanup;
        }
    }

	if (m_sizeScreen.cx == 0 || m_sizeScreen.cy == 0)
	{
		TRACE("No good video mode found");
        uMsg = IDS_NO_VID_MODE;
        hr = E_INVALIDARG;
        goto cleanup;
	}

	TRACE("Video Mode Selected: %d x %d", m_sizeScreen.cx, m_sizeScreen.cy);

	// compute blit offset to center image

#ifdef DOUBLE_WIDTH
	m_ptBlitOffset.x = ((m_sizeScreen.cx - m_sizeSurface.cx*2) / 2);
#else
	m_ptBlitOffset.x = ((m_sizeScreen.cx - m_sizeSurface.cx) / 2);
#endif

	m_ptBlitOffset.y = ((m_sizeScreen.cy - m_sizeSurface.cy) / 2);

	TRACE("Game dimensions: %dx%d (blit offset = %d, %d)", 
		m_sizeSurface.cx, m_sizeSurface.cy, m_ptBlitOffset.x, m_ptBlitOffset.y);
	
	// Set cooperative level

	hr = m_piDD->SetCooperativeLevel( hwnd, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN );
	if ( FAILED(hr) )
	{
		TRACE( "IDD:SetCooperativeLevel failed, hr=%x", hr );
        uMsg = IDS_DDSCL_FAILED;
        goto cleanup;
	}

	hr = m_piDD->SetDisplayMode( m_sizeScreen.cx, m_sizeScreen.cy, 8 );
	if ( FAILED(hr) )
	{
		TRACE( "IDD:SetDisplayMode failed, hr=%x", hr );
        uMsg = IDS_DDSDM_FAILED;
        goto cleanup;
	}

    hr = CreateSurfacesAndPalette();
    if ( FAILED(hr) )
    {
        TRACE( "CreateSurfacesAndPalette failed, hr=%X", hr );
        uMsg = IDS_DDCS_FAILED;
        goto cleanup;
    }

    hr = InitPalette( pulPalette );
    if ( FAILED(hr) )
    {
        TRACE( "InitPalette failed, hr=%X", hr );
        uMsg = IDS_DDCP_FAILED;
        goto cleanup;
    }

    m_fInitialized = TRUE;

cleanup:

    if ( FAILED(hr) )
    {
        Cleanup();

        if ( uMsg != 0 )
        {
            MessageBox( (HINSTANCE)::GetWindowLong( hwnd, GWL_HINSTANCE ), 
                hwnd, uMsg );
        }
    }

    return hr;
}

void CDirectDraw::Cleanup(
	void
    )
{
	// release all of the objects

	if (m_piDDSurfaceBack)
	{
		m_piDDSurfaceBack->Release();
		m_piDDSurfaceBack = NULL;
	}

	if (m_piDDSurface)
	{
		m_piDDSurface->Release();
		m_piDDSurface = NULL;
	}

	if (m_piDDPalette)
	{
		m_piDDPalette->Release();
		m_piDDPalette = NULL;
	}

	if (m_piDD)
	{
		m_piDD->Release();
		m_piDD = NULL;
	}

    m_fInitialized = FALSE;
}


HRESULT WINAPI CDirectDraw::EnumModesCallback(
	LPDDSURFACEDESC lpDDSurfaceDesc,
    LPVOID lpContext
    )
{
	CDirectDraw* pThis = (CDirectDraw*)lpContext;

    DWORD dwWidthReq = pThis->m_sizeSurface.cx;
#ifdef DOUBLE_WIDTH
    dwWidthReq *= 2;
#endif
    DWORD dwHeightReq = pThis->m_sizeSurface.cy;

	DWORD dwWidth = lpDDSurfaceDesc->dwWidth;
	DWORD dwHeight = lpDDSurfaceDesc->dwHeight;
	DWORD dwRGBBitCount = lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount;

    // must be 8 bit mode

	if (dwRGBBitCount != 8)
    {
        return DDENUMRET_OK;
    }

	// TRACE( "EnumModesCallback found: %ld x %ld x %ld", dwWidth, dwHeight, dwRGBBitCount );

    // must be larger then required screen size

    if ( dwWidth < dwWidthReq || dwHeight < dwHeightReq )
    {
        return DDENUMRET_OK;
    }

    if ( pThis->m_sizeScreen.cx != 0 && pThis->m_sizeScreen.cy != 0 )
    {
        // check to see if this is better than the previous choice

        if ( (dwWidth - dwWidthReq) > (pThis->m_sizeScreen.cx - dwWidthReq) )
        {
            return DDENUMRET_OK;
        }

        if ( (dwHeight - dwHeightReq) > (pThis->m_sizeScreen.cy - dwHeightReq) )
        {
            return DDENUMRET_OK;
        }
    }


    // use it!

    pThis->m_sizeScreen.cx = dwWidth;
    pThis->m_sizeScreen.cy = dwHeight;
    // TRACE( "\tEnumModesCallback likes this mode!" );

	return DDENUMRET_OK;
}


HRESULT CDirectDraw::CreateSurfacesAndPalette(
	void
    )
{
	TRACE("CDirectDraw::CreateSurfacesAndPalette");

	HRESULT hr;
	DDSURFACEDESC ddsd;
	HDC hdc;

	// Create the primary surface

	ZeroMemory(&ddsd, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

	hr = m_piDD->CreateSurface(&ddsd, &m_piDDSurface, NULL);
	if (FAILED(hr))
	{
        TRACE( "CreateSurface failed, hr=%X", hr );
		return hr;
	}

	hr = m_piDDSurface->GetDC(&hdc);
	if (hr == DD_OK)
	{
		::SetBkColor(hdc, RGB(0, 0, 0));
		RECT rc = { 0, 0, m_sizeScreen.cx, m_sizeScreen.cy };

		::ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);

        m_piDDSurface->ReleaseDC(hdc);
	}

	// Create the offscreen surface

	ZeroMemory(&ddsd, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
#ifdef DOUBLE_WIDTH
	ddsd.dwWidth = m_sizeSurface.cx * 2;
#else
	ddsd.dwWidth = m_sizeSurface.cx;
#endif

	ddsd.dwHeight = m_sizeSurface.cy;
	hr = m_piDD->CreateSurface(&ddsd, &m_piDDSurfaceBack, NULL);
	if (FAILED(hr))
	{
        TRACE( "CreateSurface failed, hr=%x", hr );
		return hr;
	}

	// Erase the surface

	hr = m_piDDSurfaceBack->GetDC(&hdc);
	if (hr == DD_OK)
	{
		::SetBkColor(hdc, RGB(0, 0, 0));
#ifdef DOUBLE_WIDTH
		m_sizeScreen.cx = m_sizeSurface.cx * 2;
		m_sizeScreen.cy = m_sizeSurface.cy;
#else
		m_sizeScreen.cx = m_sizeSurface.cx;
		m_sizeScreen.cy = m_sizeSurface.cy;
#endif
		RECT rc = { 0, 0, m_sizeScreen.cx, m_sizeScreen.cy };

		::ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);

		m_piDDSurfaceBack->ReleaseDC(hdc);
	}

    return S_OK;
}

HRESULT CDirectDraw::InitPalette(
	const ULONG* pulPalette
    )
{
	TRACE("CDirectDraw::InitPalette");

	// Create the palette and attach it to the primary surface

	PALETTEENTRY pe[256];

    int i;
	for ( i = 0; i < 256; ++i )
	{
		pe[i].peRed   = (BYTE)( (pulPalette[i] & 0x00FF0000) >> 16 );
		pe[i].peGreen = (BYTE)( (pulPalette[i] & 0x0000FF00) >> 8 );
		pe[i].peBlue  = (BYTE)( (pulPalette[i] & 0x000000FF) );
		pe[i].peFlags = 0;
	}

	HRESULT hr = S_OK;
    
    hr = m_piDD->CreatePalette( DDPCAPS_8BIT, 
                                pe, 
                                &m_piDDPalette, 
                                NULL );
    if ( FAILED(hr) )
    {
    	TRACE( "IDD::CreatePalette failed, hr=%X", hr);
        goto cleanup;
    }

    hr = m_piDDSurface->SetPalette( m_piDDPalette );
    if ( FAILED(hr) )
    {
        TRACE( "SetPalette failed, hr=%x", hr );
        goto cleanup;
    }

cleanup:

    if ( FAILED(hr) )
    {
        if (m_piDDPalette)
        {
            m_piDDPalette->Release();
            m_piDDPalette = NULL;
        }
    }

	return hr;
}

HRESULT CDirectDraw::Lock(
	BYTE** ppSurface, 
	LONG* plPitch
    )
{
	if (ppSurface == NULL || plPitch == NULL)
    {
		return E_INVALIDARG;
    }

	HRESULT hr;

	DDSURFACEDESC ddsd;
	ZeroMemory(&ddsd, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	
	hr = m_piDDSurfaceBack->Lock(
        NULL, 
        &ddsd, 
		DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, 
        NULL );
	if (hr != S_OK)
	{
		TRACE("Lock failed on back surface");
		return hr;
	}

	*ppSurface = (BYTE*)ddsd.lpSurface;
	*plPitch = ddsd.lPitch;

	return S_OK;
}

HRESULT CDirectDraw::Unlock(
	BYTE* pSurface
    )
{
	if (pSurface == NULL)
    {
        return E_INVALIDARG;
    }

	return m_piDDSurfaceBack->Unlock( pSurface );
}


HRESULT CDirectDraw::BltFast(
	const RECT* prc
    )
{
	HRESULT hr;

	for ( ; ; )
	{
		hr = m_piDDSurface->BltFast(
            m_ptBlitOffset.x, 
            m_ptBlitOffset.y,
			m_piDDSurfaceBack, 
            const_cast<RECT*>( prc ), 
            DDBLTFAST_NOCOLORKEY );
		if (hr == DD_OK)
        {
			break;
        }

		if (hr == DDERR_SURFACELOST)
		{
			m_piDDSurface->Restore();
			m_piDDSurfaceBack->Restore();
		}
		else if (hr != DDERR_WASSTILLDRAWING)
		{
			// FATAL ERROR
			TRACE("IDDS:BltFast failed, hr = %08X", hr);
			return E_FAIL;
		}
	}

	return S_OK;
}
