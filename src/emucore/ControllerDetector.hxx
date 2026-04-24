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
// Copyright (c) 1995-2026 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef CONTROLLER_DETECTOR_HXX
#define CONTROLLER_DETECTOR_HXX

class Settings;

#include "Control.hxx"

/**
  Auto-detect controller type by matching determining pattern.

  @author  Thomas Jentzsch
*/

class ControllerDetector
{
  public:
    /**
      Detects the controller type at the given port if no controller is provided.

      @param image       A reference to the ROM image
      @param size        The size of the ROM image
      @param type        The provided controller type of the ROM image
      @param port        The port to be checked
      @param settings    A reference to the various settings (read-only)
      @param isQuadTari  If true, try to detect the QuadTari's controllers
      @return   The detected controller type
    */
    static Controller::Type detectType(const ByteBuffer& image, size_t size,
        Controller::Type type, Controller::Jack port,
        const Settings& settings, bool isQuadTari = false);

    /**
      Detects the controller type at the given port if no controller is provided
      and returns its name.

      @param image       A reference to the ROM image
      @param size        The size of the ROM image
      @param type        The provided controller type of the ROM image
      @param port        The port to be checked
      @param settings    A reference to the various settings (read-only)
      @param isQuadTari  If true, try to detect the QuadTari's controllers

      @return   The (detected) controller name
    */
    static string detectName(const ByteBuffer& image, size_t size,
        Controller::Type type, Controller::Jack port,
        const Settings& settings, bool isQuadTari = false);

  private:
    /**
      Detects the controller type at the given port.

      @param image       A reference to the ROM image
      @param size        The size of the ROM image
      @param port        The port to be checked
      @param settings    A reference to the various settings (read-only)
      @param isQuadTari  If true, try to detect the QuadTari's controllers

      @return   The detected controller type
    */
    static Controller::Type autodetectPort(ByteSpan image,
        Controller::Jack port, const Settings& settings, bool isQuadTari);

    // Returns true if the port's joystick button access code is found.
    static bool usesJoystickButton(ByteSpan image, Controller::Jack port);

    // Returns true if joystick direction access code is found.
    static bool usesJoystickDirections(ByteSpan image);

    // Returns true if the port's keyboard access code is found.
    static bool usesKeyboard(ByteSpan image, Controller::Jack port);

    // Returns true if the port's 2nd Genesis button access code is found.
    static bool usesGenesisButton(ByteSpan image, Controller::Jack port);

    // Returns true if the port's paddle button access code is found.
    static bool usesPaddle(ByteSpan image, Controller::Jack port,
                           const Settings& settings);

    // Returns true if a Trak-Ball table is found.
    static bool isProbablyTrakBall(ByteSpan image);

    // Returns true if an Atari Mouse table is found.
    static bool isProbablyAtariMouse(ByteSpan image);

    // Returns true if an Amiga Mouse table is found.
    static bool isProbablyAmigaMouse(ByteSpan image);

    // Returns true if a SaveKey code pattern is found.
    static bool isProbablySaveKey(ByteSpan image, Controller::Jack port);

    // Returns true if a Lightgun code pattern is found
    static bool isProbablyLightGun(ByteSpan image, Controller::Jack port);

    // Returns true if a QuadTari code pattern is found.
    static bool isProbablyQuadTari(ByteSpan image, Controller::Jack port);

    // Returns true if a Kid Vid code pattern is found.
    static bool isProbablyKidVid(ByteSpan image, Controller::Jack port);

  private:
    // Following constructors and assignment operators not supported
    ControllerDetector() = delete;
    ~ControllerDetector() = delete;
    ControllerDetector(const ControllerDetector&) = delete;
    ControllerDetector(ControllerDetector&&) = delete;
    ControllerDetector& operator=(const ControllerDetector&) = delete;
    ControllerDetector& operator=(ControllerDetector&&) = delete;
};

#endif  // CONTROLLER_DETECTOR_HXX
