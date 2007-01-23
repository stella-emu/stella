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
// Copyright (c) 1995-2007 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// Windows CE Port by Kostas Nakos
// $Id: FrameBufferWinCE.hxx,v 1.13 2007-01-23 09:40:57 knakos Exp $
//============================================================================

#ifndef FRAMEBUFFER_WINCE_HXX
#define FRAMEBUFFER_WINCE_HXX

#include <gx.h>
#include "bspf.hxx"
#include "FrameBuffer.hxx"
#include "OSystem.hxx"


// The necessary nonsense for extended resolutions
#define GETRAWFRAMEBUFFER   0x00020001

#define FORMAT_565 1
#define FORMAT_555 2
#define FORMAT_OTHER 3

#if _WIN32_WCE <= 300
typedef struct _RawFrameBufferInfo
{
   WORD wFormat;
   WORD wBPP;
   VOID *pFramePointer;
   int  cxStride;
   int  cyStride;
   int  cxPixels;
   int  cyPixels;
} RawFrameBufferInfo;
#endif

class FrameBufferWinCE : public FrameBuffer
{
	public:

	FrameBufferWinCE(OSystem *osystem);
	~FrameBufferWinCE();
	virtual void setTIAPalette(const uInt32* palette);
	virtual void setUIPalette(const uInt32* palette);
	virtual bool initSubsystem();
	virtual BufferType type() { return kSoftBuffer; } 
    virtual void setAspectRatio() { return; };
    virtual bool createScreen() { return true; };
    virtual void toggleFilter() { return; };
    virtual void drawMediaSource();
    virtual void preFrameUpdate();
    virtual void postFrameUpdate();
	virtual void scanline(uInt32 row, uInt8* data) { return; };
    virtual Uint32 mapRGB(Uint8 r, Uint8 g, Uint8 b) { return 0xFFFFFFFF; };
    virtual void hLine(uInt32 x, uInt32 y, uInt32 x2, int color);
    virtual void vLine(uInt32 x, uInt32 y, uInt32 y2, int color);
    virtual void fillRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h, int color);
    virtual void drawChar(const GUI::Font* font, uInt8 c, uInt32 x, uInt32 y, int color);
    virtual void drawBitmap(uInt32* bitmap, Int32 x, Int32 y, int color, Int32 h = 8);
    virtual void translateCoords(Int32* x, Int32* y);
    virtual void addDirtyRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h);
	virtual void enablePhosphor(bool enable, int blend)  { return; };
    virtual uInt32 lineDim() { return 1; };
	virtual string about();
	virtual void setScaler(Scaler scaler) { return; };
	void wipescreen(bool atinit=false);
	void setmode(uInt8 mode);
	uInt8 rotatedisplay(void);
	
	private:

	void lateinit(void);
	void PlothLine(uInt32 x, uInt32 y, uInt32 x2, int color);
    void PlotvLine(uInt32 x, uInt32 y, uInt32 y2, int color);
	void PlotfillRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h, int color);
	void GetDeviceProperties(void);

	uInt16 pal[256+kNumColors], myWidth, myWidthdiv4, myHeight, scrwidth, scrheight;
	Int32 pixelstep, pixelstepdouble, linestep, linestepdouble, scrpixelstep, scrlinestep;
	uInt32 paldouble[256+kNumColors], displacement;
	bool SubsystemInited;
	uInt8 *myDstScreen;

	bool issmartphone, islandscape, legacygapi, screenlocked;
	enum {SM_LOW, QVGA, VGA} devres;
	uInt16 minydim, optgreenmaskN, optgreenmask;
	Int32 pixelsteptimes5, pixelsteptimes6, pixelsteptimes8, pixelsteptimes12, pixelsteptimes16;
	GXDisplayProperties gxdp;
	uInt8 displaymode;

	public:
	bool IsSmartphone(void) { return issmartphone; }
	bool IsSmartphoneLowRes(void) { return (issmartphone && devres==SM_LOW); }
	bool IsVGA(void) { return (devres==VGA); }
	uInt8 getmode(void) { return displaymode; }
	bool IsLandscape(void) { return islandscape; }
};

#endif
