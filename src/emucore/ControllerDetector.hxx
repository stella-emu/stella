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
// Copyright (c) 1995-2019 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================


#ifndef CONTROLLER_DETECTOR_HXX
#define CONTROLLER_DETECTOR_HXX

class Cartridge;
class Properties;
class OSystem;

class FilesystemNode;

//#include "Bankswitch.hxx"

#include "Control.hxx"
#include "bspf.hxx"

class ControllerDetector
{
  public:
    /**
      Create a new cartridge object allocated on the heap.  The
      type of cartridge created depends on the properties object.

      @param image      A pointer to the ROM image
      @param size       The size of the ROM image
      @param controller The provided left controller type of the ROM image
      @param port       The port to be checked
      @param osystem    The osystem associated with the system
      @return   The detected controller name
    */
    static string detect(const BytePtr& image, uInt32 size,
                         const string& controller, const Controller::Jack port,
                         const OSystem& osystem);

  private:
    static string autodetectPort(const BytePtr& image, uInt32 size, Controller::Jack port, const OSystem& osystem);

    /**
      Search the image for the specified byte signature

      @param image      A pointer to the ROM image
      @param imagesize  The size of the ROM image
      @param signature  The byte sequence to search for
      @param sigsize    The number of bytes in the signature
      @param minhits    The minimum number of times a signature is to be found

      @return  True if the signature was found at least 'minhits' time, else false
    */
    static bool searchForBytes(const uInt8* image, uInt32 imagesize,
                               const uInt8* signature, uInt32 sigsize,
                               uInt32 minhits = 1);

    // Returns true if the port's joystick button access code is found
    static bool usesJoystickButtons(const BytePtr& image, uInt32 size, Controller::Jack port);

    // Returns true if the port's paddle button access code is found
    static bool usesPaddleButtons(const BytePtr& image, uInt32 size, Controller::Jack port, const OSystem& osystem);

    // Returns true if Trak-Ball table is found
    static bool isProbablyTrakBall(const BytePtr& image, uInt32 size);

    // Returns true if Atari Mouse table is found
    static bool isProbablyAtariMouse(const BytePtr& image, uInt32 size);

    // Returns true if Amiga Mouse table is found
    static bool isProbablyAmigaMouse(const BytePtr& image, uInt32 size);

    // Returns true if the AtariVox code pattern is found (TODO)
    static bool isProbablyAtariVox(const BytePtr& image, uInt32 size, Controller::Jack port);

    // Returns true if the SaveKey code pattern is found
    static bool isProbablySaveKey(const BytePtr& image, uInt32 size, Controller::Jack port);

  private:
    // Following constructors and assignment operators not supported
    ControllerDetector() = delete;
    ControllerDetector(const ControllerDetector&) = delete;
    ControllerDetector(ControllerDetector&&) = delete;
    ControllerDetector& operator=(const ControllerDetector&) = delete;
    ControllerDetector& operator=(ControllerDetector&&) = delete;
};

#endif

