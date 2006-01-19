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
// $Id: FrameBufferPSP.hxx,v 1.2 2006-01-19 00:45:13 stephena Exp $
//============================================================================

#ifndef FRAMEBUFFER_PSP_HXX
#define FRAMEBUFFER_PSP_HXX



#include "Font.hxx"
#include "bspf.hxx"
#include "GuiUtils.hxx"
#include "FrameBufferSoft.hxx"


/**
  This class implements an SDL software framebuffer.

  @author  Stephen Anthony
  @version $Id: FrameBufferPSP.hxx,v 1.2 2006-01-19 00:45:13 stephena Exp $
*/
class FrameBufferPSP : public FrameBufferSoft
{
  public:
    /**
      Creates a new software framebuffer
    */
    FrameBufferPSP(OSystem* osystem);

    /**
      Destructor
    */
    virtual ~FrameBufferPSP();

    //////////////////////////////////////////////////////////////////////
    // The following methods are derived from FrameBuffer.hxx
    //////////////////////////////////////////////////////////////////////
    /**
    This method is called to initialize software video mode.
    Return false if any operation fails, otherwise return true.
     */
    virtual bool initSubsystem();

    /**
      This method is called to query the type of the FrameBuffer.
    */
    virtual BufferType type() { return kSoftBuffer; }

    /**
    This method is called whenever the screen needs to be recreated.
    It updates the global screen variable.
     */
    virtual bool createScreen();


};


#endif
