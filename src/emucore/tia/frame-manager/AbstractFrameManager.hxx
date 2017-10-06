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
// Copyright (c) 1995-2017 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef TIA_ABSTRACT_FRAME_MANAGER
#define TIA_ABSTRACT_FRAME_MANAGER

#include <functional>

#include "Serializable.hxx"
#include "FrameLayout.hxx"

class AbstractFrameManager : public Serializable
{
  public:

      using callback = std::function<void()>;

  public:

    AbstractFrameManager();

  public:

    void setHandlers(
      callback frameStartCallback,
      callback frameCompletionCallback,
      callback renderingStartCallback
    );

    void reset();

    void nextLine();

    void setVblank(bool vblank);

    void setVsync(bool vsync);

    bool isRendering() const { return myIsRendering; }

    bool vsync() const { return myVsync; }

    bool vblank() const { return myVblank; }

    uInt32 scanlinesLastFrame() const { return myCurrentFrameFinalLines; }

    bool scanlineCountTransitioned() const {
      return (myPreviousFrameFinalLines & 0x1) != (myCurrentFrameFinalLines & 0x1);
    }

    uInt32 frameCount() const { return myTotalFrames; }

    FrameLayout layout() const { return myLayout; };

    float frameRate() const { return myFrameRate; }

    bool save(Serializer& out) const override;

    bool load(Serializer& in) override;

  public:

    virtual void setJitterFactor(uInt8 factor) {}

    virtual bool jitterEnabled() const { return false; }

    virtual void enableJitter(bool enabled) {};

  public:

    virtual uInt32 height() const = 0;

    virtual void setFixedHeight(uInt32 height) = 0;

    virtual uInt32 getY() const = 0;

    virtual uInt32 scanlines() const = 0;

    virtual Int32 missingScanlines() const = 0;

    virtual void setYstart(uInt32 ystart) = 0;

    virtual uInt32 ystart() const = 0;

    // TODO: this looks pretty weird --- does this actually work?
    virtual bool ystartIsAuto(uInt32 line) const = 0;

    // TODO: this has to go
    virtual void autodetectLayout(bool toggle) = 0;

    // TODO: this collides with layout(...). Refactor after all is done.
    virtual void setLayout(FrameLayout mode) = 0;

  protected:

    virtual void onSetVblank() {}

    virtual void onSetVsync() {}

    virtual void onNextLine() {}

    virtual void onReset() {}

    virtual void onLayoutChange() {}

    virtual bool onSave(Serializer& out) const { throw runtime_error("cannot be serialized"); }

    virtual bool onLoad(Serializer& in) { throw runtime_error("cannot be serialized"); }

  protected:

    void notifyFrameStart();

    void notifyFrameComplete();

    void notifyRenderingStart();

    void layout(FrameLayout layout);

  protected:

    bool myIsRendering;

    bool myVsync;

    bool myVblank;

    uInt32 myCurrentFrameTotalLines;

    uInt32 myCurrentFrameFinalLines;

    uInt32 myPreviousFrameFinalLines;

    uInt32 myTotalFrames;

    float myFrameRate;

  private:

    FrameLayout myLayout;

    callback myOnFrameStart;
    callback myOnFrameComplete;
    callback myOnRenderingStart;

  private:

    AbstractFrameManager(const AbstractFrameManager&) = delete;
    AbstractFrameManager(AbstractFrameManager&&) = delete;
    AbstractFrameManager& operator=(const AbstractFrameManager&);
    AbstractFrameManager& operator=(AbstractFrameManager&&);

};

#endif // TIA_ABSTRACT_FRAME_MANAGER
