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

#ifndef COMPUMATE_CASSETTE_HXX
#define COMPUMATE_CASSETTE_HXX

class System;

#include "bspf.hxx"
#include "ConsoleTiming.hxx"
#include "FSNode.hxx"

/**
  Handles CompuMate cassette tape emulation as two distinct layers:

    Digital layer (myCasData): the canonical cassette content.  Populated
      from a file on LOAD, written back to a file on SAVE.  Persists
      across the session and is the single source of truth.

    Transport layer (FSK): the analog mechanism by which the ROM exchanges
      bytes with the "tape".  For LOAD, cassetteBit() synthesises FSK
      samples from myCasData for the ROM to read via SWCHA D7.  For SAVE,
      cassetteD6Toggled() decodes incoming FSK from the ROM and writes
      decoded bytes back into myCasData at indexed positions.

  Because the decoder writes into myCasData byte-by-byte, idempotency is
  automatic: positions the decoder cannot recover keep their loaded
  values, so a failed decode degrades to "file unchanged" rather than
  "file corrupted".

  FSK encodes each bit as an audio tone at one of two frequencies: a 0-bit
  is one LOW/HIGH cycle per bit period (~1225 Hz) and a 1-bit is two
  cycles (~2450 Hz).  Each byte is framed as 1 start bit + 8 data bits
  (LSB first) + 4 stop bits, at 22050 Hz with 18 samples per bit.

  The save decoder uses per-byte UART-style resync: each byte establishes
  its own bit-period base on its start bit, so timing drift cannot
  accumulate across bytes.  Keyboard-column CLK transitions (~6 cycles
  apart) are filtered out before reaching the decoder.

  The file format is raw program bytes only (0x604 or 0x804).  The
  943-byte 0xFF header that precedes program data on a real tape is
  synthesised on-the-fly during playback and is not stored on disk.

  NOTE: Future formats (e.g. WAV) are intended to be handled here without
        any changes to the rest of the CompuMate subsystem.

  @author  Stephen Anthony

  With ideas/inspiration from https://github.com/goldnchild/mame/tree/compumate
  License: BSD-3-Clause
  Copyright-holders: Nathan Woods, Wilbert Pol, David Shah and Golden Child
*/
class CompuMateCassette
{
  public:
    explicit CompuMateCassette(const System& system);
    ~CompuMateCassette() = default;

    /**
      Load program bytes from romFile's .bin sibling.
      Also stores the ROM path for deriving the default save destination.
    */
    void load(const FSNode& romFile, ConsoleTiming timing);

    /**
      Arm playback.  After this call, cassetteBit() returns live waveform
      data rather than the idle-high level.
    */
    void startPlayback(uInt64 cycle) { myCasStartCycle = cycle; }

    /**
      Return the current FSK waveform bit for SWCHA D7.
      Returns 1 (pulled high) when no cassette is loaded or playback
      has not started yet.
    */
    uInt8 cassetteBit() const;

    bool isLoaded()  const { return !myCasData.empty(); }
    bool isPlaying() const { return myCasStartCycle != UINT64_MAX; }

    /**
      Arm FSK recording.  The expected save size is derived from any
      previously loaded cassette data (0x804 if that was the loaded size;
      0x604 otherwise).  Decoded bytes update myCasData in place; bytes
      that fail to decode retain their loaded values.
    */
    void startRecord();

    /**
      Called by CartCM whenever SWCHA bit 6 (D6, audio-out/CLK) transitions.
      Filters keyboard-CLK noise and feeds genuine FSK transitions to the
      per-byte UART decoder.
    */
    void cassetteD6Toggled(uInt64 cycles);

    bool isRecording() const { return mySaveActive; }

    /**
      Called periodically while recording.  When D6 has been idle for long
      enough, treats the save as complete and writes myCasData to file.
    */
    void checkTimeout(uInt64 currentCycle);

    /**
      Set the destination path for the next save.  Called when the
      file-browser dialog confirms a filename; for now set automatically
      to the default sibling path.
    */
    void setSavePath(const FSNode& path) { mySavePath = path; }

    /**
      The default save destination: the .bin sibling of the loaded ROM.
      Used until a file-browser dialog is available to provide a real path.
    */
    FSNode defaultCassettePath() const { return myCasRomPath.getSiblingNode(".bin"); }

    /**
      Write the current cassette data (myCasData) to destFile.
      Returns true on success.
    */
    bool save(const FSNode& destFile);

  private:
    void finalizeSaveByte();
    void finalizeSave();

    // FSK cassette format constants
    // Sample rate used for all FSK timing calculations (Hz)
    static constexpr double SAMPLE_RATE          = 22050.0;
    // Audio samples per FSK bit period; determines tone frequencies
    static constexpr double SAMPLES_PER_BIT      = 18.0;
    // Bits per UART byte frame: 1 start + 8 data + 4 stop
    static constexpr uInt32 FRAME_BITS           = 13;
    // 0xFF sync bytes synthesised before program data during playback;
    // at NTSC this is ~10 seconds of sync tone, giving the ROM's FSK
    // decoder time to lock before the first data byte arrives
    static constexpr uInt32 SYNC_BYTES           = 943;
    // Valid cassette image sizes (raw program bytes, no header)
    static constexpr size_t CAS_SIZE_NORMAL      = 0x604;
    static constexpr size_t CAS_SIZE_EXTENDED    = 0x804;

    // FSK decoder thresholds (CPU cycles)
    // Gaps shorter than this are keyboard column-CLK noise, not FSK transitions
    static constexpr uInt32 MIN_FSK_GAP          = 150;
    // During calibration, a gap this wide means the captured T0 was a stray
    // pre-FSK edge; re-anchor T0 to the current transition
    static constexpr uInt32 MAX_CALIB_GAP        = 800;
    // D6 idle time before treating a save as complete (~50 ms at NTSC)
    static constexpr uInt32 SAVE_TIMEOUT_CYCLES  = 60'000;

  private:
    const System& mySystem;

    // Playback
    ByteArray      myCasData;
    double         myCasFreqRatio{0.0};
    uInt64         myCasStartCycle{UINT64_MAX};
    FSNode         myCasRomPath;
    mutable uInt64 myCasLastPct{UINT64_MAX};

    // Save (transport-layer FSK decoder; writes into myCasData)
    FSNode  mySavePath;
    bool    mySaveActive{false};
    // Start cycle of current byte (UINT64_MAX = IDLE)
    uInt64  mySaveT0{UINT64_MAX};
    uInt64  mySaveLastToggle{0};
    // CPU cycles per FSK bit window (~974 at NTSC)
    double  myCyclesPerBit{0.0};
    size_t  mySaveExpectedSize{0};
    // Index into myCasData for next data byte
    size_t  mySaveBytePos{0};
    // Skipping the leading 0xFF sync run
    bool    mySaveInHeader{true};
    // Last reported save percentage (for dedup)
    uInt64  mySaveLastPct{UINT64_MAX};
    // Diagnostic: bytes that passed validation
    uInt64  mySaveOkCount{0};
    // Diagnostic: bytes that failed validation
    uInt64  mySaveFailCount{0};
    // Bit period auto-measured from first start bit
    bool    mySaveCalibrated{false};
    // Transitions per bit window in current byte
    std::array<uInt8, FRAME_BITS> mySaveBitTrans{};

  private:
    // Following constructors and assignment operators not supported
    CompuMateCassette() = delete;
    CompuMateCassette(const CompuMateCassette&) = delete;
    CompuMateCassette(CompuMateCassette&&) = delete;
    CompuMateCassette& operator=(const CompuMateCassette&) = delete;
    CompuMateCassette& operator=(CompuMateCassette&&) = delete;
};

#endif  // COMPUMATE_CASSETTE_HXX
