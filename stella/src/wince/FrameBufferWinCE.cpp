#include <windows.h>
#include <gx.h>
#include "FrameBufferWinCE.hxx"
#include "Console.hxx"
#include "OSystem.hxx"
#include "Font.hxx"

FrameBufferWinCE::FrameBufferWinCE(OSystem *osystem)
: FrameBuffer(osystem), myDstScreen(NULL), SubsystemInited(false), displacement(0)
{
}

FrameBufferWinCE::~FrameBufferWinCE()
{
}

void FrameBufferWinCE::setPalette(const uInt32* palette)
{
	//setup palette
	GXDisplayProperties gxdp = GXGetDisplayProperties();
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
	GXDisplayProperties gxdp = GXGetDisplayProperties();
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
	//GXDisplayProperties gxdp = GXGetDisplayProperties();
	pixelstep = gxdp.cbxPitch;
	linestep = gxdp.cbyPitch;
	scrwidth = gxdp.cxWidth;
	scrheight = gxdp.cyHeight;
	
	SubsystemInited = false;
	return true;
}

void FrameBufferWinCE::lateinit(void)
{
	myWidth = myOSystem->console().mediaSource().width();
	myHeight = myOSystem->console().mediaSource().height();
	if (scrwidth > myWidth)
		displacement = (scrwidth - myWidth) / 2 * pixelstep;
	else
		displacement = 0;
	if (scrheight > myHeight)
		displacement += (scrheight - myHeight) / 2 * linestep;

	SubsystemInited = true;
}

void FrameBufferWinCE::preFrameUpdate()
{
	myDstScreen = (uInt8 *) GXBeginDraw();
}

void FrameBufferWinCE::drawMediaSource()
{
	uInt8 *sc, *sp;
	uInt8 *d, *pl;

	if (!SubsystemInited)
	{
		lateinit();
		return;
	}
	
	if ( (d = myDstScreen) == NULL )
		return;

	d += displacement;
	pl = d;
	sc = myOSystem->console().mediaSource().currentFrameBuffer();
	sp = myOSystem->console().mediaSource().previousFrameBuffer();
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

void FrameBufferWinCE::postFrameUpdate()
{
	GXEndDraw();
}

void FrameBufferWinCE::drawChar(const GUI::Font* font, uInt8 c, uInt32 x, uInt32 y, OverlayColor color)
{
  GUI::Font* myfont = (GUI::Font*)font;
  const FontDesc& desc = myfont->desc();

  if (!myDstScreen) return;
  //if (!SubsystemInited) {lateinit(false); return;}

  if(c < desc.firstchar || c >= desc.firstchar + desc.size)
  {
    if (c == ' ')
      return;
    c = desc.defaultchar;
  }

  const Int32 w = myfont->getCharWidth(c);
  const Int32 h = myfont->getFontHeight();
  c -= desc.firstchar;
  const uInt16* tmp = desc.bits + (desc.offset ? desc.offset[c] : (c * h));

  uInt8 *d = myDstScreen + (y/* >> 1*/) * linestep + (x >> 1) * pixelstep;
  uInt32 stride = (scrwidth - w) * pixelstep;
  uInt16 col = guipal[((int) color) - 256];

  for(int y2 = 0; y2 < h; y2++)
  {
    const uInt16 buffer = *tmp++;
    uInt16 mask = 0x8000;

    for(int x2 = 0; x2 < w; x2++, mask >>= 1)
    {
      if ((buffer & mask) != 0)
      {
		  *((uInt16 *)d) = col;
      }
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
	//if (!SubsystemInited) {lateinit(false); return;}
	int kx = x >> 1; int ky = y/* >> 1*/; int kx2 = x2>> 1;
	uInt8 *d = myDstScreen + ky * linestep + kx * pixelstep;
	uInt16 col = guipal[((int) color) - 256];
	for (;kx < kx2; kx++)
	{
		*((uInt16 *)d) = col;
		d += pixelstep;
	}
}

void FrameBufferWinCE::vLine(uInt32 x, uInt32 y, uInt32 y2, OverlayColor color)
{
	if (!myDstScreen) return;
	//if (!SubsystemInited) {lateinit(false); return;}
	int kx = x >> 1; int ky = y/* >> 1*/; int ky2 = y2>> 1;
	uInt8 *d = myDstScreen + ky * linestep + kx * pixelstep;
	uInt16 col = guipal[((int) color) - 256];
	for (;ky < ky2; ky++)
	{
		*((uInt16 *)d) = col;
		d += linestep;
	}
}

void FrameBufferWinCE::fillRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h, OverlayColor color)
{
	if (!myDstScreen) return;
	//if (!SubsystemInited) {lateinit(false); return;}
	int kx = x >> 1; int ky = y/* >> 1*/; int kw = (w >> 1) - 1; int kh = h /*>> 1*/;
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
