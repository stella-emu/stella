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

#include "System.hxx"
#include "CompuMateCassette.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CompuMateCassette::CompuMateCassette(const System& system)
  : mySystem{system}
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CompuMateCassette::load(const FSNode& romFile, ConsoleTiming timing)
{
  myCasStartCycle = UINT64_MAX;  // stop any current playback
  myCasRomPath = romFile;

  double cpuFreq{};
  switch(timing)
  {
    case ConsoleTiming::pal:   cpuFreq = 1182298.0; break;
    case ConsoleTiming::secam: cpuFreq = 1187500.0; break;
    default:                   cpuFreq = 1193182.0; break;
  }
  myCasFreqRatio = SAMPLE_RATE / cpuFreq;

  const FSNode casFile = romFile.getSiblingNode(".bin");
  if(!casFile.isReadable())
    return;

  ByteArray data;
  if(casFile.read(data) == 0)
    return;

  if(data.size() != CAS_SIZE_NORMAL && data.size() != CAS_SIZE_EXTENDED)
  {
    cerr << std::format("CompuMate: unexpected cassette size {}\n", data.size());
    return;
  }

  myCasData = std::move(data);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CompuMateCassette::cassetteBit() const
{
  // No cassette loaded, or play hasn't been started yet
  if(myCasFreqRatio == 0.0 || myCasStartCycle == UINT64_MAX)
    return 1;

  // Convert current system cycle count to cassette sample position.
  // Header: SYNC_BYTES bytes of 0xFF precede the program data (~10 seconds of sync tone)

  const auto samplePos = static_cast<double>(mySystem.cycles() - myCasStartCycle) * myCasFreqRatio;
  const auto streamBitIdx = static_cast<uInt64>(samplePos / SAMPLES_PER_BIT);
  const auto phase = samplePos - static_cast<double>(streamBitIdx) * SAMPLES_PER_BIT;

  const uInt64 byteIdx = streamBitIdx / FRAME_BITS;
  const auto bitInByte = static_cast<uInt32>(streamBitIdx % FRAME_BITS);

  const uInt64 totalBytes = SYNC_BYTES + myCasData.size();
  const uInt64 pct = byteIdx < totalBytes ? byteIdx * 100 / totalBytes : 100;
  if(pct != myCasLastPct)
  {
    myCasLastPct = pct;
    if(pct < 100)
      cerr << std::format("\rLoading cassette: {}%", pct) << std::flush;
    else
      cerr << "\rLoading cassette: 100%\n";
  }

  uInt8 byte{};
  if(byteIdx < SYNC_BYTES)
    byte = 0xFF;
  else if(byteIdx - SYNC_BYTES < myCasData.size())
    byte = myCasData[byteIdx - SYNC_BYTES];
  else
    return 1;  // past end of cassette

  // Decode the current bit value
  bool bit{};
  if(bitInByte == 0)
    bit = false;                          // start bit
  else if(bitInByte < 9)
    bit = (byte >> (bitInByte - 1)) & 1;  // 8 data bits, LSB first
  else
    bit = true;                           // 4 stop bits

  // Return waveform level at sub-bit phase position
  if(!bit)
    return phase < 9.0 ? 0 : 1;
  else
    return (phase < 5.0 || (phase >= 9.0 && phase < 14.0)) ? 0 : 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CompuMateCassette::startRecord()
{
  if(myCasFreqRatio == 0.0)
  {
    cerr << "CompuMate: cassette not initialised; cannot record\n";
    return;
  }

  mySaveExpectedSize = (myCasData.size() == CAS_SIZE_EXTENDED) ? CAS_SIZE_EXTENDED : CAS_SIZE_NORMAL;

  // Ensure the canonical buffer is the right size.  Bytes already present
  // from a prior load act as the baseline; bytes the decoder fails to
  // recover keep those loaded values, giving graceful idempotency.
  if(myCasData.size() < mySaveExpectedSize)
    myCasData.resize(mySaveExpectedSize, 0);

  mySaveActive     = true;
  mySaveT0         = UINT64_MAX;
  mySaveLastToggle = mySystem.cycles();
  mySaveBytePos    = 0;
  mySaveInHeader   = true;
  mySaveLastPct    = UINT64_MAX;
  mySaveOkCount    = 0;
  mySaveFailCount  = 0;
  mySaveCalibrated = false;
  mySaveBitTrans.fill(0);
  myCyclesPerBit   = SAMPLES_PER_BIT / myCasFreqRatio;  // initial guess; refined on first start bit
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CompuMateCassette::cassetteD6Toggled(uInt64 cycles)
{
  if(!mySaveActive)
    return;

  // Filter out keyboard column-CLK transitions.  These appear at ~66 cycles
  // (LOW phase) and ~105 cycles (HIGH phase); genuine FSK transitions are
  // at least ~216 cycles apart (4-sample 1-bit phase), so 150 cleanly
  // separates the two populations.
  const uInt64 gap = cycles - mySaveLastToggle;
  mySaveLastToggle = cycles;

  if(gap < MIN_FSK_GAP)
    return;

  // Auto-calibrate the bit period from the very first start bit.
  // SAMPLES_PER_BIT / myCasFreqRatio gives a nominal value, but the ROM's
  // actual FSK timing differs enough that transitions near bit boundaries
  // get misclassified without calibration.  The start bit is a 0 (LOW/HIGH
  // cycle = 2 transitions); the 3rd transition is the HIGH->LOW edge that
  // begins bit 1, so its offset from T0 is exactly one complete bit period.
  //
  // Genuine FSK transitions never exceed ~half a bit period apart, so any
  // larger gap during calibration means the previously-captured T0 was a
  // stray edge (a keyboard transition that slipped through the gap filter).
  // Re-anchor T0 to the current transition.
  if(!mySaveCalibrated)
  {
    if(mySaveT0 != UINT64_MAX && gap > MAX_CALIB_GAP)
    {
      mySaveT0 = UINT64_MAX;
      mySaveBitTrans.fill(0);
    }
    if(mySaveT0 == UINT64_MAX)
    {
      mySaveT0 = cycles;
      mySaveBitTrans[0] = 1;
    }
    else if(mySaveBitTrans[0] == 1)
    {
      mySaveBitTrans[0] = 2;
    }
    else
    {
      myCyclesPerBit   = static_cast<double>(cycles - mySaveT0);
      mySaveCalibrated = true;
      mySaveBitTrans[1] = 1;
      cerr << std::format("CompuMate FSK calibrated: {} cycles/bit\n", myCyclesPerBit);
    }
    return;
  }

  // Per-byte UART resync: if the current byte's time window has elapsed,
  // finalize it before treating this transition as the next byte's start.
  if(mySaveT0 != UINT64_MAX)
  {
    // A small phase offset absorbs cycle jitter at bin boundaries.
    // Without it, a transition arriving just before an expected bit boundary
    // floors into the previous bin, corrupting the byte's transition pattern.
    // myCyclesPerBit/16 is wider than any expected jitter and still well
    // within the bin 12 upper bound.
    const auto cyclesSinceT0 = static_cast<double>(cycles - mySaveT0);
    const auto bitWindow = static_cast<int>(
        (cyclesSinceT0 + myCyclesPerBit / 16.0) / myCyclesPerBit);
    // A complete 1-bit stop bit holds exactly 4 transitions; once bit 12 is
    // "full", any further transition belongs to the next byte's start bit.
    // This protects against floor() rounding at the bin-12 boundary.
    const bool byteFull = (bitWindow > 12) ||
                          (bitWindow == 12 && mySaveBitTrans[12] >= 4);
    if(!byteFull)
    {
      ++mySaveBitTrans[bitWindow];
      return;
    }
    finalizeSaveByte();  // sets mySaveT0 = UINT64_MAX
  }

  // Begin a new byte.  This transition is the start bit's HIGH→LOW edge.
  mySaveT0 = cycles;
  mySaveBitTrans.fill(0);
  mySaveBitTrans[0] = 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CompuMateCassette::finalizeSaveByte()
{
  mySaveT0 = UINT64_MAX;

  // Frame validation: start bit must be a 0 (2 transitions),
  // and all four stop bits must be 1 (4 transitions each).
  const bool validStart = (mySaveBitTrans[0] == 2);
  bool validStop = true;
  for(int i = 9; i <= 12; ++i)
    if(mySaveBitTrans[i] != 4) { validStop = false; break; }

  if(!validStart || !validStop)
  {
    ++mySaveFailCount;
    return;
  }

  // Decode 8 data bits, LSB first
  uInt8 byte = 0;
  for(int i = 1; i <= 8; ++i)
  {
    if(mySaveBitTrans[i] == 4)
      byte |= static_cast<uInt8>(1U << (i - 1));
    else if(mySaveBitTrans[i] != 2)
    {
      ++mySaveFailCount;
      return;
    }
  }
  ++mySaveOkCount;

  // The ROM emits a long run of 0xFF sync bytes before the program data.
  // Its length varies by ROM revision, so rather than hardcoding it,
  // treat the first non-0xFF decoded byte as data[0].  Real program data
  // essentially never begins with 0xFF.
  if(mySaveInHeader)
  {
    if(byte == 0xFF) return;
    mySaveInHeader = false;
  }

  if(mySaveBytePos < mySaveExpectedSize)
  {
    myCasData[mySaveBytePos] = byte;
    ++mySaveBytePos;
    const uInt64 pct = mySaveBytePos * 100 / mySaveExpectedSize;
    if(pct != mySaveLastPct)
    {
      mySaveLastPct = pct;
      if(pct < 100)
        cerr << std::format("\rSaving cassette: {}%", pct) << std::flush;
      else
        cerr << "\rSaving cassette: 100%\n";
    }
    if(mySaveBytePos >= mySaveExpectedSize)
      finalizeSave();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CompuMateCassette::checkTimeout(uInt64 currentCycle)
{
  if(!mySaveActive)
    return;
  if(currentCycle - mySaveLastToggle > SAVE_TIMEOUT_CYCLES)
  {
    if(mySaveT0 != UINT64_MAX)
      finalizeSaveByte();
    if(mySaveActive)  // finalizeSaveByte() may have already finalised
      finalizeSave();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CompuMateCassette::finalizeSave()
{
  mySaveActive = false;
  mySaveT0     = UINT64_MAX;
  cerr << std::format("CompuMate FSK decode: {} bytes ok, {} bytes failed\n",
                      mySaveOkCount, mySaveFailCount);
  save(mySavePath);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CompuMateCassette::save(const FSNode& destFile)
{
  if(myCasData.empty())
  {
    cerr << "CompuMate: no cassette data to save\n";
    return false;
  }
  if(destFile.write(myCasData) != myCasData.size())
  {
    cerr << "CompuMate: cassette save write failed\n";
    return false;
  }
  cerr << std::format("CompuMate: cassette saved {} bytes to {}\n",
                      myCasData.size(), destFile.getShortPath());
  return true;
}
