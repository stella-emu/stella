//============================================================================
//
//   SSSS    tt          lll  lll
//  SS  SS   tt           ll   ll
//  SS     tttttt  eeee   ll   ll   aaaa
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2002 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: MainWin32.hxx,v 1.1 2003-11-13 00:25:07 stephena Exp $
//============================================================================

#ifndef MAIN_WIN32_HXX
#define MAIN_WIN32_HXX

class Console;
class FrameBufferWin32;
class MediaSource;
class PropertiesSet;
class Sound;
class Settings;
class DirectInput;

#include "GlobalData.hxx"
#include "FrameBuffer.hxx"
#include "Event.hxx"
#include "StellaEvent.hxx"

/**
  This class implements a main-like method where all per-game
  instantiation is done.  Event gathering is also done here.

  This class is meant to be quite similar to the mainDOS or mainSDL
  classes so that all platforms have a main-like method as described
  in the Porting.txt document

  @author  Stephen Anthony
  @version $Id: MainWin32.hxx,v 1.1 2003-11-13 00:25:07 stephena Exp $
*/
class MainWin32
{
  public:
    /**
	  Create a new instance of the emulation core for the specified
      rom image.

      @param image       The ROM image of the game to emulate 
      @param size        The size of the ROM image
      @param filename    The name of the file that contained the ROM image
      @param settings    The settings object to use
      @param properties  The game profiles object to use
     */
    MainWin32(const uInt8* image, uInt32 size, const char* filename,
              Settings& settings, PropertiesSet& properties);
    /**
      Destructor
    */ 
    virtual ~MainWin32();

    // Start the main emulation loop
    DWORD run();

  private:
    void UpdateEvents();

  private:
    // Pointer to the console object
    Console* theConsole;

    // Reference to the settings object
    Settings& theSettings;

    // Reference to the properties set object
    PropertiesSet& thePropertiesSet;

    // Pointer to the display object
    FrameBufferWin32* theDisplay;

    // Pointer to the sound object
    Sound* theSound;

    // Pointer to the input object
    DirectInput* theInput;

    struct Switches
    {
      uInt32 nVirtKey;
      StellaEvent::KeyCode keyCode;
    };
    static Switches keyList[StellaEvent::LastKCODE];


/////////////////////////////////////////////////////////
//
// These will move into a separate Framebuffer class soon
//
/////////////////////////////////////////////////////////
#if 0
  public:
    HWND hwnd() const { return myHWND; }

    //////////////////////////////////////////////////////////////////////
    // The following methods are derived from FrameBuffer.hxx
    //////////////////////////////////////////////////////////////////////
    /**
      This routine should be called once the console is created to setup
      the video system for us to use.  Return false if any operation fails,
      otherwise return true.
    */
    bool init();

    /**
      This routine should be called anytime the MediaSource needs to be redrawn
      to the screen.
    */
    void drawMediaSource();

    /**
      This routine should be called to draw a rectangular box with sides
      at the specified coordinates.

      @param x   The x coordinate
      @param y   The y coordinate
      @param w   The width of the box
      @param h   The height of the box
    */
    void drawBoundedBox(uInt32 x, uInt32 y, uInt32 w, uInt32 h);

    /**
      This routine should be called to draw text at the specified coordinates.

      @param x        The x coordinate
      @param y        The y coordinate
      @param message  The message text
    */
    void drawText(uInt32 x, uInt32 y, const string& message);

    /**
      This routine should be called to draw character 'c' at the specified coordinates.

      @param x   The x coordinate
      @param y   The y coordinate
      @param c   The character to draw
    */
    void drawChar(uInt32 x, uInt32 y, uInt32 c);

    /**
      This routine is called before any drawing is done (per-frame).
    */
    void preFrameUpdate();

    /**
      This routine is called after any drawing is done (per-frame).
    */
    void postFrameUpdate();

    /**
      This routine is called when the emulation has received
      a pause event.

      @param status  The received pause status
    */
    virtual void pauseEvent(bool status);
#endif
//////////////////////////////////////////////////
// Some of this will stay here, some will go to
// the FrameBufferWin32 class
//////////////////////////////////////////////////
  private:
    const CGlobalData* m_rGlobalData;

    bool myIsInitialized;

    void cleanup();
#if 0
    static LRESULT CALLBACK StaticWindowProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
    BOOL WndProc( UINT uMsg, WPARAM wParam, LPARAM lParam );

    static HRESULT WINAPI EnumModesCallback( LPDDSURFACEDESC lpDDSurfaceDesc, LPVOID lpContext);

    void cleanup();

    HWND myHWND;
    bool m_fActiveWindow;

    RECT m_rectScreen;
    POINT m_ptBlitOffset;

    // Stella objects
    SIZE mySizeGame;
    BYTE myPalette[256];

    //
    // DirectX
    //
    IDirectDraw* m_piDD;
    IDirectDrawSurface* m_piDDSPrimary;
    IDirectDrawSurface* m_piDDSBack;
    IDirectDrawPalette* m_piDDPalette;

    static LPCTSTR pszClassName;
#endif
};

#endif
