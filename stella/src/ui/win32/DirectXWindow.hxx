//
// StellaX
// Jeff Miller 05/15/2000
//
#ifndef DXWIN_H
#define DXWIN_H
#pragma once

class Console;
class MediaSource;

class CDirectInput;

#include "Event.hxx"
#include "GlobalData.hxx"

class CDirectXWindow
{
public:

	CDirectXWindow( const CGlobalData& rGlobalData,
                    const Console* pConsole, 
                    Event& rEvent );
	~CDirectXWindow();

    HRESULT Initialize( HWND hwndParent, LPCSTR pszTitle );

	DWORD Run();

    operator HWND( void ) const { return m_hwnd; }

private:

    void ReleaseAllObjects( void );
    HRESULT InitSurfaces( void );
    HRESULT ChangeCoopLevel( void );

	HWND m_hwnd;

    BOOL m_fReady;
    BOOL m_fWindowed;
	BOOL m_fActive;

    RECT m_rcWindow;
    RECT m_rcScreen;

    IDirectDraw* m_piDD;
    IDirectDrawSurface* m_piDDSPrimary;
    IDirectDrawSurface* m_piDDSBack;

    SIZE m_sizeFS;

	static HRESULT WINAPI EnumModesCallback( LPDDSURFACEDESC lpDDSurfaceDesc,
		LPVOID lpContext);

	static LRESULT CALLBACK StaticWindowProc( HWND hwnd, UINT uMsg, 
		WPARAM wParam, LPARAM lParam );
    BOOL WndProc( UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* plResult );

	void UpdateEvents();
	BOOL UpdateDisplay( MediaSource& rMediaSource );

    PALETTEENTRY m_rgpe[256];

	// Stella objects

	const Console* m_pConsole;
	Event& m_rEvent;

    const CGlobalData& m_rGlobalData;
    SIZE m_sizeGame;
	BYTE m_rgbPixelDataTable[256];


    //
	// DirectX
    //

    IDirectDrawPalette* m_piDDPalette;

    CDirectInput* m_pDirectMouse;
	CDirectInput* m_pDirectJoystick;
	CDirectInput* m_pDirectKeyboard;

    static LPCTSTR pszClassName;

	CDirectXWindow( const CDirectXWindow& );  // no implementation
	void operator=( const CDirectXWindow& );  // no implementation
};

#endif
