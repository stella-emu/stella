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

#ifndef TIA_VBLANK_MANAGER
#define TIA_VBLANK_MANAGER

#include "Serializable.hxx"

class VblankManager : public Serializable
{
  public:

    VblankManager();

  public:

    void reset();

    void start();

    bool nextLine(bool isGarbageFrame);

    void setVblankLines(uInt32 lines) { myVblankLines = lines; }

    void setYstart(uInt32 ystart);

    uInt32 ystart() const { return myYstart == 0 ? myLastVblankLines : myYstart; }
    bool ystartIsAuto(uInt32 line) const { return myLastVblankLines == line; }

    void setVblank(bool vblank) { myVblank = vblank; }

    bool setVblankDuringVblank(bool vblank, bool isGarbageFrame);

    bool vblank() const { return myVblank; }

    uInt32 currentLine() const { return myCurrentLine; };

    void setJitter(Int32 jitter);
    void setJitterFactor(uInt8 jitterFactor) { myJitterFactor = jitterFactor; }

    bool isStable() const { return myMode != VblankMode::floating; }

    /**
      Serializable methods (see that class for more information).
    */
    bool save(Serializer& out) const override;
    bool load(Serializer& in) override;
    string name() const override { return "TIA_VblankManager"; }

  private:

    enum VblankMode {
      locked,
      floating,
      final,
      fixed
    };

  private:

    bool shouldTransition(bool isGarbageFrame);

    void setVblankMode(VblankMode mode);

  private:

    uInt32 myVblankLines;
    uInt32 myYstart;
    bool myVblank;
    uInt32 myCurrentLine;

    VblankMode myMode;
    uInt32 myLastVblankLines;
    uInt8 myVblankViolations;
    uInt8 myStableVblankFrames;
    bool myVblankViolated;
    uInt8 myFramesInLockedMode;

    Int32 myJitter;
    uInt8 myJitterFactor;

    bool myIsRunning;

  private:

    VblankManager(const VblankManager&) = delete;
    VblankManager(VblankManager&&) = delete;
    VblankManager& operator=(const VblankManager&) = delete;
    VblankManager& operator=(VblankManager&&) = delete;

};

#endif // TIA_VBLANK_MANAGER
