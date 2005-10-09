#ifndef FRAMEBUFFER_WINCE_HXX
#define FRAMEBUFFER_WINCE_HXX

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
	
	private:

	void FrameBufferWinCE::lateinit(void);

//	uInt32 myXstart, myYstart;
//	uInt8 *currentFrame, *previousFrame;
	uInt16 pal[256], myWidth, myHeight, guipal[kNumColors-256], scrwidth, scrheight;
	uInt32 pixelstep, linestep, displacement;
	bool SubsystemInited;
	uInt8 *myDstScreen;
};

#endif
