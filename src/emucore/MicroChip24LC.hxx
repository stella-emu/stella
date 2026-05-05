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

#ifndef MICRO_CHIP_24LC_HXX
#define MICRO_CHIP_24LC_HXX

// #define DEBUG_EEPROM 1

#include "Control.hxx"
#include "FSNode.hxx"
#include "System.hxx"
#include "bspf.hxx"

/**
  Emulates a Microchip Technology Inc. 24LC256, a 32KB Serial Electrically
  Erasable PROM accessed using the I2C protocol.  Thanks to J. Payson
  (aka Supercat) for the bulk of this code (variables and methods named
  starting with jpee_).

  Converted to templates to support also the 24LC16B / EFF (Grizzards) cart
  type by Bruce-Robert Pocock with loads of help from Ryan Witmer.

  @author Stephen Anthony & J. Payson, and Bruce-Robert Pocock & Ryan Witmer
*/
template<size_t DEVICE_FLASH_SIZE, size_t DEVICE_PAGE_SIZE>
class MicroChip24LC
{
  public:
    /**
      Create a new 24LC device with its data stored in the given file.

      @param eepromfile Data file containing the EEPROM data
      @param system     The system using the controller of this device
      @param callback   Called to pass messages back to the parent controller
    */
    MicroChip24LC(const FSNode& eepromfile, const System& system,
                  const Controller::onMessageCallback& callback);

    /**
      Destructor. Saves EEPROM data to disk if it was modified during emulation.
    */
    ~MicroChip24LC();

  public:
    // Total EEPROM size in bytes
    static constexpr size_t FLASH_SIZE = DEVICE_FLASH_SIZE;
    // Size of one write page in bytes
    static constexpr size_t PAGE_SIZE  = DEVICE_PAGE_SIZE;
    // Number of pages in the EEPROM
    static constexpr size_t PAGE_NUM   = FLASH_SIZE / PAGE_SIZE;

    // Initial state value of flash EEPROM
    static constexpr uInt8 INITIAL_VALUE = 0xFF;

    /**
      Read the current state of the SDA line.
      Returns true only if both the master and slave are driving SDA high.

      @return  Current logical state of the SDA line
    */
    bool readSDA() const { return jpee_mdat && jpee_sdat; }

    /**
      Write the state of the SDA (data) line.
      In smallmode, drives the I2C data logic directly.
      In normal mode, queues the update for simultaneous SDA/SCL processing.

      @param state  The new logical state of the SDA line
    */
    void writeSDA(bool state);

    /**
      Write the state of the SCL (clock) line.
      In smallmode, drives the I2C clock logic directly.
      In normal mode, queues the update for simultaneous SDA/SCL processing.

      @param state  The new logical state of the SCL line
    */
    void writeSCL(bool state);

    /**
      Called when the system is being reset.
      Clears the page-hit tracking array so eraseCurrent() starts fresh.
    */
    void systemReset();

    /**
      Erase the entire EEPROM to the initial value (0xFF).
      Marks the data as changed so it will be saved on destruction.
    */
    void eraseAll();

    /**
      Erase only the pages accessed by the current ROM to the initial value (0xFF).
      Pages are identified by the myPageHit array, populated during emulation.
      Marks the data as changed so it will be saved on destruction.
    */
    void eraseCurrent();

    /**
      Returns true if the given page has been accessed by the current ROM.

      @param page  The page number to query (0-based)
      @return      True if the page was read or written during emulation
    */
    bool isPageUsed(uInt32 page) const;

  private:
    /**
      Process simultaneous SDA and SCL pin updates.
      Only fires when both pins were written in the same cycle, ensuring
      correct I2C signal ordering regardless of which pin was written first.
    */
    void update();

  private:
    // The system of the parent controller
    const System& mySystem;

    // Sends messages back to the parent class
    // Currently used for indicating read/write access
    Controller::onMessageCallback myCallback;

    // The EEPROM data buffer, allocated on first use
    ByteBuffer myData;

    // Tracks which EEPROM pages have been accessed by the current ROM
    std::array<bool, PAGE_NUM> myPageHit{};

    // Cached state of the SDA and SCL pins on the last write
    bool mySDA{false}, mySCL{false};

    // Indicates that the EEPROM write timer is active and hasn't expired yet
    bool myTimerActive{false};

    // Cycle count when the write timer was last armed
    uInt64 myCyclesWhenTimerSet{0};

    // Cycle counts when the SDA and SCL pins were last written
    // Used to detect simultaneous pin updates in non-smallmode
    uInt64 myCyclesWhenSDASet{0}, myCyclesWhenSCLSet{0};

    // The file used to persist EEPROM data across emulator sessions
    FSNode myDataFile;

    // True if the EEPROM data has been modified since the last save
    bool myDataChanged{false};

    ////////////////////////////////////////////////
    // Required for I2C functionality

    // State values for I2C
    enum class JPEEState: uInt8 {
      Idle        = 0,  // Idle
      ByteIn      = 1,  // Byte going to chip (shift left until bit 8 is set)
      Acknowledge = 2,  // Chip outputting acknowledgement
      ByteOut     = 3,  // Byte coming in from chip (shift left until lower 8 bits are clear)
      WaitAck     = 4   // Chip waiting for acknowledgement
    };
    enum class TimerMode: bool { Check = false, Set = true };

    // Current I2C protocol state
    JPEEState jpee_state{JPEEState::Idle};
    // Current state of the SDA line as driven by the master
    bool jpee_mdat{false};
    // Current state of the SDA line as driven by the chip (slave)
    bool jpee_sdat{true};
    // Current state of the SCL clock line
    bool jpee_mclk{false};
    // True if the chip has seen enough address bytes to know the current address
    bool jpee_ad_known{false};
    // Packet buffer write pointer; index of next byte to write in jpee_packet
    uInt32 jpee_pptr{0};
    // I2C shift register; bits are shifted in/out during clock transitions
    uInt32 jpee_nb{0};
    // Current EEPROM byte address being read from or written to
    uInt32 jpee_address{0};
    // Packet buffer holding the current I2C transaction bytes
    std::array<uInt8, 2 * PAGE_SIZE> jpee_packet{};

    // Bitmask for wrapping addresses to the EEPROM flash size
    static constexpr auto jpee_sizemask = static_cast<uInt32>(FLASH_SIZE - 1);
    // Bitmask for detecting page boundary crossings during writes
    static constexpr auto jpee_pagemask = static_cast<uInt32>(PAGE_SIZE - 1);
    // True for 24LC16B (small/page-16 mode); selects I2C address encoding variant
    static constexpr bool jpee_smallmode = (PAGE_SIZE == 16);  // EFF (Grizzards)

  private:
    /**
      Handle an I2C START condition (SDA falling while SCL is high).
      Resets the packet buffer and transitions to the Acknowledge state,
      unless the write timer is still active (chip busy).
    */
    void jpee_data_start();

    /**
      Handle an I2C STOP condition (SDA rising while SCL is high).
      If a valid write transaction is pending, commits the data to the
      EEPROM buffer and arms the write timer.
    */
    void jpee_data_stop();

    /**
      Handle a falling edge on the SCL clock line.
      Drives the I2C state machine — shifts bits in or out depending on
      the current state (ByteIn, Acknowledge, ByteOut, WaitAck).
    */
    void jpee_clock_fall();

    /**
      Arm or check the EEPROM write timer.
      The timer models the real-world write cycle time (~5ms) during which
      the chip ignores new START conditions.

      @param mode  TimerMode::Set to arm the timer, TimerMode::Check to test it
      @return      true if the timer is active (chip busy), false if expired or
                   in smallmode (where the timer is always bypassed)
    */
    bool jpee_timercheck(TimerMode mode);

    /**
      Process a change on the SCL clock line.
      Detects falling edges and delegates to jpee_clock_fall().

      @param state  The new logical state of the SCL line
    */
    void jpee_clock(bool state) {
      if(state)
        jpee_mclk = true;
      else
      {
        if(jpee_mclk)
          jpee_clock_fall();  // falling edge detected
        jpee_mclk = false;
      }
    }

    /**
      Process a change on the SDA data line.
      Detects START (falling while SCL high) and STOP (rising while SCL high)
      conditions and delegates accordingly.

      @param state  The new logical state of the SDA line
    */
    void jpee_data(bool state) {
      if(state)
      {
        // rising edge: 0 → 1
        if(!jpee_mdat && jpee_sdat && jpee_mclk)
          jpee_data_stop();
        jpee_mdat = true;
      }
      else
      {
        // falling edge: 1 → 0
        if(jpee_mdat && jpee_sdat && jpee_mclk)
          jpee_data_start();
        jpee_mdat = false;
      }
    }

  private:
    // Following constructors and assignment operators not supported
    MicroChip24LC() = delete;
    MicroChip24LC(const MicroChip24LC&) = delete;
    MicroChip24LC(MicroChip24LC&&) = delete;
    MicroChip24LC& operator=(const MicroChip24LC&) = delete;
    MicroChip24LC& operator=(MicroChip24LC&&) = delete;
};

// ############################################################################
// Implementation
// ############################################################################

#ifdef DEBUG_EEPROM
  static constexpr bool DEBUG_EEPROM_LOG = true;
#else
  static constexpr bool DEBUG_EEPROM_LOG = false;
#endif

static inline void jpee_logproc(string_view msg) {
  if constexpr(DEBUG_EEPROM_LOG)
    cerr << "    " << msg << '\n';
}
static inline void JPEE_LOG0(string_view msg) {
  if constexpr(DEBUG_EEPROM_LOG)
    jpee_logproc(msg);
}
static inline void JPEE_LOG1(string_view msg, int arg1) {
  if constexpr(DEBUG_EEPROM_LOG)
    jpee_logproc(std::vformat(msg, std::make_format_args(arg1)));
}
static inline void JPEE_LOG2(string_view msg, int arg1, int arg2) {
  if constexpr(DEBUG_EEPROM_LOG)
    jpee_logproc(std::vformat(msg, std::make_format_args(arg1, arg2)));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<size_t FLASH_SIZE, size_t PAGE_SIZE>
MicroChip24LC<FLASH_SIZE, PAGE_SIZE>
::MicroChip24LC(const FSNode& eepromfile, const System& system,
                const Controller::onMessageCallback& callback)
  : mySystem{system},
    myCallback{callback},
    myDataFile{eepromfile}
{
  if constexpr(DEBUG_EEPROM_LOG)
    cerr << " NVRAM (EEPROM) file: " << myDataFile.getPath() << "\n";

  // Load the data from an external file (if it exists)
  // A valid file must be FLASH_FIZE bytes; otherwise we create a new one
  bool fileValid = false;
  try { fileValid = (myDataFile.read(myData) == FLASH_SIZE); }
  catch(...) { ; /* Any read error means we create a new buffer below */ }

  if(!fileValid)
  {
    myData = std::make_unique<uInt8[]>(FLASH_SIZE);
    std::fill_n(myData.get(), FLASH_SIZE, INITIAL_VALUE);
    myDataChanged = true;
  }

  systemReset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<size_t FLASH_SIZE, size_t PAGE_SIZE>
MicroChip24LC<FLASH_SIZE, PAGE_SIZE>
::~MicroChip24LC()
{
  // Save EEPROM data to external file only when necessary
  if(myDataChanged)
  {
    try { myDataFile.write(myData, FLASH_SIZE); }
    catch(...) {
      cerr << "ERROR writing MT24LC flash data file " << myDataFile.getPath() << '\n';
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<size_t FLASH_SIZE, size_t PAGE_SIZE>
void MicroChip24LC<FLASH_SIZE, PAGE_SIZE>
::writeSDA(bool state)
{
  mySDA = state;
  myCyclesWhenSDASet = mySystem.cycles();

  if constexpr(jpee_smallmode)
    jpee_data(mySDA);
  else
    update();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<size_t FLASH_SIZE, size_t PAGE_SIZE>
void MicroChip24LC<FLASH_SIZE, PAGE_SIZE>
::writeSCL(bool state)
{
  mySCL = state;
  myCyclesWhenSCLSet = mySystem.cycles();

  if constexpr(jpee_smallmode)
    jpee_clock(mySCL);
  else
    update();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<size_t FLASH_SIZE, size_t PAGE_SIZE>
void MicroChip24LC<FLASH_SIZE, PAGE_SIZE>
::update()
{
  // These pins have to be updated at the same time
  // However, there's no guarantee that the writeSDA() and writeSCL()
  // methods will be called at the same time or in the correct order, so
  // we only do the write when they have the same 'timestamp'
  if(myCyclesWhenSDASet == myCyclesWhenSCLSet)
  {
    if constexpr(DEBUG_EEPROM_LOG)
      cerr << "\n  I2C_PIN_WRITE(SCL = " << mySCL
           << ", SDA = " << mySDA << ")" << " @ " << mySystem.cycles() << '\n';

    jpee_clock(mySCL);
    jpee_data(mySDA);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<size_t FLASH_SIZE, size_t PAGE_SIZE>
void MicroChip24LC<FLASH_SIZE, PAGE_SIZE>
::systemReset()
{
  myPageHit.fill(false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<size_t FLASH_SIZE, size_t PAGE_SIZE>
void MicroChip24LC<FLASH_SIZE, PAGE_SIZE>
::eraseAll()
{
  std::fill_n(myData.get(), FLASH_SIZE, INITIAL_VALUE);
  myDataChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<size_t FLASH_SIZE, size_t PAGE_SIZE>
void MicroChip24LC<FLASH_SIZE, PAGE_SIZE>
::eraseCurrent()
{
  for(uInt32 page = 0; page < PAGE_NUM; ++page)
  {
    if(myPageHit[page])
    {
      std::fill_n(myData.get() + page * PAGE_SIZE, PAGE_SIZE, INITIAL_VALUE);
      myDataChanged = true;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<size_t FLASH_SIZE, size_t PAGE_SIZE>
bool MicroChip24LC<FLASH_SIZE, PAGE_SIZE>
::isPageUsed(uInt32 page) const
{
  return page < PAGE_NUM && myPageHit[page];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<size_t FLASH_SIZE, size_t PAGE_SIZE>
void MicroChip24LC<FLASH_SIZE, PAGE_SIZE>
::jpee_data_start()
{
  /* We have a start condition */
  if(jpee_state == JPEEState::ByteIn && (jpee_nb != 1 || jpee_pptr != 3))
  {
    JPEE_LOG0("I2C_WARNING ABANDON WRITE");
    jpee_ad_known = false;
  }
  if(jpee_state == JPEEState::ByteOut)
  {
    JPEE_LOG0("I2C_WARNING ABANDON READ");
  }
  if(!jpee_timercheck(TimerMode::Check))
  {
    JPEE_LOG0("I2C_START");
    jpee_state = JPEEState::Acknowledge;
  }
  else
  {
    JPEE_LOG0("I2C_BUSY");
    jpee_state = JPEEState::Idle;
  }
  jpee_pptr = 0;
  jpee_nb = 0;
  jpee_packet[0] = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<size_t FLASH_SIZE, size_t PAGE_SIZE>
void MicroChip24LC<FLASH_SIZE, PAGE_SIZE>
::jpee_data_stop()
{
  if(jpee_state == JPEEState::ByteIn && jpee_nb != 1)
  {
    JPEE_LOG0("I2C_WARNING ABANDON_WRITE");
    jpee_ad_known = false;
  }
  if(jpee_state == JPEEState::ByteOut)
  {
    JPEE_LOG0("I2C_WARNING ABANDON_READ");
    jpee_ad_known = false;
  }
  /* We have a stop condition. */
  if(jpee_state == JPEEState::ByteIn && jpee_nb == 1 && jpee_pptr > 3)
  {
    jpee_timercheck(TimerMode::Set);
    JPEE_LOG2("I2C_STOP(Write {} bytes at {:04X})", jpee_pptr - 3, jpee_address);
    if(((jpee_address + jpee_pptr - 4) ^ jpee_address) & ~jpee_pagemask)
    {
      jpee_pptr = 4 + jpee_pagemask - (jpee_address & jpee_pagemask);
      JPEE_LOG1("I2C_WARNING PAGECROSSING!(Truncate to {} bytes)", jpee_pptr - 3);
    }
    for(uInt32 i = 3; i < jpee_pptr; ++i)
    {
      myDataChanged = true;
      myPageHit[jpee_address / PAGE_SIZE] = true;

      if(myCallback)
      {
        if constexpr(jpee_smallmode)
          myCallback("Cartridge EEPROM write");
        else
          myCallback("AtariVox/SaveKey EEPROM write");
      }

      myData[(jpee_address++) & jpee_sizemask] = jpee_packet[i];
      if(!(jpee_address & jpee_pagemask))
        break;  /* Writes can't cross page boundary! */
    }
    jpee_ad_known = false;
  }
  else
  {
    if constexpr(DEBUG_EEPROM_LOG)
      jpee_logproc("I2C_STOP");

    // Do NOT call jpee_timercheck(TimerMode::Set) here — the write timer
    // must only be armed after a committed page write. Arming it on an
    // abandoned transaction causes the next jpee_data_start() to see a
    // false busy condition and drop the transaction.
  }

  jpee_state = JPEEState::Idle;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<size_t FLASH_SIZE, size_t PAGE_SIZE>
void MicroChip24LC<FLASH_SIZE, PAGE_SIZE>
::jpee_clock_fall()
{
  switch(jpee_state)
  {
    case JPEEState::ByteIn:
      jpee_nb <<= 1;
      jpee_nb |= static_cast<int>(jpee_mdat);
      if(jpee_nb & 256)
      {
        if(!jpee_pptr)
        {
          jpee_packet[0] = static_cast<uInt8>(jpee_nb);

          if constexpr(jpee_smallmode)
          {
            if((jpee_nb & 0xF0) == 0xA0)
            {
              jpee_packet[1] = (jpee_nb >> 1) & 7;
              if constexpr(DEBUG_EEPROM_LOG)
                if(jpee_packet[1] != (jpee_address >> 8) && (jpee_packet[0] & 1))
                  jpee_logproc("I2C_WARNING ADDRESS MSB CHANGED");
              jpee_nb &= 0x1A1;
            }
          }

          if(jpee_nb == 0x1A0)
          {
            JPEE_LOG1("I2C_SENT({:02X}--start write)", jpee_packet[0]);
            jpee_state = JPEEState::Acknowledge;
            jpee_sdat = false;
          }
          else if(jpee_nb == 0x1A1)
          {
            jpee_state = JPEEState::WaitAck;
            if constexpr(DEBUG_EEPROM_LOG)
            {
              JPEE_LOG2("I2C_SENT({:02X}--start read @{:04X})",
                        jpee_packet[0],jpee_address);
              if(!jpee_ad_known)
                jpee_logproc("I2C_WARNING ADDRESS IS UNKNOWN");
            }

            jpee_sdat = false;
          }
          else
          {
            JPEE_LOG1("I2C_WARNING ODDBALL FIRST BYTE!({:02X})", jpee_nb);
            jpee_state = JPEEState::Idle;
          }
        }
        else
        {
          jpee_state = JPEEState::Acknowledge;
          jpee_sdat = false;
        }
      }
      break;

    case JPEEState::Acknowledge:
      if(jpee_nb)
      {
        if(!jpee_pptr)
        {
          jpee_packet[0] = static_cast<uInt8>(jpee_nb);
          if constexpr(jpee_smallmode)
            jpee_pptr = 2;
          else
            jpee_pptr = 1;
        }
        else if(jpee_pptr < static_cast<uInt32>(jpee_packet.size()))
        {
          JPEE_LOG1("I2C_SENT({:02X})", jpee_nb);
          jpee_packet[jpee_pptr++] = static_cast<uInt8>(jpee_nb);
          jpee_address = (jpee_packet[1] << 8) | jpee_packet[2];
          if(jpee_pptr > 2)
            jpee_ad_known = true;
        }
        else if constexpr(DEBUG_EEPROM_LOG)
          jpee_logproc("I2C_WARNING OUTPUT_OVERFLOW!");
      }
      jpee_sdat = true;
      jpee_nb = 1;
      jpee_state = JPEEState::ByteIn;
      break;

    case JPEEState::WaitAck:
      if(jpee_mdat && jpee_sdat)
      {
        JPEE_LOG0("I2C_READ_NAK");
        jpee_state = JPEEState::Idle;
        break;
      }
      jpee_state = JPEEState::ByteOut;
      myPageHit[jpee_address / PAGE_SIZE] = true;

      if(myCallback)
      {
        if constexpr(jpee_smallmode)
          myCallback("Cartridge EEPROM read");
        else
          myCallback("AtariVox/SaveKey EEPROM read");
      }

      jpee_nb = (myData[jpee_address & jpee_sizemask] << 1) | 1;
      JPEE_LOG2("I2C_READ({:04X}={:02X})", jpee_address, jpee_nb >> 1);

      [[fallthrough]];

    case JPEEState::ByteOut:
      jpee_sdat = jpee_nb & 256;
      jpee_nb <<= 1;
      if(!(jpee_nb & 510))
      {
        jpee_state = JPEEState::WaitAck;
        jpee_sdat = true;
        ++jpee_address;
      }
      break;

    default:
      /* Do nothing */
      break;
  }
  JPEE_LOG2("I2C_CLOCK (dat={}/{})", jpee_mdat, jpee_sdat);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<size_t FLASH_SIZE, size_t PAGE_SIZE>
bool MicroChip24LC<FLASH_SIZE, PAGE_SIZE>
::jpee_timercheck(TimerMode mode)
{
  /*
    Number of 2600 CPU cycles corresponding to the 24LC256's 5ms write cycle
    time (tWR).  5,000,000 microseconds per 5ms divided by ~838 CPU cycles
    per millisecond (derived from the 2600's ~1.19MHz CPU clock:
    3.58MHz master / 3 / ~1428).

    The chip must not receive a new START condition until this timer expires.

    TODO: The 838 is a conservative value; document where it's coming from
  */
  static constexpr auto TIMER_CYCLES = static_cast<uInt64>(5000000.0 / 838.0);

  if(mode == TimerMode::Set)
  {
    myCyclesWhenTimerSet = mySystem.cycles();
    return myTimerActive = true;
  }

  if constexpr(!jpee_smallmode)
  {
    if(myTimerActive)  // TimerMode::Check
    {
      const uInt64 elapsed = mySystem.cycles() - myCyclesWhenTimerSet;
      myTimerActive = elapsed < TIMER_CYCLES;
    }
    return myTimerActive;
  }
  else
  {
    /*
      Not sure why this timer isn't working with EFF/Grizzards but
      blocking it out works fine in practice, may not emulate an
      actual timing bug if one occurs however. -BRP
    */
    return false;
  }
}

#endif  // MICRO_CHIP_24LC_HXX
