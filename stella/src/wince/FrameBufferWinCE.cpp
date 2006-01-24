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

#include <windows.h>
#include "FrameBufferWinCE.hxx"
#include "Console.hxx"
#include "OSystem.hxx"
#include "Font.hxx"

#define OPTPIXAVERAGE(pix1,pix2) ( ((((pix1 & optgreenmaskN) + (pix2 & optgreenmaskN)) >> 1) & optgreenmaskN) | ((((pix1 & optgreenmask) + (pix2 & optgreenmask)) >> 1) & optgreenmask) )

FrameBufferWinCE::FrameBufferWinCE(OSystem *osystem)
: FrameBuffer(osystem), myDstScreen(NULL), SubsystemInited(false), displacement(0),
issmartphone(true), islandscape(false), displaymode(0), legacygapi(true), devres(SM_LOW)
{
	gxdp.cxWidth = gxdp.cyHeight = gxdp.cbxPitch = gxdp.cbyPitch = gxdp.cBPP = gxdp.ffFormat = 0;
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
	/*else if (gxdp.cxWidth == 480 && gxdp.cyHeight == 640)
		devres = VGA;*/
	else
		devres = QVGA;
}

void FrameBufferWinCE::setPalette(const uInt32* palette)
{
	//setup palette
	GetDeviceProperties();
	for (uInt16 i=0; i<256; i++)
	{
		uInt8 r = (uInt8) ((palette[i] & 0xFF0000) >> 16);
		uInt8 g = (uInt8) ((palette[i] & 0x00FF00) >> 8);
		uInt8 b = (uInt8) (palette[i] & 0x0000FF);
		if(gxdp.ffFormat & kfDirect565)
			pal[i] = (uInt16) ( ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3) );
		else if(gxdp.ffFormat & kfDirect555)
			pal[i] = (uInt16) ( ((r & 0xF8) << 7) | ((g & 0xF8) << 3) | ((b & 0xF8) >> 3) );
		else
			return;
	}
	SubsystemInited = false;
}

bool FrameBufferWinCE::initSubsystem()
{
	GetDeviceProperties();
	for (int i=0; i<kNumColors - 256; i++)
	{
		uInt8 r = (ourGUIColors[i][0] & 0xFF);
		uInt8 g = (ourGUIColors[i][1] & 0xFF);
		uInt8 b = (ourGUIColors[i][2] & 0xFF);
		if(gxdp.ffFormat & kfDirect565)
			guipal[i] = (uInt16) ( ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3) );
		else if(gxdp.ffFormat & kfDirect555)
			guipal[i] = (uInt16) ( ((r & 0xF8) << 7) | ((g & 0xF8) << 3) | ((b & 0xF8) >> 3) );
		else
			return false;
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

//	if (scrwidth == 176 && scrheight == 220)
//		issmartphone = true;
//	else
//		issmartphone = false;

	setmode(displaymode);
	SubsystemInited = false;
	return true;
}

void FrameBufferWinCE::setmode(uInt8 mode)
{
	displaymode = mode % 3;
	switch (displaymode)
	{
		// portrait
		case 0:
			pixelstep = gxdp.cbxPitch;
			linestep = gxdp.cbyPitch;
			islandscape = false;
			break;

		// landscape
		case 1:
			pixelstep = - gxdp.cbyPitch;
			linestep = gxdp.cbxPitch;
			islandscape = true;
			break;

		// inverted landscape
		case 2:
			pixelstep = gxdp.cbyPitch;
			linestep = - gxdp.cbxPitch;
			islandscape = true;
			break;
	}

	pixelsteptimes5 = pixelstep * 5;
	pixelsteptimes6 = pixelstep * 6;
	SubsystemInited = false;
}

uInt8 FrameBufferWinCE::rotatedisplay(void)
{
	displaymode = (displaymode + 1) % 3;
	setmode(displaymode);
	wipescreen();
	return displaymode;
}

void FrameBufferWinCE::lateinit(void)
{
	int w,h;

	myWidth = myOSystem->console().mediaSource().width();
	myHeight = myOSystem->console().mediaSource().height();
	myWidthdiv4 = myWidth >> 2;

	if (devres == SM_LOW)
		if (!islandscape)
			w = myWidth;
		else
			w = (int) ((float) myWidth * 11.0f / 8.0f + 0.5f);
	else //if (devres == QVGA)
		if (!islandscape)
			w = (int) ((float) myWidth * 3.0f / 2.0f + 0.5f);
		else
			w = myWidth * 2;
	/*else
	{
		// VGA
	}
	*/
	if (devres == SM_LOW && islandscape)
		h = (int) ((float) myHeight * 4.0f / 5.0f + 0.5f);
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

	SubsystemInited = true;
}

void FrameBufferWinCE::preFrameUpdate()
{
	static HDC hdc;
	static RawFrameBufferInfo rfbi;

	if (legacygapi)
		myDstScreen = (uInt8 *) GXBeginDraw();
	else
	{
		hdc = GetDC(NULL);
		ExtEscape(hdc, GETRAWFRAMEBUFFER, 0, NULL, sizeof(RawFrameBufferInfo), (char *) &rfbi);
		ReleaseDC(NULL, hdc);
		myDstScreen = (uInt8 *) rfbi.pFramePointer;
	}
}

void FrameBufferWinCE::drawMediaSource()
{
	static uInt8 *sc, *sp, *sc_n, *sp_n;
	static uInt8 *d, *pl, *p;
	static uInt16 pix1, pix2, pix3, x, y;

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
		p = sp;
		for (uInt16 y=0; y<myHeight; y++)
			for (uInt16 x=0; x<myWidth; x += 4, *p = *p + 1, p += 4);
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
	else if (devres == QVGA && !islandscape)
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
					d += (pixelstep << 3);
				}
				sp += 4;
			}
			d = pl + linestep;
			pl = d;
		}
	}

}

void FrameBufferWinCE::wipescreen(void)
{
	if (!SubsystemInited)
		lateinit();

	uInt8 *s = myOSystem->console().mediaSource().currentFrameBuffer();
	memset(s, 0, myWidth*myHeight-1);
	s = myOSystem->console().mediaSource().previousFrameBuffer();
	memset(s, 0, myWidth*myHeight-1);

	preFrameUpdate();
	//uInt8 *d;
	//d=myDstScreen;
	//for (int i=0; i < scrwidth*scrheight; i++, *((uInt16 *)d) = 0, d += scrpixelstep);
	memset(myDstScreen, 0, scrwidth*scrheight*2);
	postFrameUpdate();
}

void FrameBufferWinCE::postFrameUpdate()
{
	if (legacygapi) GXEndDraw();
}

void FrameBufferWinCE::drawChar(const GUI::Font* font, uInt8 c, uInt32 x, uInt32 y, OverlayColor color)
{
  GUI::Font* myfont = (GUI::Font*)font;
  const FontDesc& desc = myfont->desc();

  if (!myDstScreen) return;

  if(c < desc.firstchar || c >= desc.firstchar + desc.size)
  {
    if (c == ' ')
      return;
    c = desc.defaultchar;
  }

  Int32 w = myfont->getCharWidth(c);
  const Int32 h = myfont->getFontHeight();
  c -= desc.firstchar;
  const uInt16* tmp = desc.bits + (desc.offset ? desc.offset[c] : (c * h));

  if (x<0 || y<0 || (x>>1)+w>scrwidth || y+h>scrheight) return;

  uInt8 *d;
  uInt32 stride;
  if (devres == SM_LOW)
  {
	  d = myDstScreen + y * scrlinestep + ((x+1) >> 1) * scrpixelstep;
	  stride = (scrwidth - w) * scrpixelstep;
  }
  else
  {
	  if (displaymode != 2)
		d = myDstScreen + (scrheight-x-1) * scrlinestep + (y-1) * scrpixelstep;
	  else
		d = myDstScreen + x * scrlinestep + (scrwidth-y-1) * scrpixelstep;
  }

  uInt16 col = guipal[((int) color) - 256];

  for(int y2 = 0; y2 < h; y2++)
  {
    const uInt16 buffer = *tmp++;
	if (devres == SM_LOW)
	{
		uInt16 mask = 0xC000;
		for(int x2 = 0; x2 < w; x2++, mask >>= 2)
		{
		  if (buffer & mask)
			  *((uInt16 *)d) = col;
		  d += scrpixelstep;
		}
		d += stride;
	}
	else
	{
		uInt16 mask = 0x8000;
		uInt8 *tmp = d;
		for(int x2 = 0; x2 < w; x2++, mask >>= 1)
		{
		  if (buffer & mask)
			  *((uInt16 *)d) = col;
		  if (displaymode != 2)
			d -= scrlinestep;
		  else
			d += scrlinestep;
		}
		if (displaymode != 2)
			d = tmp + scrpixelstep;
		else
			d = tmp - scrpixelstep;
	}
  }
}

void FrameBufferWinCE::scanline(uInt32 row, uInt8* data)
{
	return;
}

void FrameBufferWinCE::setAspectRatio()
{
	return;
}

bool FrameBufferWinCE::createScreen()
{
	return true;
}

void FrameBufferWinCE::toggleFilter()
{
	return;
}

uInt32 FrameBufferWinCE::mapRGB(Uint8 r, Uint8 g, Uint8 b)
{
	return 0xFFFFFFFF;
}

void FrameBufferWinCE::hLine(uInt32 x, uInt32 y, uInt32 x2, OverlayColor color)
{
	if (devres == SM_LOW)
	{
		int kx = x >> 1; int ky = y; int kx2 = x2>> 1;
		if (kx<0) kx=0; if (ky<0) ky=0; if (ky>scrheight-1) return; if (kx2>scrwidth-1) kx2=scrwidth-1;
		PlothLine(kx, ky, kx2, color);
	}
	else
		if (displaymode != 2)
			PlotvLine(y, scrheight-x, scrheight-x2-1, color);
		else
			PlotvLine(scrwidth-y-1, x, x2+1, color);
}

void FrameBufferWinCE::PlothLine(uInt32 x, uInt32 y, uInt32 x2, OverlayColor color)
{
	if (!myDstScreen) return;
	if (x>x2) { x2 ^= x; x ^= x2; x2 ^= x;} //lazy swap
	uInt8 *d = myDstScreen + y * scrlinestep + x * scrpixelstep;
	uInt16 col = guipal[((int) color) - 256];
	for (;x < x2; x++, *((uInt16 *)d) = col, d += scrpixelstep);
}

void FrameBufferWinCE::vLine(uInt32 x, uInt32 y, uInt32 y2, OverlayColor color)
{
	if (devres == SM_LOW)
	{
		int kx = x >> 1; int ky = y; int ky2 = y2;
		if (kx<0) kx=0; if (ky<0) ky=0; if (kx>scrwidth-1) return; if (ky>scrheight-1) ky=scrheight-1; if (ky2>scrheight-1) ky2=scrheight-1;
		PlotvLine(kx, ky, ky2, color);
	}
	else
		if (displaymode != 2)
			PlothLine(y, scrheight-x-1, y2, color);
		else
			PlothLine(scrwidth-y, x, scrwidth-y2, color);

}

void FrameBufferWinCE::PlotvLine(uInt32 x, uInt32 y, uInt32 y2, OverlayColor color)
{
	if (y>y2) { y2 ^= y; y ^= y2; y2 ^= y;} //lazy swap
	if (!myDstScreen) return;
	uInt8 *d = myDstScreen + y * scrlinestep + x * scrpixelstep;
	uInt16 col = guipal[((int) color) - 256];
	for (;y < y2; y++, *((uInt16 *)d) = col, d += scrlinestep);
}

void FrameBufferWinCE::fillRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h, OverlayColor color)
{
	if (devres == SM_LOW)
	{
		int kx = x >> 1; int ky = y; int kw = (w >> 1); int kh = h;
		if (ky>scrheight-1) return; if (kx>scrwidth-1) return;
		if (kx<0) kx=0; if (ky<0) ky=0;if (kw<0) kw=0; if (kh<0) kh=0;
		if (kx+kw>scrwidth-1) kw=scrwidth-kx-1; if (ky+kh>scrheight-1) kh=scrheight-ky-1;
		PlotfillRect(kx, ky, kw, kh, color);
	}
	else
	{
		if (x>scrheight) return; if (y>scrwidth) return;
		if (x+w>scrheight) w=scrheight-x; if (y+h>scrwidth) h=scrwidth-y;
		if (displaymode != 2)
			PlotfillRect(y, scrheight-x-w, h-1, w-1, color);
		else
			PlotfillRect(scrwidth-y-h, x, h-1, w-1, color);
	}

}

void FrameBufferWinCE::PlotfillRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h, OverlayColor color)
{
	if (!myDstScreen) return;
	uInt8 *d = myDstScreen + y * scrlinestep + x * scrpixelstep;
	uInt16 col = guipal[((int) color) - 256];
	uInt32 stride = (scrwidth - w - 1) * scrpixelstep;
	for (h++; h != 0; h--, d += stride)
		for (int w2=w; w2>=0; w2--, *((uInt16 *)d) = col, d += scrpixelstep);
}

void FrameBufferWinCE::drawBitmap(uInt32* bitmap, Int32 x, Int32 y, OverlayColor color, Int32 h)
{
	return;
}

void FrameBufferWinCE::translateCoords(Int32* x, Int32* y)
{
	if (!issmartphone)
	{
		if ((displaymode == 1) || (displaymode==0 && myOSystem->eventHandler().state() != 1))
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
	}
	return;
}

void FrameBufferWinCE::addDirtyRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h)
{
	static bool initflag = false;

	if (myOSystem->eventHandler().state() == 3)
		initflag = true;

	if (myOSystem->eventHandler().state()==1 && initflag)
	{
		// TODO: optimize here
		theRedrawTIAIndicator = true;
	}

	return;
}

uInt32 FrameBufferWinCE::lineDim()
{
	return 1;
}


