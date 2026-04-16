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

#ifndef TIMER_MAP_HXX
#define TIMER_MAP_HXX

#include <cmath>
#include <climits>
#include <map>
#include <deque>

#include "bspf.hxx"
#include "Serializable.hxx"

/**
  This class handles debugger timers. Each timer needs a 'from' and a 'to'
  address.

  @author  Thomas Jentzsch
*/
class TimerMap : public Serializable
{
  private:
    static constexpr uInt16 ADDRESS_MASK = 0x1fff;  // either 0x1fff or 0xffff (not needed then)
    static constexpr uInt8 ANY_BANK = 255;  // timer point valid in any bank

  private:
    struct TimerPoint
    {
      uInt16 addr{0};
      uInt8  bank{ANY_BANK};

      constexpr TimerPoint() = default;
      explicit constexpr TimerPoint(uInt16 c_addr, uInt8 c_bank)
        : addr{c_addr}, bank{c_bank} { }

      constexpr bool operator<(const TimerPoint& other) const
      {
        if(bank == ANY_BANK || other.bank == ANY_BANK)
          return addr < other.addr;

        return bank < other.bank || (bank == other.bank && addr < other.addr);
      }
    };

  public:
    struct Timer : public Serializable
    {
      TimerPoint from;
      TimerPoint to;
      bool   mirrors{false};
      bool   anyBank{false};
      bool   isPartial{false};

      uInt64 execs{0};
      uInt64 lastCycles{0};
      uInt64 totalCycles{0};
      uInt64 minCycles{ULONG_MAX};
      uInt64 maxCycles{0};
      bool   isStarted{false};

      /*
        Create full timer
      */

      explicit constexpr Timer(const TimerPoint& c_from, const TimerPoint& c_to,
                               bool c_mirrors = false, bool c_anyBank = false)
        : from{c_from}, to{c_to}, mirrors{c_mirrors}, anyBank{c_anyBank} { }

      /*
        Create half timer (start point only)
      */
      explicit constexpr Timer(const TimerPoint& tp, bool c_mirrors = false,
                               bool c_anyBank = false)
        : from{tp}, mirrors{c_mirrors}, anyBank{c_anyBank}, isPartial{true} { }

      /*
        Define timer end point
      */
      void setTo(const TimerPoint& tp, bool c_mirrors = false,
                 bool c_anyBank = false)
      {
        to = tp;
        mirrors |= c_mirrors;
        anyBank |= c_anyBank;
        isPartial = false;
      }

      /*
        Reset the timer
      */
      constexpr void reset()
      {
        execs = lastCycles = totalCycles = maxCycles = 0;
        minCycles = ULONG_MAX;
      }

      /*
        Start the timer
      */
      constexpr void start(uInt64 cycles)
      {
        lastCycles = cycles;
        isStarted = true;
      }

      /*
        Stop the timer and update stats
      */
      constexpr void stop(uInt64 cycles)
      {
        if(isStarted)
        {
          const uInt64 diffCycles = cycles - lastCycles;

          ++execs;
          totalCycles += diffCycles;
          minCycles = std::min(minCycles, diffCycles);
          maxCycles = std::max(maxCycles, diffCycles);
          isStarted = false;
        }
      }

      constexpr uInt32 averageCycles() const {
        return execs ? static_cast<uInt32>(std::llround(
            static_cast<double>(totalCycles) / execs)) : 0;
      }

      bool save(Serializer& out) const
      {
        try
        {
          out.putInt(from.addr);
          out.putShort(from.bank);
          out.putInt(to.addr);
          out.putShort(to.bank);

          out.putBool(mirrors);
          out.putBool(anyBank);
          out.putBool(isPartial);

          out.putLong(execs);
          out.putLong(lastCycles);
          out.putLong(totalCycles);
          out.putLong(minCycles);
          out.putLong(maxCycles);
          out.putBool(isStarted);
        }
        catch(...)
        {
          cerr << "ERROR: Timer::save\n";
          return false;
        }

        return true;
      }

      bool load(Serializer& in)
      {
        try
        {
          from.addr = in.getInt();
          from.bank = in.getShort();
          to.addr = in.getInt();
          to.bank = in.getShort();

          mirrors = in.getBool();
          anyBank = in.getBool();
          isPartial = in.getBool();

          execs = in.getLong();
          lastCycles = in.getLong();
          totalCycles = in.getLong();
          minCycles = in.getLong();
          maxCycles = in.getLong();
          isStarted = in.getBool();
        }
        catch(...)
        {
          cerr << "ERROR: Timer::load\n";
          return false;
        }

        return true;
      }
    }; // Timer

    explicit TimerMap() = default;
    ~TimerMap() = default;

    bool isInitialized() const { return !myList.empty(); }

    /** Add new timer */
    uInt32 add(uInt16 fromAddr, uInt16 toAddr,
               uInt8 fromBank, uInt8 toBank,
               bool mirrors, bool anyBank);
    uInt32 add(uInt16 addr, uInt8 bank,
               bool mirrors, bool anyBank);

    /** Erase timer */
    bool erase(uInt32 idx);

    /** Clear all timers */
    void clear();

    /** Reset all timers */
    void reset();

    /** Get timer */
    const Timer& get(uInt32 idx) const { return myList[idx]; }
    uInt32 size() const { return static_cast<uInt32>(myList.size()); }

    /** Update timer */
    void update(uInt16 addr, uInt8 bank, uInt64 cycles);

    /**
    Save the current state of this cart to the given Serializer.

    @param out  The Serializer object to use
    @return  False on any errors, else true
    */
    bool save(Serializer& out) const override;

    /**
    Load the current state of this cart from the given Serializer.

    @param in  The Serializer object to use
    @return  False on any errors, else true
    */
    bool load(Serializer& in) override;

  private:
    static void toKey(TimerPoint& tp, bool mirrors, bool anyBank);

  private:
    using TimerList = std::deque<Timer>; // makes sure that the element pointers do NOT change
    using TimerPair = std::pair<TimerPoint, Timer*>;
    using FromMap = std::multimap<TimerPoint, Timer*>;
    using ToMap = std::multimap<TimerPoint, Timer*>;

    TimerList myList;
    FromMap myFromMap;
    ToMap myToMap;

    // Following constructors and assignment operators not supported
    TimerMap(const TimerMap&) = delete;
    TimerMap(TimerMap&&) = delete;
    TimerMap& operator=(const TimerMap&) = delete;
    TimerMap& operator=(TimerMap&&) = delete;
};

#endif
