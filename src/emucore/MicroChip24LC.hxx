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

#ifndef MICROCHIP24LC_HXX
#define MICROCHIP24LC_HXX

// #define DEBUG_EEPROM 1

#include "Control.hxx"
#include "FSNode.hxx"
#include "System.hxx"
#include "bspf.hxx"

/**
  Emulates a Microchip Technology Inc. 24LC256, a 32KB Serial Electrically
  Erasable PROM accessed using the I2C protocol.  Thanks to J. Payson
  (aka Supercat) for the bulk of this code.

  Converted to templates to support also the 24LC16B by Bruce-Robert
  Pocock with loads of help from Ryan Witmer.

  @author Stephen Anthony & J. Payson, and Bruce-Robert Pocock & Ryan Witmer
*/
template<std::size_t device_flash_size, std::size_t device_page_size>
class MicroChip24LC
{
  public:
    /**
      Create a new 24LC device with its data stored in the given file

      @param eepromfile Data file containing the EEPROM data
      @param system     The system using the controller of this device
      @param callback   Called to pass messages back to the parent controller
    */
    MicroChip24LC(const FSNode& eepromfile, const System& system,
                  const Controller::onMessageCallback& callback);

    virtual ~MicroChip24LC();

  public:
    // Sizes of the EEPROM
    static constexpr size_t FLASH_SIZE = device_flash_size;
    static constexpr size_t PAGE_SIZE = device_page_size;
    static constexpr size_t PAGE_NUM = FLASH_SIZE / PAGE_SIZE;

    // Initial state value of flash EEPROM
    static constexpr uInt8 INITIAL_VALUE = 0xFF;

    /** Read boolean data from the SDA line */
    bool readSDA() const { return jpee_mdat && jpee_sdat; }

    /** Write boolean data to the SDA and SCL lines */
    void writeSDA(bool state);
    void writeSCL(bool state);

    /** Called when the system is being reset */
    void systemReset();

    /** Erase entire EEPROM to known state ($FF) */
    void eraseAll();

    /** Erase the pages used by the current ROM to known state ($FF) */
    void eraseCurrent();

    /** Returns true if the page is used by the current ROM */
    bool isPageUsed(uInt32 page) const;

  private:
    // I2C access code provided by Supercat
    void jpee_init();
    void jpee_data_start();
    void jpee_data_stop();
    void jpee_clock_fall();
    bool jpee_timercheck(int mode);
    static void jpee_logproc(string_view st) { cerr << "    " << st << '\n'; }

    void update();

  private:
    // The system of the parent controller
    const System& mySystem;

    // Sends messages back to the parent class
    // Currently used for indicating read/write access
    Controller::onMessageCallback myCallback;

    // The EEPROM data
    ByteBuffer myData;

    // Track which pages are used
    std::array<bool, PAGE_NUM> myPageHit{};

    // Cached state of the SDA and SCL pins on the last write
    bool mySDA{false}, mySCL{false};

    // Indicates that a timer has been set and hasn't expired yet
    bool myTimerActive{false};

    // Indicates when the timer was set
    uInt64 myCyclesWhenTimerSet{0};

    // Indicates when the SDA and SCL pins were set/written
    uInt64 myCyclesWhenSDASet{0}, myCyclesWhenSCLSet{0};

    // The file containing the EEPROM data
    FSNode myDataFile;

    // Indicates if the EEPROM has changed since class invocation
    bool myDataChanged{false};

    // Required for I2C functionality
    bool jpee_smallmode{false};
    Int32 jpee_mdat{0}, jpee_sdat{0}, jpee_mclk{0};
    Int32 jpee_sizemask{0}, jpee_pagemask{0}, jpee_logmode{0};
    Int32 jpee_pptr{0}, jpee_state{0}, jpee_nb{0};
    uInt32 jpee_address{0}, jpee_ad_known{0};
    std::array<uInt8, 2 * PAGE_SIZE> jpee_packet{};

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
  static constexpr size_t jpee_log_msg_len = 0x100;
  static inline char jpee_msg[jpee_log_msg_len];
  #define JPEE_LOG0(msg) jpee_logproc(msg);
  #define JPEE_LOG1(msg,arg1) snprintf(jpee_msg,jpee_log_msg_len-1,(msg),(arg1)), jpee_logproc(jpee_msg);
  #define JPEE_LOG2(msg,arg1,arg2) snprintf(jpee_msg,jpee_log_msg_len-1,(msg),(arg1),(arg2)), jpee_logproc(jpee_msg);
#else
  #define JPEE_LOG0(msg) ;
  #define JPEE_LOG1(msg,arg1) ;
  #define JPEE_LOG2(msg,arg1,arg2) ;
#endif

/*
  State values for I2C:
  0 - Idle
  1 - Byte going to chip (shift left until bit 8 is set)
  2 - Chip outputting acknowledgement
  3 - Byte coming in from chip (shift left until lower 8 bits are clear)
  4 - Chip waiting for acknowledgement
*/

#define jpee_clock(x) ( (x) \
  ? (jpee_mclk = 1)         \
  : (jpee_mclk && (jpee_clock_fall(),1), jpee_mclk = 0))

#define jpee_data(x) ( (x)  \
  ? (!jpee_mdat && jpee_sdat && jpee_mclk && (jpee_data_stop(),1), jpee_mdat = 1) \
  : (jpee_mdat && jpee_sdat && jpee_mclk && (jpee_data_start(),1), jpee_mdat = 0))

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<std::size_t device_flash_size, std::size_t device_page_size>
MicroChip24LC<device_flash_size, device_page_size>
::MicroChip24LC(const FSNode& eepromfile, const System& system,
                const Controller::onMessageCallback& callback)
  : mySystem{system},
    myCallback{callback},
    myDataFile{eepromfile},
    jpee_smallmode{PAGE_SIZE == 16}
{
#ifdef DEBUG_EEPROM
  cerr << " NVRAM (EEPROM) file: " << myDataFile.getPath() << "\n";
#endif
  // Load the data from an external file (if it exists)
  bool fileValid = false;
  try
  {
    // A valid file must be FLASH_FIZE bytes; otherwise we create a new one
    if(myDataFile.read(myData) == FLASH_SIZE)
      fileValid = true;
  }
  catch(...)
  {
    fileValid = false;
  }

  if(!fileValid)
  {
    myData = make_unique<uInt8[]>(FLASH_SIZE);
    std::fill_n(myData.get(), FLASH_SIZE, INITIAL_VALUE);
    myDataChanged = true;
  }

  // Then initialize the I2C state
  jpee_init();

  systemReset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<std::size_t device_flash_size, std::size_t device_page_size>
MicroChip24LC<device_flash_size, device_page_size>
::~MicroChip24LC()
{
  // Save EEPROM data to external file only when necessary
  if(myDataChanged)
  {
    try { myDataFile.write(myData, FLASH_SIZE); }
    catch(...) { cerr << "ERROR writing MT24LC flash data file"
                          << myDataFile.getPath() << "\n"; }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<std::size_t device_flash_size, std::size_t device_page_size>
void MicroChip24LC<device_flash_size, device_page_size>
::writeSDA(bool state)
{
  mySDA = state;
  myCyclesWhenSDASet = mySystem.cycles();

  if(jpee_smallmode)
    jpee_data(mySDA);
  else
    update();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<std::size_t device_flash_size, std::size_t device_page_size>
void MicroChip24LC<device_flash_size, device_page_size>
::writeSCL(bool state)
{
  mySCL = state;
  myCyclesWhenSCLSet = mySystem.cycles();

  if(jpee_smallmode)
    jpee_clock(mySCL);
  else
    update();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<std::size_t device_flash_size, std::size_t device_page_size>
void MicroChip24LC<device_flash_size, device_page_size>
::update()
{
  // These pins have to be updated at the same time
  // However, there's no guarantee that the writeSDA() and writeSCL()
  // methods will be called at the same time or in the correct order, so
  // we only do the write when they have the same 'timestamp'
  if(myCyclesWhenSDASet == myCyclesWhenSCLSet)
  {
  #ifdef DEBUG_EEPROM
    cerr << "\n  I2C_PIN_WRITE(SCL = " << mySCL
         << ", SDA = " << mySDA << ")" << " @ " << mySystem.cycles() << '\n';
  #endif
    jpee_clock(mySCL);
    jpee_data(mySDA);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<std::size_t device_flash_size, std::size_t device_page_size>
void MicroChip24LC<device_flash_size, device_page_size>
::systemReset()
{
  myPageHit.fill(false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<std::size_t device_flash_size, std::size_t device_page_size>
void MicroChip24LC<device_flash_size, device_page_size>
::eraseAll()
{
  std::fill_n(myData.get(), FLASH_SIZE, INITIAL_VALUE);
  myDataChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<std::size_t device_flash_size, std::size_t device_page_size>
void MicroChip24LC<device_flash_size, device_page_size>
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
template<std::size_t device_flash_size, std::size_t device_page_size>
bool MicroChip24LC<device_flash_size, device_page_size>
::isPageUsed(uInt32 page) const
{
  return (page < PAGE_NUM) ? myPageHit[page] : false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<std::size_t device_flash_size, std::size_t device_page_size>
void MicroChip24LC<device_flash_size, device_page_size>
::jpee_init()
{
  jpee_sdat = 1;
  jpee_address = 0;
  jpee_state = 0;
  jpee_sizemask = FLASH_SIZE - 1;
  jpee_pagemask = PAGE_SIZE - 1;
  jpee_logmode = -1;
  jpee_packet.fill(0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<std::size_t device_flash_size, std::size_t device_page_size>
void MicroChip24LC<device_flash_size, device_page_size>
::jpee_data_start()
{
  /* We have a start condition */
  if(jpee_state == 1 && (jpee_nb != 1 || jpee_pptr != 3))
  {
    JPEE_LOG0("I2C_WARNING ABANDON WRITE")
    jpee_ad_known = 0;
  }
  if(jpee_state == 3)
  {
    JPEE_LOG0("I2C_WARNING ABANDON READ")
  }
  if(!jpee_timercheck(0))
  {
    JPEE_LOG0("I2C_START")
    jpee_state = 2;
  }
  else
  {
    JPEE_LOG0("I2C_BUSY")
    jpee_state = 0;
  }
  jpee_pptr = 0;
  jpee_nb = 0;
  jpee_packet[0] = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<std::size_t device_flash_size, std::size_t device_page_size>
void MicroChip24LC<device_flash_size, device_page_size>
::jpee_data_stop()
{
  if(jpee_state == 1 && jpee_nb != 1)
  {
    JPEE_LOG0("I2C_WARNING ABANDON_WRITE")
    jpee_ad_known = 0;
  }
  if(jpee_state == 3)
  {
    JPEE_LOG0("I2C_WARNING ABANDON_READ")
    jpee_ad_known = 0;
  }
  /* We have a stop condition. */
  if(jpee_state == 1 && jpee_nb == 1 && jpee_pptr > 3)
  {
    jpee_timercheck(1);
    JPEE_LOG2("I2C_STOP(Write %d bytes at %04X)", jpee_pptr - 3, jpee_address)
    if(((jpee_address + jpee_pptr - 4) ^ jpee_address) & ~jpee_pagemask)
    {
      jpee_pptr = 4 + jpee_pagemask - (jpee_address & jpee_pagemask);
      JPEE_LOG1("I2C_WARNING PAGECROSSING!(Truncate to %d bytes)", jpee_pptr - 3)
    }
    for(int i = 3; i < jpee_pptr; i++)
    {
      myDataChanged = true;
      myPageHit[jpee_address / PAGE_SIZE] = true;

      if(myCallback)
      {
        if(jpee_smallmode)
          myCallback("Cartridge EEPROM write");
        else
          myCallback("AtariVox/SaveKey EEPROM write");
      }

      myData[(jpee_address++) & jpee_sizemask] = jpee_packet[i];
      if(!(jpee_address & jpee_pagemask))
        break;  /* Writes can't cross page boundary! */
    }
    jpee_ad_known = 0;
  }
  else
  {
  #ifdef DEBUG_EEPROM
    jpee_logproc("I2C_STOP")
  #endif
    jpee_timercheck(1);
  }

  jpee_state = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<std::size_t device_flash_size, std::size_t device_page_size>
void MicroChip24LC<device_flash_size, device_page_size>
::jpee_clock_fall()
{
  switch(jpee_state)
  {
    case 1:
      jpee_nb <<= 1;
      jpee_nb |= jpee_mdat;
      if(jpee_nb & 256)
      {
        if(!jpee_pptr)
        {
          jpee_packet[0] = static_cast<uInt8>(jpee_nb);
          if(jpee_smallmode && ((jpee_nb & 0xF0) == 0xA0))
          {
            jpee_packet[1] = (jpee_nb >> 1) & 7;
          #ifdef DEBUG_EEPROM
            if(jpee_packet[1] != (jpee_address >> 8) && (jpee_packet[0] & 1))
              jpee_logproc("I2C_WARNING ADDRESS MSB CHANGED")
          #endif
            jpee_nb &= 0x1A1;
          }
          if(jpee_nb == 0x1A0)
          {
            JPEE_LOG1("I2C_SENT(%02X--start write)",jpee_packet[0])
            jpee_state = 2;
            jpee_sdat = 0;
          }
          else if(jpee_nb == 0x1A1)
          {
            jpee_state = 4;
            JPEE_LOG2("I2C_SENT(%02X--start read @%04X)", jpee_packet[0],jpee_address)
          #ifdef DEBUG_EEPROM
            if(!jpee_ad_known)
              jpee_logproc("I2C_WARNING ADDRESS IS UNKNOWN")
          #endif
            jpee_sdat = 0;
          }
          else
          {
            JPEE_LOG1("I2C_WARNING ODDBALL FIRST BYTE!(%02X)",jpee_nb & 0xFF)
            jpee_state = 0;
          }
        }
        else
        {
          jpee_state = 2;
          jpee_sdat = 0;
        }
      }
      break;

    case 2:
      if(jpee_nb)
      {
        if(!jpee_pptr)
        {
          jpee_packet[0] = static_cast<uInt8>(jpee_nb);
          if(jpee_smallmode)
            jpee_pptr = 2;
          else
            jpee_pptr = 1;
        }
        else if(jpee_pptr < 70)
        {
          JPEE_LOG1("I2C_SENT(%02X)", jpee_nb & 0xFF)
          jpee_packet[jpee_pptr++] = static_cast<uInt8>(jpee_nb);
          jpee_address = (jpee_packet[1] << 8) | jpee_packet[2];
          if(jpee_pptr > 2)
            jpee_ad_known = 1;
        }
      #ifdef DEBUG_EEPROM
        else
          jpee_logproc("I2C_WARNING OUTPUT_OVERFLOW!")
      #endif
      }
      jpee_sdat = 1;
      jpee_nb = 1;
      jpee_state = 1;
      break;

    case 4:
      if(jpee_mdat && jpee_sdat)
      {
        JPEE_LOG0("I2C_READ_NAK")
        jpee_state = 0;
        break;
      }
      jpee_state = 3;
      myPageHit[jpee_address / PAGE_SIZE] = true;

      if(myCallback)
      {
        if(jpee_smallmode)
          myCallback("Cartridge EEPROM read");
        else
          myCallback("AtariVox/SaveKey EEPROM read");
      }

      jpee_nb = (myData[jpee_address & jpee_sizemask] << 1) | 1;  /* Fall through */
      JPEE_LOG2("I2C_READ(%04X=%02X)",jpee_address,jpee_nb/2)

      [[fallthrough]];

    case 3:
      jpee_sdat = !!(jpee_nb & 256);
      jpee_nb <<= 1;
      if(!(jpee_nb & 510))
      {
        jpee_state = 4;
        jpee_sdat = 1;
        ++jpee_address;
      }
      break;

    default:
      /* Do nothing */
      break;
  }
  JPEE_LOG2("I2C_CLOCK (dat=%d/%d)",jpee_mdat,jpee_sdat)
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<std::size_t device_flash_size, std::size_t device_page_size>
bool MicroChip24LC<device_flash_size, device_page_size>
::jpee_timercheck(int mode)
{
  /*
    Evaluate how long the EEPROM is busy.  When invoked with an argument of 1,
    start a timer (probably about 5 milliseconds); when invoked with an
    argument of 0, return zero if the timer has expired or non-zero if it is
    still running.
  */
  if(mode)  // set timer
  {
    myCyclesWhenTimerSet = mySystem.cycles();
    return myTimerActive = true;
  }
  else      // read timer
  {
    /*
      Not sure why this timer isn't working with EFF/Grizzards but
      blocking it out works fine in practice, may not emulate an
      actual timing bug if one occurs however. -BRP
    */
    if(jpee_smallmode)
      return false;
    if(myTimerActive)
    {
      const uInt64 elapsed = mySystem.cycles() - myCyclesWhenTimerSet;
      myTimerActive = elapsed < static_cast<uInt64>(5000000.0 / 838.0);
    }
    return myTimerActive;
  }
}

#endif  // MicroChip24LC

