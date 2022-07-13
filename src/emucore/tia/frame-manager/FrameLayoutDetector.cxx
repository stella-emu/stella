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
// Copyright (c) 1995-2022 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "EventHandler.hxx"
#include "Logger.hxx"
#include "M6532.hxx"

#include "FrameLayoutDetector.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameLayoutDetector::simulateInput(M6532& riot, EventHandler& eventHandler, bool pressed) const
{
  // Console
  eventHandler.handleEvent(Event::ConsoleSelect, pressed);
  eventHandler.handleEvent(Event::ConsoleReset, pressed);
  // Various controller types
  eventHandler.handleEvent(Event::LeftJoystickFire, pressed);
  eventHandler.handleEvent(Event::RightJoystickFire, pressed);
  // Required for Console::redetectFrameLayout
  eventHandler.handleEvent(Event::LeftPaddleAFire, pressed);
  eventHandler.handleEvent(Event::LeftPaddleBFire, pressed);
  eventHandler.handleEvent(Event::RightPaddleAFire, pressed);
  eventHandler.handleEvent(Event::RightPaddleBFire, pressed);
  eventHandler.handleEvent(Event::LeftDrivingFire, pressed);
  eventHandler.handleEvent(Event::RightDrivingFire, pressed);
  riot.update();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameLayout FrameLayoutDetector::detectedLayout(bool detectPal60, bool detectNtsc50, const string& name) const
{
#if 0 // debug
  cerr << endl << name << endl;
  int i = 0;
  for(auto count : myColorCount)
  {
    if(i % 8 == 0)
      cerr << std::uppercase << std::setw(2) << std::hex << (i >> 3) << "x: ";
    cerr << std::setw(6) << std::dec << count;
    if(++i % 8 == 0)
      cerr << endl;
    else
      cerr << ", ";
  }
  cerr << endl;
#endif
#if 0 // save sampled color values
  std::ofstream file;

  file.open("d:/Users/Thomas/Documents/Atari/Games/test/autodetect/colors.csv", std::ios::app);
  if(file.is_open())
  {
    file << name;
    for(auto count : myColorCount)
      file << "; " << count;
    file << "\n";
    file.close();
  }
#endif

  // Multiply each hue's count with its NTSC and PAL stats and aggregate results
  // If NTSC/PAL results differ significantly, overrule frame result
  FrameLayout layout = myPalFrames > myNtscFrames ? FrameLayout::pal : FrameLayout::ntsc;

  constexpr std::array<double, NUM_HUES> ntscColorFactor{
    0.00000, 0.05683, 0.06220, 0.05505, 0.06162, 0.02874, 0.03532, 0.03716,
    0.15568, 0.06471, 0.02886, 0.03224, 0.06903, 0.11478, 0.02632, 0.01675
  }; // ignore black = 0x00!
  constexpr std::array<double, NUM_HUES> palColorFactor{
    0.00000, 0.00450, 0.09962, 0.07603, 0.06978, 0.13023, 0.09638, 0.02268,
    0.02871, 0.04700, 0.02950, 0.11974, 0.03474, 0.08025, 0.00642, 0.00167
  }; // ignore black = 0x00!
  // Calculation weights and params (optimum based on sampled ROMs optimized for PAL-60)
  constexpr double POWER_FACTOR = 0.17; // Level the color counts (large values become less relevant)
  constexpr uInt32 SUM_DIV = 20;        // Skip too small counts
  constexpr uInt32 MIN_VALID = 3;       // Minimum number of different hues with significant counts
  constexpr double SUM_FACTOR = 2.0;    // Minimum sum difference which triggers a layout change

  double ntscSum{0}, palSum{0};
  std::array<double, NUM_HUES> hueSum{0};
  double totalHueSum = 0;
  uInt32 validHues = 0;

  // Aggregate hues
  for(int hue = 0; hue < NUM_HUES; ++hue)
  {
    for(int lum = 0; lum < NUM_LUMS; ++lum)
      if(hue || lum) // skip 0x00
        hueSum[hue] += myColorCount[hue * NUM_LUMS + lum];
    hueSum[hue] = std::pow(hueSum[hue], POWER_FACTOR);
    totalHueSum += hueSum[hue];
  }
  // Calculate hue sums
  for(int hue = 0; hue < NUM_HUES; ++hue)
  {
    if(hueSum[hue] > totalHueSum / SUM_DIV)
      validHues++;
    ntscSum += hueSum[hue] * ntscColorFactor[hue];
    palSum += hueSum[hue] * palColorFactor[hue];
  }
  // Correct layout if enough valid hues and significant sum difference
  // TODO: Use fractional scanline counts for intermediate values around 285 scanlines, e.g.
  // Desert Falcon, Dumbo's Flying Circus, Dungeon, Firefox, Millipede, Popeye, RS Basketball, Star Trek, Stunt Cycle
  if(validHues >= MIN_VALID)
  {
    if(detectPal60 && layout == FrameLayout::ntsc && ntscSum * SUM_FACTOR < palSum)
    {
      layout = FrameLayout::pal60;
      Logger::debug("Changed layout from NTSC into PAL-60");
    }
    // Note: three false positives (Berzerk, Canyon Bomber, Jawbreaker) for NTSC-50 after
    //  optimizing for PAL-60
    else if(detectNtsc50 && layout == FrameLayout::pal && palSum * SUM_FACTOR < ntscSum)
    {
      layout = FrameLayout::ntsc50;
      Logger::debug("Changed layout from PAL into NTSC-50");
    }
  }
  return layout;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameLayoutDetector::FrameLayoutDetector()
{
  reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameLayoutDetector::onReset()
{
  myState = State::waitForVsyncStart;
  myNtscFrames = myPalFrames = 0;
  myLinesWaitingForVsyncToStart = 0;
  myColorCount.fill(0);
  myIsRendering = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameLayoutDetector::onSetVsync(uInt64)
{
  if (myVsync)
    setState(State::waitForVsyncEnd);
  else
    setState(State::waitForVsyncStart);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameLayoutDetector::onNextLine()
{
  const uInt32 frameLines = layout() == FrameLayout::ntsc ? Metrics::frameLinesNTSC : Metrics::frameLinesPAL;

  switch (myState) {
    case State::waitForVsyncStart:
      // We start counting the number of "lines spent while waiting for vsync start" from
      // the "ideal" frame size (corrected by the three scanlines spent in vsync).
      if (myCurrentFrameTotalLines > frameLines - 3 || myTotalFrames == 0)
        ++myLinesWaitingForVsyncToStart;

      if (myLinesWaitingForVsyncToStart > Metrics::waitForVsync) setState(State::waitForVsyncEnd);

      break;

    case State::waitForVsyncEnd:
      if (++myLinesWaitingForVsyncToStart > Metrics::waitForVsync) setState(State::waitForVsyncStart);

      break;

    default:
      throw runtime_error("cannot happen");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameLayoutDetector::pixelColor(uInt8 color)
{
  if(myTotalFrames > Metrics::initialGarbageFrames)
    myColorCount[color >> 1]++;
  // Ideas:
  // - contrast to previous pixels (left/top)
  // - ???
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameLayoutDetector::setState(State state)
{
  if (state == myState) return;

  myState = state;
  myLinesWaitingForVsyncToStart = 0;

  switch (myState) {
    case State::waitForVsyncEnd:
      break;

    case State::waitForVsyncStart:
      finalizeFrame();
      notifyFrameStart();
      break;

    default:
      throw runtime_error("cannot happen");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameLayoutDetector::finalizeFrame()
{
  notifyFrameComplete();

  if (myTotalFrames <= Metrics::initialGarbageFrames) return;

  // Calculate the delta between scanline count and the sweet spot for the respective
  // frame layouts
  const uInt32
    deltaNTSC = abs(static_cast<Int32>(myCurrentFrameFinalLines) - static_cast<Int32>(frameLinesNTSC)),
    deltaPAL =  abs(static_cast<Int32>(myCurrentFrameFinalLines) - static_cast<Int32>(frameLinesPAL));

  // Does the scanline count fall into one of our tolerance windows? -> use it
  if (std::min(deltaNTSC, deltaPAL) <= Metrics::tvModeDetectionTolerance)
    layout(deltaNTSC <= deltaPAL ? FrameLayout::ntsc : FrameLayout::pal);
  else if (
    // If scanline count is odd and lies between the PAL and NTSC windows we assume
    // it is NTSC (it would cause color loss on PAL CRTs)
    (myCurrentFrameFinalLines < frameLinesPAL) &&
    (myCurrentFrameFinalLines > frameLinesNTSC) &&
    (myCurrentFrameFinalLines % 2)
  )
    layout(FrameLayout::ntsc);
  else
    // Take the nearest layout if all else fails
    layout(deltaNTSC <= deltaPAL ? FrameLayout::ntsc : FrameLayout::pal);

  switch (layout()) {
    case FrameLayout::ntsc:
      ++myNtscFrames;
      break;

    case FrameLayout::pal:
      ++myPalFrames;
      break;

    default:
      throw runtime_error("cannot happen");
  }
}
