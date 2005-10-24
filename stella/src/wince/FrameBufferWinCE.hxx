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
// Copyright (c) 1995-2005 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// Windows CE Port by Kostas Nakos
//============================================================================

#ifndef FRAMEBUFFER_WINCE_HXX
#define FRAMEBUFFER_WINCE_HXX

#include <gx.h>
#include "bspf.hxx"
#include "FrameBuffer.hxx"
#include "OSystem.hxx"

class FrameBufferWinCE : public FrameBuffer
{
	public:

	FrameBufferWinCE(OSystem *osystem);
	~FrameBufferWinCE();
	virtual void setPalette(const uInt32* palette);
	virtual bool initSubsystem();
    virtual void setAspectRatio() ;
    virtual bool createScreen();
    virtual void toggleFilter();
    virtual void drawMediaSource();
    virtual void preFrameUpdate();
    virtual void postFrameUpdate();
    virtual void scanline(uInt32 row, uInt8* data);
    virtual Uint32 mapRGB(Uint8 r, Uint8 g, Uint8 b);
    virtual void hLine(uInt32 x, uInt32 y, uInt32 x2, OverlayColor color);
    virtual void vLine(uInt32 x, uInt32 y, uInt32 y2, OverlayColor color);
    virtual void fillRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h, OverlayColor color);
    virtual void drawChar(const GUI::Font* font, uInt8 c, uInt32 x, uInt32 y, OverlayColor color);
    virtual void drawBitmap(uInt32* bitmap, Int32 x, Int32 y, OverlayColor color, Int32 h = 8);
    virtual void translateCoords(Int32* x, Int32* y);
    virtual void addDirtyRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h);
    virtual uInt32 lineDim();
	void wipescreen(void);
	void setmode(uInt8 mode);
	uInt8 rotatedisplay(void);
	
	private:

	void lateinit(void);
	void PlothLine(uInt32 x, uInt32 y, uInt32 x2, OverlayColor color);
    void PlotvLine(uInt32 x, uInt32 y, uInt32 y2, OverlayColor color);
	void PlotfillRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h, OverlayColor color);

	uInt16 pal[256], myWidth, myHeight, guipal[kNumColors-256], scrwidth, scrheight;
	Int32 pixelstep, linestep, scrpixelstep, scrlinestep;
	uInt32 displacement;
	bool SubsystemInited;
	uInt8 *myDstScreen;

	bool issmartphone, islandscape;
	uInt16 minydim, optgreenmaskN, optgreenmask;
	Int32 pixelsteptimes5, pixelsteptimes6;
	GXDisplayProperties gxdp;
	uInt8 displaymode;

	public:
	bool IsSmartphone(void) { return issmartphone; }
	uInt8 getmode(void) { return displaymode; }
};

#endif