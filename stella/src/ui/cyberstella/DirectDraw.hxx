//
// StellaX
// Jeff Miller 05/06/2000
//

#ifndef DIRECTDR_H
#define DIRECTDR_H
#pragma once

class CDirectDraw
{
public:

	CDirectDraw( const SIZE& sizeSurface );
	~CDirectDraw();

    HRESULT Initialize( HWND hwnd, const ULONG* pulPalette, 
                        int cx = 0, int cy = 0 );

	HRESULT Lock( BYTE** ppSurface, LONG* lPitch );
	HRESULT Unlock( BYTE* pSurface );
	HRESULT BltFast( const RECT* prc );

private:

	HRESULT CreateSurfacesAndPalette( void );
	
	// param should really be a const BYTE*
	HRESULT InitPalette( const ULONG* pulPalette );

	void Cleanup( void );
	static HRESULT WINAPI EnumModesCallback( LPDDSURFACEDESC lpDDSurfaceDesc,
		LPVOID lpContext);

    BOOL m_fInitialized;

	IDirectDraw* m_piDD;
	IDirectDrawSurface* m_piDDSurface;
	IDirectDrawSurface* m_piDDSurfaceBack;
	IDirectDrawPalette* m_piDDPalette;

	SIZE m_sizeScreen;
	SIZE m_sizeSurface; // blit surface size
	POINT m_ptBlitOffset;

    HWND m_hwnd;

	CDirectDraw( const CDirectDraw& );  // no implementation
	void operator=( const CDirectDraw& );  // no implementation

};

#endif
