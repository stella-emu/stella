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

#include "FSNode.hxx"
#include "Settings.hxx"
#include "System.hxx"
#ifdef DEBUGGER_SUPPORT
  #include "Debugger.hxx"
  #include "Base.hxx"
#endif

#include "Cart.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge::Cartridge(const Settings& settings, string_view md5)
  : mySettings{settings}
{
  const uInt32 seed =
    BSPF::stoi<16>(md5.substr(0, 8))  ^ BSPF::stoi<16>(md5.substr(8, 8)) ^
    BSPF::stoi<16>(md5.substr(16, 8)) ^ BSPF::stoi<16>(md5.substr(24, 8));

  const Random rand(seed);
  for(uInt32 i = 0; i < 256; ++i)
    myRWPRandomValues[i] = rand.next();

  const bool devSettings = mySettings.getBool("dev.settings");
  myRandomHotspots = devSettings ? mySettings.getBool("dev.randomhs") : false;
  myRamReadAccesses.reserve(5);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge::setProperties(const Properties* props)
{
  myProperties = props;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge::setAbout(string_view about, string_view type, string_view id)
{
  myAbout = about;
  myDetectedType = type;
  myMultiCartID = id;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge::saveROM(const FSNode& out) const
{
  try
  {
    const ByteSpan image = getImage();
    if(image.empty())
    {
      cerr << "save not supported\n";
      return false;
    }
    out.write(image);
  }
  catch(...)
  {
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge::bankChanged()
{
  const bool changed = myBankChanged;
  myBankChanged = false;
  return changed;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 Cartridge::bankSize(uInt16 bank) const
{
  return static_cast<uInt16>(
    std::min(getImage().size() / romBankCount(), 4_KB)); // assuming that each bank has the same size
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 Cartridge::peekRAM(uInt8& dest, uInt16 address)
{
  const uInt8 value = myRWPRandomValues[address & 0xFF];

  // Reading from the write port triggers an unwanted write
  // But this only happens when in normal emulation mode
#ifdef DEBUGGER_SUPPORT
  if(!hotspotsLocked() && !mySystem->autodetectMode())
  {
    // Record access here; final determination will happen in ::pokeRAM()
    myRamReadAccesses.push_back(address);
    dest = value;
  }
#else
  if(!mySystem->autodetectMode())
    dest = value;
#endif
  return value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge::pokeRAM(uInt8& dest, uInt16 address, uInt8 value)
{
#ifdef DEBUGGER_SUPPORT
  // Compare with target address and with one page before (in case of page crossed while indexing)
  if(const auto it = std::ranges::find_if(myRamReadAccesses,
      [address](uInt16 a) { return a == address || a == address - 256; });
      it != myRamReadAccesses.end())
  {
    myRamReadAccesses.erase(it);
  }
#endif
  dest = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge::createRomAccessArrays(size_t size)
{
  myAccessSize = static_cast<uInt32>(size);

  // Always create ROM access base even if DEBUGGER_SUPPORT is disabled,
  // since other parts of the code depend on it existing
  myRomAccessBase = std::make_unique<Device::AccessFlags[]>(size);
  std::fill_n(myRomAccessBase.get(), size, Device::ROW);
  myRomAccessCounter = std::make_unique<Device::AccessCounter[]>(size * 2);
  std::fill_n(myRomAccessCounter.get(), size * 2, 0);
}

#ifdef DEBUGGER_SUPPORT
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Cartridge::getAccessCounters() const
{
  string out;
  uInt32 offset = 0;

  const string lastBank = Common::Base::toString(romBankCount() - 1,
    Common::Base::Fmt::_10_8);

  for(uInt16 bank = 0; bank < romBankCount(); ++bank)
  {
    const uInt16 origin  = bankOrigin(bank);
    const uInt16 bankSz  = this->bankSize(bank);
    const string bankStr = Common::Base::toString(bank, Common::Base::Fmt::_10_8);
    const string header  = std::format("Bank {} / 0..{}", bankStr, lastBank);

    out += header + " reads:\n";
    for(uInt16 addr = 0; addr < bankSz; ++addr)
      out += std::format("{},{}, ",
        Common::Base::toString(addr | origin, Common::Base::Fmt::_16_4),
        Common::Base::toString(myRomAccessCounter[offset + addr],
                              Common::Base::Fmt::_10_8));
    out += "\n";

    out += header + " writes:\n";
    for(uInt16 addr = 0; addr < bankSz; ++addr)
      out += std::format("{},{}, ",
        Common::Base::toString(addr | origin, Common::Base::Fmt::_16_4),
        Common::Base::toString(myRomAccessCounter[offset + addr + myAccessSize],
                              Common::Base::Fmt::_10_8));
    out += "\n";

    offset += bankSz;
  }
  return out;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 Cartridge::bankOrigin(uInt16 bank, uInt16 PC) const
{
  // Isolate the high 3 address bits, count them, include the PC if provided
  // and select the most frequent to define the bank origin
  // TODO: origin for banks smaller than 4K
  constexpr int intervals = 0x8000 / 0x100;
  const uInt32 offset = bank * bankSize();
  //uInt16 addrMask = (4_KB - 1) & ~(bankSize(bank) - 1);
  //int addrShift = 0;
  std::array<uInt16, intervals> count{}; // up to 128 256 byte interval origins

  //if(addrMask)
  //  addrShift = log(addrMask) / log(2);
  //addrMask;

  if(PC)
    count[PC >> 13]++;
  for(uInt16 addr = 0x0000; addr < bankSize(bank); ++addr)
  {
    const Device::AccessFlags flags = myRomAccessBase[offset + addr];
    // only count really accessed addresses
    if(flags & ~Device::ROW)
    {
      //uInt16 addrBit = addr >> addrShift;
      count[(flags & Device::HADDR) >> 13]++;
    }
  }
  uInt16 max = 0, maxIdx = 0;
  for(int idx = 0; idx < intervals; ++idx)
  {
    if(count[idx] > max)
    {
      max = count[idx];
      maxIdx = idx;
    }
  }
  return maxIdx << 13 | 0x1000; //| (offset & 0xfff);
}
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge::initializeRAM(ByteMSpan arr, uInt8 val) const
{
  if(randomInitialRAM())
    std::ranges::generate(arr,
      [this]{ return mySystem->randGenerator().next(); });
  else
    std::fill(arr.begin(), arr.end(), val);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 Cartridge::initializeStartBank(uInt16 defaultBank)
{
  const int propsBank = myStartBankFromPropsFunc();

  if(randomStartBank())
    return myStartBank = mySystem->randGenerator().next() % romBankCount();
  else if(propsBank >= 0)
    return myStartBank = BSPF::clamp(propsBank, 0, romBankCount() - 1);
  else
    return myStartBank = BSPF::clamp(static_cast<int>(defaultBank), 0, romBankCount() - 1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge::randomInitialRAM() const
{
  return mySettings.getBool(mySettings.getBool("dev.settings") ? "dev.ramrandom" : "plr.ramrandom");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge::randomStartBank() const
{
  return mySettings.getBool(mySettings.getBool("dev.settings") ? "dev.bankrandom" : "plr.bankrandom");
}
