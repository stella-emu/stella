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
// $Id: FrameBufferWinCE.cpp,v 1.14 2007-02-12 11:16:59 knakos Exp $
//============================================================================

#include <windows.h>
#include "FrameBufferWinCE.hxx"
#include "Console.hxx"
#include "OSystem.hxx"
#include "Font.hxx"

#define OPTPIXAVERAGE(pix1,pix2) ( ((((pix1 & optgreenmaskN) + (pix2 & optgreenmaskN)) >> 1) & optgreenmaskN) | ((((pix1 & optgreenmask) + (pix2 & optgreenmask)) >> 1) & optgreenmask) )

FrameBufferWinCE::FrameBufferWinCE(OSystem *osystem): FrameBuffer(osystem), 
	myDstScreen(NULL), SubsystemInited(false), displacement(0), issquare(false),
	issmartphone(true), islandscape(false), legacygapi(true), devres(SM_LOW), screenlocked(false)
{
	gxdp.cxWidth = gxdp.cyHeight = gxdp.cbxPitch = gxdp.cbyPitch = gxdp.cBPP = gxdp.ffFormat = 0;
	displaymode = myOSystem->settings().getInt("wince_orientation");
	if (displaymode > 2) displaymode = 0;
}

FrameBufferWinCE::~FrameBufferWinCE()
{
}

void FrameBufferWinCE::GetDeviceProperties(void)
{
	if (gxdp.cxWidth) return;

	// screen access mode
	gxdp = GXGetDisplayProperties();
	legacygapi = true;
	if (((unsigned int) GetSystemMetrics(SM_CXSCREEN) != gxdp.cxWidth) || ((unsigned int) GetSystemMetrics(SM_CYSCREEN) != gxdp.cyHeight))
	{
		// 2003SE+ and lying about the resolution. good luck.
		legacygapi = false;

		RawFrameBufferInfo rfbi;
		HDC hdc = GetDC(NULL);
		ExtEscape(hdc, GETRAWFRAMEBUFFER, 0, NULL, sizeof(RawFrameBufferInfo), (char *) &rfbi);
		ReleaseDC(NULL, hdc);

		if (rfbi.wFormat == FORMAT_565)
			gxdp.ffFormat = kfDirect565;
		else if (rfbi.wFormat == FORMAT_555)
			gxdp.ffFormat = kfDirect555;
		else
			gxdp.ffFormat = 0;
		gxdp.cBPP = rfbi.wBPP;
		gxdp.cbxPitch = rfbi.cxStride;
		gxdp.cbyPitch = rfbi.cyStride;
		gxdp.cxWidth  = rfbi.cxPixels;
		gxdp.cyHeight = rfbi.cyPixels;
	}

	// device detection (some redundancy here, but nevermind :)
	TCHAR platform[100];
	issmartphone = false;
	if (gxdp.cxWidth == 176 && gxdp.cyHeight == 220)
		issmartphone = true;
	if (SystemParametersInfo(SPI_GETPLATFORMTYPE, 100, platform, 0))
	{
		if (wcsstr(platform, _T("mart")))
			issmartphone = true;
	}
	else
		issmartphone = true;	// most likely

	if (gxdp.cxWidth == 176 && gxdp.cyHeight == 220)
		devres = SM_LOW;
	else if (gxdp.cxWidth == 480 && gxdp.cyHeight == 640)
		devres = VGA;
	else
	{
		devres = QVGA;
		// as a special case, qvga landscape devices are represented by the combination of:
		// devres=QVGA && islandscape=true && displaymode=0
		if (gxdp.cxWidth > gxdp.cyHeight)
			islandscape = true;
		// square QVGA (240x240) devices are portrait, landscape (as above) and square
		else if (gxdp.cxWidth == gxdp.cyHeight)
			issquare = islandscape = true;

	}
}

void FrameBufferWinCE::setTIAPalette(const uInt32* palette)
{
	GetDeviceProperties();
	for (uInt16 i=0; i<256; i++)
	{
		uInt8 r = (uInt8) ((palette[i] & 0xFF0000) >> 16);
		uInt8 g = (uInt8) ((palette[i] & 0x00FF00) >> 8);
		uInt8 b = (uInt8)  (palette[i] & 0x0000FF);
		if(gxdp.ffFormat & kfDirect565)
			pal[i] = (uInt16) ( ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3) );
		else if(gxdp.ffFormat & kfDirect555)
			pal[i] = (uInt16) ( ((r & 0xF8) << 7) | ((g & 0xF8) << 3) | ((b & 0xF8) >> 3) );
		else
			return;
		paldouble[i] = pal[i] | (pal[i] << 16);
	}
	SubsystemInited = false;
}

void FrameBufferWinCE::setUIPalette(const uInt32* palette)
{
	GetDeviceProperties();
	for (int i=0; i<kNumColors - 256; i++)
	{
		uInt8 r = (uInt8) ((palette[i] & 0xFF0000) >> 16);
		uInt8 g = (uInt8) ((palette[i] & 0x00FF00) >> 8);
		uInt8 b = (uInt8)  (palette[i] & 0x0000FF);
		if(gxdp.ffFormat & kfDirect565)
			pal[i+256] = (uInt16) ( ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3) );
		else if(gxdp.ffFormat & kfDirect555)
			pal[i+256] = (uInt16) ( ((r & 0xF8) << 7) | ((g & 0xF8) << 3) | ((b & 0xF8) >> 3) );
		else
			return;
		paldouble[i+256] = pal[i+256] | (pal[i+256] << 16);
	}
	SubsystemInited = false;
}

bool FrameBufferWinCE::initSubsystem()
{
	static bool firsttime = true;

	GetDeviceProperties();

	// we need to modify our basedim rect in order to center the ingame menu properly
	// this is, more or less, a hack
	if (IsSmartphoneLowRes())
	{
		myBaseDim.w = 220;
		myBaseDim.h = 176;
	}
	else if (issquare)
	{
		myBaseDim.w = 240;
		myBaseDim.h = 240;
	}

	// screen extents
	if(gxdp.ffFormat & kfDirect565)
	{
		optgreenmask = 0x7E0;
		optgreenmaskN = 0xF81F;
	}
	else
	{
		optgreenmask = 0x3E0;
		optgreenmaskN = 0x7C1F;
	}

	scrwidth = gxdp.cxWidth;
	scrheight = gxdp.cyHeight;
	scrpixelstep = gxdp.cbxPitch;
	scrlinestep = gxdp.cbyPitch;

	setmode(displaymode);
	if (!firsttime)
		wipescreen(true);		// hold the 'initializing' screen on startup
	firsttime = false;
	SubsystemInited = false;
	return true;
}

void FrameBufferWinCE::setmode(uInt8 mode)
{
	bool qvga_l = (displaymode == 0) && islandscape;
	
	displaymode = mode % 3;
	switch (displaymode)
	{
		// portrait
		case 0:
			pixelstep = gxdp.cbxPitch;
			linestep = gxdp.cbyPitch;
			break;

		// landscape
		case 1:
			pixelstep = - gxdp.cbyPitch;
			linestep = gxdp.cbxPitch;
			break;

		// inverted landscape
		case 2:
			pixelstep = gxdp.cbyPitch;
			linestep = - gxdp.cbxPitch;
			break;
	}
	islandscape = displaymode || qvga_l;

	pixelstepdouble = pixelstep << 1;
	linestepdouble = linestep << 1;
	pixelsteptimes5 = pixelstep * 5;
	pixelsteptimes6 = pixelstep * 6;
	pixelsteptimes8 = pixelstep * 8;
	pixelsteptimes12 = pixelstep * 12;
	pixelsteptimes16 = pixelstep * 16;
	SubsystemInited = false;
}

uInt8 FrameBufferWinCE::rotatedisplay(void)
{
	if (!(displaymode == 0 && islandscape))
	{
		islandscape = false;
		displaymode = (displaymode + 1) % 3;
	}
	setmode(displaymode);
	wipescreen();
	myOSystem->settings().setInt("wince_orientation", displaymode);
	return displaymode;
}

void FrameBufferWinCE::lateinit(void)
{
	int w, h;

	myWidth = myOSystem->console().mediaSource().width();
	myHeight = myOSystem->console().mediaSource().height();
	myWidthdiv4 = myWidth >> 2;

	if (devres == SM_LOW)
		if (!islandscape)
			w = myWidth;
		else
			w = (int) ((float) myWidth * 11.0f / 8.0f + 0.5f);
	else if (devres == QVGA)
		if (!islandscape && !issquare)
			w = (int) ((float) myWidth * 3.0f / 2.0f + 0.5f);
		else
			w = myWidth * 2;
	else
	{
		if (!islandscape)
			w = (int) ((float) myWidth * 3.0f + 0.5f);
		else
			w = myWidth * 4;
	}

	if (devres == SM_LOW && islandscape)
		h = (int) ((float) myHeight * 4.0f / 5.0f + 0.5f);
	else if (devres == VGA)
		h = myHeight * 2;
	else
		h = myHeight;

	switch (displaymode)
	{
		case 0:
			if (scrwidth > w)
				displacement = (scrwidth - w) / 2 * gxdp.cbxPitch;
			else
				displacement = 0;
			if (scrheight > h)
			{
				displacement += (scrheight - h) / 2 * gxdp.cbyPitch;
				minydim = h;
			}
			else
				minydim = scrheight;
			break;

		case 1:
			displacement = gxdp.cbyPitch*(gxdp.cyHeight-1);
			if (scrwidth > h)
			{
				minydim = h;
				displacement += (scrwidth - h) / 2 * gxdp.cbxPitch;
			}
			else
				minydim = scrwidth;
			if (scrheight > w)
				displacement -= (scrheight - w) / 2 * gxdp.cbyPitch;
			break;

		case 2:
			displacement = gxdp.cbxPitch*(gxdp.cxWidth-1);
			if (scrwidth > h)
			{
				minydim = h;
				displacement -= (scrwidth - h) / 2 * gxdp.cbxPitch;
			}
			else
				minydim = scrwidth;
			if (scrheight > w)
				displacement += (scrheight - w) / 2 * gxdp.cbyPitch;
			break;

	}

	if (devres == VGA)
	{
		minydim >>= 1;
		displacement &= ~3;		// ensure longword alignment
	}

	SubsystemInited = true;
}

void FrameBufferWinCE::preFrameUpdate()
{
	static HDC hdc;
	static RawFrameBufferInfo rfbi;

	if (screenlocked)
		return;

	if (legacygapi)
		myDstScreen = (uInt8 *) GXBeginDraw();
	else
	{
		hdc = GetDC(NULL);
		ExtEscape(hdc, GETRAWFRAMEBUFFER, 0, NULL, sizeof(RawFrameBufferInfo), (char *) &rfbi);
		ReleaseDC(NULL, hdc);
		myDstScreen = (uInt8 *) rfbi.pFramePointer;
	}

	screenlocked = true;
}

void FrameBufferWinCE::drawMediaSource()
{
	static uInt8 *sc, *sp, *sc_n, *sp_n;
	static uInt8 *d, *pl, *nl;
	static uInt16 pix1, pix2, pix3, x, y;
	uInt32 pix1d, pix2d, pix3d;

	if (!SubsystemInited)
		lateinit();
	
	if ( (d = myDstScreen) == NULL )
		return;

	d += displacement;
	pl = d;
	sc = myOSystem->console().mediaSource().currentFrameBuffer();
	sp = myOSystem->console().mediaSource().previousFrameBuffer();

	if (theRedrawTIAIndicator)
	{
		memset(sp, 0, myWidth*myHeight-1);
		memset(myDstScreen, 0, scrwidth*scrheight*2);
		theRedrawTIAIndicator = false;
	}
	
	if (devres == SM_LOW && !islandscape)
	{
		// straight
		for (y=0; y<minydim; y++)
		{
			for (x=0; x<myWidthdiv4; x++)
			{
				if ( *((uInt32 *) sc) != *((uInt32 *) sp) )
				{
					*((uInt16 *)d) = pal[*sc++]; d += pixelstep;
					*((uInt16 *)d) = pal[*sc++]; d += pixelstep;
					*((uInt16 *)d) = pal[*sc++]; d += pixelstep;
					*((uInt16 *)d) = pal[*sc++]; d += pixelstep;
				}
				else
				{
					sc += 4;
					d += (pixelstep << 2);
				}
				sp += 4;
			}
			d = pl + linestep;
			pl = d;
		}
	}
	else if (devres == SM_LOW && islandscape)
	{
		for (y=0; y<minydim; y++)
		{
			// 4/5
			if ((y & 3) ^ 3)
			{	// normal line
				for (x=0; x<myWidthdiv4; x++)
				{
					// 11/8
					// **X**
					if ( *((uInt32 *) sc) != *((uInt32 *) sp) )
					{
						*((uInt16 *)d) = pal[*sc++]; d += pixelstep;
						pix1 = pal[*sc++]; pix2 = pal[*sc++];
						*((uInt16 *)d) = pix1; d += pixelstep;
						*((uInt16 *)d) = OPTPIXAVERAGE(pix1,pix2); d += pixelstep;
						*((uInt16 *)d) = pix2; d += pixelstep;
						*((uInt16 *)d) = pal[*sc++]; d += pixelstep;
					}
					else
					{
						sc += 4;
						d += pixelsteptimes5;
					}
					sp += 4;
					if (++x>=myWidthdiv4) break;

					// *X**X*
					if ( *((uInt32 *) sc) != *((uInt32 *) sp) )
					{
						pix1 = pal[*sc++]; pix2 = pal[*sc++];
						*((uInt16 *)d) = pix1; d += pixelstep;
						*((uInt16 *)d) = OPTPIXAVERAGE(pix1,pix2); d += pixelstep;
						*((uInt16 *)d) = pix2; d += pixelstep;
						pix1 = pal[*sc++]; pix2 = pal[*sc++];
						*((uInt16 *)d) = pix1; d += pixelstep;
						*((uInt16 *)d) = OPTPIXAVERAGE(pix1,pix2); d += pixelstep;
						*((uInt16 *)d) = pix2; d += pixelstep;
					}
					else
					{
						sc += 4;
						d += pixelsteptimes6;
					}
					sp += 4;
				}
			}
			else
			{	// skipped line
				sc_n = sc + myWidth;
				sp_n = sp + myWidth;
				for (x=0; x<myWidthdiv4; x++)
				{
					// 11/8
					// **X**
					if ( (*((uInt32 *) sc) != *((uInt32 *) sp)) || (*((uInt32 *) sc_n) != *((uInt32 *) sp_n)) )
					{
						pix1 = pal[*sc++]; pix2 = pal[*sc_n++];
						*((uInt16 *)d) = OPTPIXAVERAGE(pix1,pix2); d += pixelstep;
						pix1 = pal[*sc++]; pix2 = pal[*sc_n++];
						pix1 = OPTPIXAVERAGE(pix1,pix2);
						*((uInt16 *)d) = pix1; d += pixelstep;
						pix2 = pal[*sc++]; pix3 = pal[*sc_n++];
						pix2 = OPTPIXAVERAGE(pix2,pix3);
						pix3 = OPTPIXAVERAGE(pix1,pix2);
						*((uInt16 *)d) = pix3; d += pixelstep;
						*((uInt16 *)d) = pix2; d += pixelstep;
						pix1 = pal[*sc++]; pix2 = pal[*sc_n++];
						*((uInt16 *)d) = OPTPIXAVERAGE(pix1,pix2); d += pixelstep;
					}
					else
					{
						sc += 4;
						sc_n += 4;
						d += pixelsteptimes5;
					}
					sp += 4;
					sp_n += 4;
					if (++x>=myWidthdiv4) break;

					// *X**X*
					if ( (*((uInt32 *) sc) != *((uInt32 *) sp)) || (*((uInt32 *) sc_n) != *((uInt32 *) sp_n)) )
					{
						pix1 = pal[*sc++]; pix2 = pal[*sc_n++];
						pix1 = OPTPIXAVERAGE(pix1,pix2);
						*((uInt16 *)d) = pix1; d += pixelstep;
						pix2 = pal[*sc++]; pix3 = pal[*sc_n++];
						pix2 = OPTPIXAVERAGE(pix2,pix3);
						pix3 = OPTPIXAVERAGE(pix1,pix2);
						*((uInt16 *)d) = pix3; d += pixelstep;
						*((uInt16 *)d) = pix2; d += pixelstep;
						pix1 = pal[*sc++]; pix2 = pal[*sc_n++];
						pix1 = OPTPIXAVERAGE(pix1,pix2);
						*((uInt16 *)d) = pix1; d += pixelstep;
						pix2 = pal[*sc++]; pix3 = pal[*sc_n++];
						pix2 = OPTPIXAVERAGE(pix2,pix3);
						pix3 = OPTPIXAVERAGE(pix1,pix2);
						*((uInt16 *)d) = pix3; d += pixelstep;
						*((uInt16 *)d) = pix2; d += pixelstep;
					}
					else
					{
						sc += 4;
						sc_n += 4;
						d += pixelsteptimes6;
					}
					sp += 4;
					sp_n += 4;
				}
				sc += myWidth;
				sp += myWidth;
			}
			d = pl + linestep;
			pl = d;
		}
	}
	else if (devres == QVGA && (!islandscape || issquare))
	{
		// 3/2
		for (y=0; y<minydim; y++)
		{
			for (x=0; x<myWidthdiv4; x++)
			{
				if ( *((uInt32 *) sc) != *((uInt32 *) sp) )
				{
					pix1 = pal[*sc++]; pix2 = pal[*sc++];
					*((uInt16 *)d) = pix1; d += pixelstep;
					*((uInt16 *)d) = OPTPIXAVERAGE(pix1,pix2); d += pixelstep;
					*((uInt16 *)d) = pix2; d += pixelstep;
					pix1 = pal[*sc++]; pix2 = pal[*sc++];
					*((uInt16 *)d) = pix1; d += pixelstep;
					*((uInt16 *)d) = OPTPIXAVERAGE(pix1,pix2); d += pixelstep;
					*((uInt16 *)d) = pix2; d += pixelstep;
				}
				else
				{
					sc += 4;
					d += pixelsteptimes6;
				}
				sp += 4;
			}
			d = pl + linestep;
			pl = d;
		}
	}
	else if (devres == QVGA && islandscape)
	{
		// 2/1
		for (y=0; y<minydim; y++)
		{
			for (x=0; x<myWidthdiv4; x++)
			{
				if ( *((uInt32 *) sc) != *((uInt32 *) sp) )
				{
					pix1 = pal[*sc++]; pix2 = pal[*sc++];
					*((uInt16 *)d) = pix1; d += pixelstep;
					*((uInt16 *)d) = OPTPIXAVERAGE(pix1,pix2); d += pixelstep;
					pix1 = pal[*sc++];
					*((uInt16 *)d) = pix2; d += pixelstep;
					*((uInt16 *)d) = OPTPIXAVERAGE(pix1,pix2); d += pixelstep;
					pix2 = pal[*sc++];
					*((uInt16 *)d) = pix1; d += pixelstep;
					*((uInt16 *)d) = OPTPIXAVERAGE(pix1,pix2); d += pixelstep;
					*((uInt16 *)d) = pix2; d += pixelstep;
					*((uInt16 *)d) = pix2; d += pixelstep;
				}
				else
				{
					sc += 4;
					d += pixelsteptimes8;
				}
				sp += 4;
			}
			d = pl + linestep;
			pl = d;
		}
	}
	else if (devres == VGA && !islandscape)
	{
		for (y=0; y<minydim; y++)
		{
			// 2/1
			for (x=0; x<myWidthdiv4; x++)
			{
				// 3/1
				if ( *((uInt32 *) sc) != *((uInt32 *) sp) )
				{
					nl = d + linestep;
					pix1d = paldouble[*sc++]; pix2d = paldouble[*sc++];
					*((uInt32 *)d) = pix1d; d += pixelstepdouble;
					*((uInt32 *)nl) = pix1d; nl += pixelstepdouble;
					pix3d = ((uInt16) pix1d) | (((uInt16) pix2d) << 16);
					*((uInt32 *)d) = pix3d; d += pixelstepdouble;
					*((uInt32 *)nl) = pix3d; nl += pixelstepdouble;
					*((uInt32 *)d) = pix2d; d += pixelstepdouble;
					*((uInt32 *)nl) = pix2d; nl += pixelstepdouble;
					pix1d = paldouble[*sc++]; pix2d = paldouble[*sc++];
					*((uInt32 *)d) = pix1d; d += pixelstepdouble;
					*((uInt32 *)nl) = pix1d; nl += pixelstepdouble;
					pix3d = ((uInt16) pix1d) | (((uInt16) pix2d) << 16);
					*((uInt32 *)d) = pix3d; d += pixelstepdouble;
					*((uInt32 *)nl) = pix3d; nl += pixelstepdouble;
					*((uInt32 *)d) = pix2d; d += pixelstepdouble;
					*((uInt32 *)nl) = pix2d;
				}
				else
				{
					sc += 4;
					d += pixelsteptimes12;
				}
				sp += 4;
			}
			d = pl + linestepdouble;
			pl = d;
		}
	}
	else if (devres == VGA && islandscape)
	{
		for (y=0; y<minydim; y++)
		{
			// 2/1
			for (x=0; x<myWidthdiv4; x++)
			{
				// 4/1
				if ( *((uInt32 *) sc) != *((uInt32 *) sp) )
				{
					nl = d + pixelstep;
					pix1d = paldouble[*sc++];
					*((uInt32 *)d) = pix1d; d += pixelstepdouble;
					*((uInt32 *)nl) = pix1d; nl += pixelstepdouble;
					*((uInt32 *)d) = pix1d; d += pixelstepdouble;
					*((uInt32 *)nl) = pix1d; nl += pixelstepdouble;
					pix1d = paldouble[*sc++];
					*((uInt32 *)d) = pix1d; d += pixelstepdouble;
					*((uInt32 *)nl) = pix1d; nl += pixelstepdouble;
					*((uInt32 *)d) = pix1d; d += pixelstepdouble;
					*((uInt32 *)nl) = pix1d; nl += pixelstepdouble;
					pix1d = paldouble[*sc++];
					*((uInt32 *)d) = pix1d; d += pixelstepdouble;
					*((uInt32 *)nl) = pix1d; nl += pixelstepdouble;
					*((uInt32 *)d) = pix1d; d += pixelstepdouble;
					*((uInt32 *)nl) = pix1d; nl += pixelstepdouble;
					pix1d = paldouble[*sc++];
					*((uInt32 *)d) = pix1d; d += pixelstepdouble;
					*((uInt32 *)nl) = pix1d; nl += pixelstepdouble;
					*((uInt32 *)d) = pix1d; d += pixelstepdouble;
					*((uInt32 *)nl) = pix1d;
				}
				else
				{
					sc += 4;
					d += pixelsteptimes16;
				}
				sp += 4;
			}
			d = pl + linestepdouble;
			pl = d;
		}
	}

}

void FrameBufferWinCE::wipescreen(bool atinit)
{
	if (!atinit)
	{
		if (!SubsystemInited)
			lateinit();

		uInt8 *s = myOSystem->console().mediaSource().currentFrameBuffer();
		memset(s, 0, myWidth*myHeight-1);
		s = myOSystem->console().mediaSource().previousFrameBuffer();
		memset(s, 0, myWidth*myHeight-1);
	}

	preFrameUpdate();
	memset(myDstScreen, 0, scrwidth*scrheight*2);
	postFrameUpdate();
}

void FrameBufferWinCE::postFrameUpdate()
{
	if (!screenlocked)	return;
	if (legacygapi)		GXEndDraw();
}

void FrameBufferWinCE::drawChar(const GUI::Font* myfont, uInt8 c, uInt32 x, uInt32 y, int color)
{
	const FontDesc& desc = myfont->desc();

	if (!myDstScreen) return;

	if (c < desc.firstchar || c >= desc.firstchar + desc.size)
	{
		if (c == ' ')
			return;
		c = desc.defaultchar;
	}

	Int32 w = myfont->getCharWidth(c);
	const Int32 h = myfont->getFontHeight();
	c -= desc.firstchar;
	const uInt16* tmp = desc.bits + (desc.offset ? desc.offset[c] : (c * h));

	if ((Int32)x<0 || (Int32)y<0 || (x>>1)+w>scrwidth || y+h>scrheight) return;

	uInt8 *d;
	if (devres != VGA)
	{
		if (!displaymode && islandscape)
			d = myDstScreen + y * scrlinestep + x * scrpixelstep;
		else if (displaymode != 2)
			d = myDstScreen + (scrheight-x) * scrlinestep + y * scrpixelstep;
		else
			d = myDstScreen + x * scrlinestep + (scrwidth-y) * scrpixelstep;
	}
	else
	{
		if (displaymode != 2)
			d = myDstScreen + ((scrheight>>1)-x-1) * (scrlinestep<<1) + y * (scrpixelstep << 1);
		else
			d = myDstScreen + x * (scrlinestep<<1) + ((scrwidth>>1)-y) * (scrpixelstep<<1);
	}

	uInt16 col = pal[color];
	uInt32 cold = paldouble[color];

	for (int y2 = 0; y2 < h; y2++)
	{
		const uInt16 buffer = *tmp++;
		if (devres != VGA)
		{
			uInt16 mask = 0x8000;
			uInt8 *tmp = d;
			for (int x2 = 0; x2 < w; x2++, mask >>= 1)
			{
				if (buffer & mask)
					*((uInt16 *)d) = col;
				if (!displaymode && islandscape)
					d += scrpixelstep;
				else if (displaymode != 2)
					d -= scrlinestep;
				else
					d += scrlinestep;
			}
			if (!displaymode && islandscape)
				d = tmp + scrlinestep;
			else if (displaymode != 2)
				d = tmp + scrpixelstep;
			else
				d = tmp - scrpixelstep;
		}
		else
		{
			uInt16 mask = 0x8000;
			uInt8 *tmp = d;
			for (int x2 = 0; x2 < w; x2++, mask >>= 1)
			{
				if (buffer & mask)
				{
					*((uInt32 *)d) = cold;
					*((uInt32 *)(d+scrlinestep)) = cold;
				}
				if (displaymode != 2)
					d -= (scrlinestep<<1);
				else
					d += (scrlinestep<<1);
			}
			if (displaymode != 2)
				d = tmp + (scrpixelstep<<1);
			else
				d = tmp - (scrpixelstep<<1);
		}
	}
}

void FrameBufferWinCE::hLine(uInt32 x, uInt32 y, uInt32 x2, int color)
{
	if (devres != VGA)
		if (!displaymode && islandscape)
			PlothLine(x, y, x2, color);
		else if (displaymode != 2)
			PlotvLine(y, scrheight-x, scrheight-x2, color);
		else
			PlotvLine(scrwidth-y, x, x2, color);
	else
		if (displaymode != 2)
		{
			PlotvLine((y<<1), (((scrheight>>1)-x)<<1)-1, (((scrheight>>1)-x2)<<1)-1, color);
			PlotvLine((y<<1)+1, (((scrheight>>1)-x)<<1)-1, (((scrheight>>1)-x2)<<1)-1, color);
		}
		else
		{
			PlotvLine(((scrwidth>>1)-y)<<1, x<<1, x2<<1, color);
			PlotvLine((((scrwidth>>1)-y)<<1)+1, x<<1, x2<<1, color);
		}
}

void FrameBufferWinCE::PlothLine(uInt32 x, uInt32 y, uInt32 x2, int color)
{
	if (!myDstScreen) return;
	if (x>x2) { x2 ^= x; x ^= x2; x2 ^= x;} //lazy swap
	uInt8 *d = myDstScreen + y * scrlinestep + x * scrpixelstep;
	uInt16 col = pal[color];
	for (;x <= x2; x++, *((uInt16 *)d) = col, d += scrpixelstep);
}

void FrameBufferWinCE::vLine(uInt32 x, uInt32 y, uInt32 y2, int color)
{
	if (devres != VGA)
		if (!displaymode && islandscape)
			PlotvLine(x, y, y2, color);
		else if (displaymode != 2)
			PlothLine(y, scrheight-x, y2, color);
		else
			PlothLine(scrwidth-y, x, scrwidth-y2, color);
	else
		if (displaymode != 2)
		{
			PlothLine(y<<1, (((scrheight>>1)-x)<<1)-2, y2<<1, color);
			PlothLine(y<<1, (((scrheight>>1)-x)<<1)-1, y2<<1, color);
		}
		else
		{
			PlothLine(((scrwidth>>1)-y)<<1, x<<1, ((scrwidth>>1)-y2)<<1, color);
			PlothLine(((scrwidth>>1)-y)<<1, (x<<1)+1, ((scrwidth>>1)-y2)<<1, color);
		}
}

void FrameBufferWinCE::PlotvLine(uInt32 x, uInt32 y, uInt32 y2, int color)
{
	if (y>y2) { y2 ^= y; y ^= y2; y2 ^= y;} //lazy swap
	if (!myDstScreen) return;
	uInt8 *d = myDstScreen + y * scrlinestep + x * scrpixelstep;
	uInt16 col = pal[color];
	for (;y <= y2; y++, *((uInt16 *)d) = col, d += scrlinestep);
}

void FrameBufferWinCE::fillRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h, int color)
{
	if (w==0 || h==0) return;
	if (devres != VGA)
	{
		if (!displaymode && islandscape)
		{
			PlotfillRect(x, y, w, h, color);
			return;
		}
		if (x>scrheight) return; if (y>scrwidth) return;
		if (x+w>scrheight) w=scrheight-x; if (y+h>scrwidth) h=scrwidth-y;
		if (displaymode != 2)
			PlotfillRect(y, scrheight-x-w+1, h, w, color);
		else
			PlotfillRect(scrwidth-y-h+1, x, h, w, color);
	}
	else
	{
		if ((int)x>(scrheight>>1)) return; if ((int)y>(scrwidth>>1)) return;
		if ((int)(x+w)>(scrheight>>1)) w=(scrheight>>1)-x; if ((int)(y+h)>(scrwidth>>1)) h=(scrwidth>>1)-y;
		if (displaymode != 2)
			PlotfillRect(y<<1, (((scrheight>>1)-x-w+1)<<1)-2, h<<1, w<<1, color);
		else
			PlotfillRect(((scrwidth>>1)-y-h+1)<<1, x<<1, h<<1, w<<1, color);
	}

}

void FrameBufferWinCE::PlotfillRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h, int color)
{
	if (!myDstScreen) return;
	uInt8 *d = myDstScreen + y * scrlinestep + x * scrpixelstep;
	uInt16 col = pal[color];
	uInt32 stride = (scrwidth - w) * scrpixelstep;
	for (;h != 0; h--, d += stride)
		for (int w2=w; w2>0; w2--, *((uInt16 *)d) = col, d += scrpixelstep);
}

void FrameBufferWinCE::drawBitmap(uInt32* bitmap, Int32 x, Int32 y, int color, Int32 h)
{
	uInt8 *d;
	uInt16 col;
	uInt32 cold;

	if (!myDstScreen) return;

	if (devres != VGA)
		if (!displaymode && islandscape)
			d = myDstScreen + y * scrlinestep + x * scrpixelstep;
		else if (displaymode != 2)
			d = myDstScreen + (scrheight-x) * scrlinestep + y * scrpixelstep;
		else
			d = myDstScreen + x * scrlinestep + (scrwidth-y) * scrpixelstep;
	else
		if (displaymode != 2)
			d = myDstScreen + ((scrheight>>1)-x-1) * (scrlinestep<<1) + y * (scrpixelstep<<1);
		else
			d = myDstScreen + x * (scrlinestep<<1) + ((scrwidth>>1)-y) * (scrpixelstep<<1);

	col = pal[color];
	cold = paldouble[color];
	for (int i = 0; i < h; i++)
	{
		uInt32 mask = 0xF0000000;
		uInt8 *tmp = d;
		
		for (int j = 0; j < 8; j++, mask >>= 4)
		{
			if(bitmap[i] & mask)
			{
				if (devres != VGA)
					*((uInt16 *)d) = col;
				else
				{
					*((uInt32 *)d) = cold;
					*((uInt32 *)(d+scrlinestep)) = cold;
				}
			}
			if (devres != VGA)
			{
				if (!displaymode && islandscape)
					d += scrpixelstep;
				else if (displaymode != 2)
					d -= scrlinestep;
				else
					d += scrlinestep;
			}
			else
			{
				if (displaymode != 2)
					d -= (scrlinestep<<1);
				else
					d += (scrlinestep<<1);
			}
		}

		if (devres != VGA)
		{
			if (!displaymode && islandscape)
				d = tmp + scrlinestep;
			else if (displaymode != 2)
				d = tmp + scrpixelstep;
			else
				d = tmp - scrpixelstep;
		}
		else
		{
			if (displaymode != 2)
				d = tmp + (scrpixelstep<<1);
			else
				d = tmp - (scrpixelstep<<1);
		}
	}
}

void FrameBufferWinCE::translateCoords(Int32* x, Int32* y)
{
	if ((displaymode == 1) || (displaymode==0 && !islandscape && myOSystem->eventHandler().state() != EventHandler::S_EMULATE))
	{
		Int32 x2 = *x;
		*x = scrheight - *y;
		*y = x2;
	}
	else if (displaymode == 2)
	{
		Int32 x2 = *x;
		*x = *y;
		*y = scrwidth - x2;
	}
	if (devres == VGA)
	{
		*x >>= 1;
		*y >>= 1;
	}

	return;
}

void FrameBufferWinCE::addDirtyRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h)
{
	static bool initflag = false;

	if (myOSystem->eventHandler().state() == EventHandler::S_MENU)
		initflag = true;

	if (myOSystem->eventHandler().state() == EventHandler::S_EMULATE && initflag)
		theRedrawTIAIndicator = true;		// TODO: optimize here

	return;
}

string FrameBufferWinCE::about()
{
	string id = "Video rendering: ";
	id += (issmartphone ? "SM " : "PPC ");
	id += (legacygapi ? "GAPI " : "Direct ");
	id += (devres == SM_LOW ? "176x220\n" : (devres == QVGA ? "240x320\n" : "480x640\n"));
	return id;
}