#include <windows.h>
#include "FrameBufferWinCE.hxx"
#include "Console.hxx"
#include "OSystem.hxx"
#include "Font.hxx"

#define OPTPIXAVERAGE(pix1,pix2) ( ((((pix1 & optgreenmaskN) + (pix2 & optgreenmaskN)) >> 1) & optgreenmaskN) | ((((pix1 & optgreenmask) + (pix2 & optgreenmask)) >> 1) & optgreenmask) )

FrameBufferWinCE::FrameBufferWinCE(OSystem *osystem)
: FrameBuffer(osystem), myDstScreen(NULL), SubsystemInited(false), displacement(0),
  issmartphone(true), islandscape(false), displaymode(0)
{
}

FrameBufferWinCE::~FrameBufferWinCE()
{
}

void FrameBufferWinCE::setPalette(const uInt32* palette)
{
	//setup palette
	gxdp = GXGetDisplayProperties();
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
	gxdp = GXGetDisplayProperties();
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
	if (scrwidth == 176 && scrheight == 220)
		issmartphone = true;
	else
		issmartphone = false;
	islandscape = false;
	setmode(0);

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

int FrameBufferWinCE::rotatedisplay(void)
{
	displaymode = (displaymode + 1) % 3;
	setmode(displaymode);
	wipescreen();
	return displaymode;
}

void FrameBufferWinCE::lateinit(void)
{
	int w;
	myWidth = myOSystem->console().mediaSource().width();
	myHeight = myOSystem->console().mediaSource().height();

	if (issmartphone)
		if (!islandscape)
			w = myWidth;
		else
			w = (int) ((float) myWidth * 11.0 / 8.0 + 0.5);
	else
		if (!islandscape)
			w = (int) ((float) myWidth * 3.0 / 2.0 + 0.5);
		else
			w = myWidth * 2;

	switch (displaymode)
	{
		case 0:
			if (scrwidth > w)
				displacement = (scrwidth - w) / 2 * gxdp.cbxPitch;
			else
				displacement = 0;
			if (scrheight > myHeight)
			{
				displacement += (scrheight - myHeight) / 2 * gxdp.cbyPitch;
				minydim = myHeight;
			}
			else
				minydim = scrheight;
			break;

		case 1:
			displacement = gxdp.cbyPitch*(gxdp.cyHeight-1);
			if (scrwidth > myHeight)
			{
				minydim = myHeight;
				displacement += (scrwidth - myHeight) / 2 * gxdp.cbxPitch;
			}
			else
				minydim = scrwidth;
			if (scrheight > w)
				displacement -= (scrheight - w) / 2 * gxdp.cbyPitch;
			break;

		case 2:
			displacement = gxdp.cbxPitch*(gxdp.cxWidth-1);
			if (scrwidth > myHeight)
			{
				minydim = myHeight;
				displacement -= (scrwidth - myHeight) / 2 * gxdp.cbxPitch;
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
	myDstScreen = (uInt8 *) GXBeginDraw();
}

void FrameBufferWinCE::drawMediaSource()
{
	// TODO: define these static, also x & y
	uInt8 *sc, *sp;
	uInt8 *d, *pl;
	uInt16 pix1, pix2;

	if (!SubsystemInited)
		lateinit();
	
	if ( (d = myDstScreen) == NULL )
		return;

	d += displacement;
	pl = d;
	sc = myOSystem->console().mediaSource().currentFrameBuffer();
	sp = myOSystem->console().mediaSource().previousFrameBuffer();
	
	if (issmartphone && islandscape == 0)
	{
		// straight
		for (uInt16 y=0; y<myHeight; y++)
		{
			for (uInt16 x=0; x<(myWidth>>2); x++)
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
	else if (issmartphone && islandscape)
	{
		for (uInt16 y=0; y<minydim; y++)
		{
			for (uInt16 x=0; x<(myWidth>>2); x++)
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
				if (++x>=(myWidth>>2)) break;

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
			d = pl + linestep;
			pl = d;
		}
	}
	else if (issmartphone == 0 && islandscape == 0)
	{
		// 3/2
		for (uInt16 y=0; y<myHeight; y++)
		{
			for (uInt16 x=0; x<(myWidth>>2); x++)
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
	else if (issmartphone == 0 && islandscape)
	{
		// 2/1
		for (uInt16 y=0; y<myHeight; y++)
		{
			for (uInt16 x=0; x<(myWidth>>2); x++)
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
					pix1 = pal[*sc++];
					*((uInt16 *)d) = pix2; d += pixelstep;
					*((uInt16 *)d) = OPTPIXAVERAGE(pix1,pix2); d += pixelstep;
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
	uInt8 *d;

	if (!SubsystemInited)
		lateinit();

	if (&(myOSystem->console().mediaSource()) != NULL)
	{
		uInt8 *s = myOSystem->console().mediaSource().currentFrameBuffer();
		memset(s, 0, myWidth*myHeight-1);
	}
	
	if ( (d = (uInt8 *) GXBeginDraw()) == NULL )
		return;
	for (int i=0; i < scrwidth*scrheight; i++, *((uInt16 *)d) = 0, d += 2);
	GXEndDraw();
}

void FrameBufferWinCE::postFrameUpdate()
{
	GXEndDraw();
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

  const Int32 w = myfont->getCharWidth(c) >> 1;
  const Int32 h = myfont->getFontHeight();
  c -= desc.firstchar;
  const uInt16* tmp = desc.bits + (desc.offset ? desc.offset[c] : (c * h));

  if (x<0 || y<0 || (x>>1)+w>scrwidth || y+h>scrheight) return;

  uInt8 *d = myDstScreen + (y/* >> 1*/) * linestep + (x >> 1) * pixelstep;
  uInt32 stride = (scrwidth - w) * pixelstep;
  uInt16 col = guipal[((int) color) - 256];
  uInt16 col2 = (col >> 1) & 0xFFE0;

  for(int y2 = 0; y2 < h; y2++)
  {
    const uInt16 buffer = *tmp++;
    uInt16 mask = 0x8000;

    for(int x2 = 0; x2 < w; x2++, mask >>= 2)
    {
      if (((buffer & mask) != 0) ^ ((buffer & mask>>1) != 0))
		  *((uInt16 *)d) = col2;
	  else if (((buffer & mask) != 0) && ((buffer & mask>>1) != 0))
		  *((uInt16 *)d) = col;
	  d += pixelstep;
    }
	d += stride;
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
	if (!myDstScreen) return;
	int kx = x >> 1; int ky = y/* >> 1*/; int kx2 = x2>> 1;
	
	//if (kx<0 || ky<0 || kx2<0 || kx+kx2>scrwidth || ky>scrheight) return;
	if (kx<0) kx=0; if (ky<0) ky=0; if (ky>scrheight-1) return; if (kx2>scrwidth-1) kx2=scrwidth-1;

	uInt8 *d = myDstScreen + ky * linestep + kx * pixelstep;
	uInt16 col = guipal[((int) color) - 256];
	for (;kx < kx2; kx++, *((uInt16 *)d) = col, d += pixelstep);
}

void FrameBufferWinCE::vLine(uInt32 x, uInt32 y, uInt32 y2, OverlayColor color)
{
	if (!myDstScreen) return;
	int kx = x >> 1; int ky = y/* >> 1*/; int ky2 = y2 /*>> 1*/;

	//if (kx<0 || ky<0 || ky2<0 || ky+ky2>scrheight || kx>scrwidth) return;
	if (kx<0) kx=0; if (ky<0) ky=0; if (kx>scrwidth-1) return; if (ky2>scrheight-1) ky2=scrheight-1;

	uInt8 *d = myDstScreen + ky * linestep + kx * pixelstep;
	uInt16 col = guipal[((int) color) - 256];
	for (;ky < ky2; ky++, *((uInt16 *)d) = col, d += linestep);
}

void FrameBufferWinCE::fillRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h, OverlayColor color)
{
	if (!myDstScreen) return;
	int kx = x >> 1; int ky = y/* >> 1*/; int kw = (w >> 1) - 1; int kh = h /*>> 1*/;

	//if (kx<0 || ky<0 || kw<0 || kh<0 || kx+kw>scrwidth || ky+kh>scrheight) return;
	if (kx<0) kx=0; if (ky<0) ky=0;if (kw<0) kw=0; if (kh<0) kh=0;
	if (kx+kw>scrwidth-1) kw=scrwidth-kx-1; if (ky+kh>scrheight-1) kh=scrheight-ky-1;

	uInt8 *d = myDstScreen + ky * linestep + kx * pixelstep;
	uInt16 col = guipal[((int) color) - 256];
	uInt32 stride = (scrwidth - kw - 1) * pixelstep;
	for (int h2 = kh; h2 >= 0; h2--)
	{
		for (int w2 = kw; w2>=0; w2--)
		{
			*((uInt16 *)d) = col;
			d += pixelstep;
		}
		d += stride;
	}
}

void FrameBufferWinCE::drawBitmap(uInt32* bitmap, Int32 x, Int32 y, OverlayColor color, Int32 h)
{
	return;
}

void FrameBufferWinCE::translateCoords(Int32* x, Int32* y)
{
	return;
}

void FrameBufferWinCE::addDirtyRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h)
{
	return;
}

uInt32 FrameBufferWinCE::lineDim()
{
	return 1;
}


