//
// StellaX
// Jeff Miller 05/12/2000
//
#ifndef DXFS_H
#define DXFS_H
#pragma once

class Console;
class MediaSource;

class CDirectInput;

#include "Event.hxx"
#include "GlobalData.hxx"

class CDirectXFullScreen
{
public:

	CDirectXFullScreen( const CGlobalData& rGlobalData,
                        const Console* pConsole,
                        Event& rEvent );
	~CDirectXFullScreen();

    HRESULT Initialize( int cx = 0, int cy = 0 );

	DWORD Run();

	operator HWND( void ) const
        {   
            return m_hwnd;
        }

private:

    BOOL m_fInitialized;

	static LRESULT CALLBACK StaticWindowProc( HWND hwnd, UINT uMsg, 
		WPARAM wParam, LPARAM lParam );
    BOOL WndProc( UINT uMsg, WPARAM wParam, LPARAM lParam );

	static HRESULT WINAPI EnumModesCallback( LPDDSURFACEDESC lpDDSurfaceDesc,
		LPVOID lpContext);

	void Cleanup();
	void UpdateEvents();
	BOOL UpdateDisplay( MediaSource& rMediaSource );

	HWND m_hwnd;
	BOOL m_fActiveWindow;

    RECT m_rectScreen;
    POINT m_ptBlitOffset;

	// Stella objects

	const Console* m_pConsole;
	Event& m_rEvent;

    const CGlobalData& m_rGlobalData;
	SIZE m_sizeGame;
	BYTE m_rgbPixelDataTable[256];

    //
	// DirectX
    //

	IDirectDraw* m_piDD;
	IDirectDrawSurface* m_piDDSPrimary;
	IDirectDrawSurface* m_piDDSBack;
	IDirectDrawPalette* m_piDDPalette;

	CDirectInput* m_pDirectMouse;
	CDirectInput* m_pDirectJoystick;
	CDirectInput* m_pDirectKeyboard;

    static LPCTSTR pszClassName;

	CDirectXFullScreen( const CDirectXFullScreen& );  // no implementation
	void operator=( const CDirectXFullScreen& );  // no implementation
};

#endif
