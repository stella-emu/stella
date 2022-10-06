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

#ifndef TIMER_MAP_HXX
#define TIMER_MAP_HXX

#include <map>
#include <deque>

#include "bspf.hxx"

/**
  This class handles debugger timers. Each timer needs a 'from' and a 'to'
  address.

  @author  Thomas Jentzsch
*/
class TimerMap
{
  public:
    static constexpr uInt8 ANY_BANK = 255;  // timer breakpoint valid in any bank

  //private:
    static constexpr uInt16 ADDRESS_MASK = 0x1fff;  // either 0x1fff or 0xffff (not needed then)

  private:
    struct TimerPoint
    {
      uInt16 addr{0};
      uInt8  bank{ANY_BANK};

      explicit constexpr TimerPoint(uInt16 c_addr, uInt8 c_bank, bool mask = true)
        : addr{c_addr}, bank{c_bank}
      {
        if(mask && bank != ANY_BANK)
          addr = addr & ADDRESS_MASK;
      }
      TimerPoint()
        : addr{0}, bank(ANY_BANK) {}

#if 0 // unused
      bool operator==(const TimerPoint& other) const
      {
        if(addr == other.addr)
        {
          if(bank == ANY_BANK || other.bank == ANY_BANK)
            return true;
          else
            return bank == other.bank;
        }
        return false;
      }
#endif

      bool operator<(const TimerPoint& other) const
      {
        if(bank == ANY_BANK || other.bank == ANY_BANK)
          return addr < other.addr;

        return bank < other.bank || (bank == other.bank && addr < other.addr);
      }
    };

  public:
    struct Timer
    {
      TimerPoint from{};
      TimerPoint to{};
      bool       isPartial{false};

      uInt64 execs{0};
      uInt64 lastCycles{0};
      uInt64 totalCycles{0};
      uInt64 minCycles{ULONG_MAX};
      uInt64 maxCycles{0};
      bool   isStarted{false};

      explicit constexpr Timer(const TimerPoint& c_from, const TimerPoint& c_to)
        : from{c_from}, to{c_to}
      {
        //if(to.bank == ANY_BANK) // TODO: check if this is required
        //{
        //  to.bank = from.bank;
        //  if(to.bank != ANY_BANK)
        //    to.addr &= ADDRESS_MASK;
        //}
      }

      Timer(uInt16 fromAddr, uInt16 toAddr, uInt8 fromBank, uInt8 toBank)
      {
        Timer(TimerPoint(fromAddr, fromBank), TimerPoint(fromAddr, fromBank));
      }

      Timer(const TimerPoint& tp)
      {
        if(!isPartial)
        {
          from = tp;
          isPartial = true;
        }
        else
        {
          to = tp;
          isPartial = false;

          //if(to.bank == ANY_BANK) // TODO: check if this is required
          //{
          //  to.bank = from.bank;
          //  if(to.bank != ANY_BANK)
          //    to.addr &= ADDRESS_MASK;
          //}
        }
      }

      Timer(uInt16 addr, uInt8 bank)
      {
        Timer(TimerPoint(addr, bank));
      }

#if 0 // unused
      bool operator==(const Timer& other) const
      {
        cerr << from.addr << ", " << to.addr << endl;
        if(from.addr == other.from.addr && to.addr == other.to.addr)
        {
          if((from.bank == ANY_BANK || other.from.bank == ANY_BANK) &&
            (to.bank == ANY_BANK || other.to.bank == ANY_BANK))
            return true;
          else
            return from.bank == other.from.bank && to.bank == other.to.bank;
        }
        return false;
      }

      bool operator<(const Timer& other) const
      {
        if(from.bank < other.from.bank || (from.bank == other.from.bank && from.addr < other.from.addr))
          return true;

        if(from.bank == other.from.bank && from.addr == other.from.addr)
          return to.bank < other.to.bank || (to.bank == other.to.bank && to.addr < other.to.addr);

        return false;
      }
#endif

      void setTo(const TimerPoint& tp)
      {
        to = tp;
        isPartial = false;

        //if(to.bank == ANY_BANK) // TODO: check if this is required
        //{
        //  to.bank = from.bank;
        //  if(to.bank != ANY_BANK)
        //    to.addr &= ADDRESS_MASK;
        //}
      }

      void reset()
      {
        execs = lastCycles = totalCycles = maxCycles = 0;
        minCycles = ULONG_MAX;
      }

      // Start the timer
      void start(uInt64 cycles)
      {
        lastCycles = cycles;
        isStarted = true;
      }

      // Stop the timer and update stats
      void stop(uInt64 cycles)
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

      uInt32 averageCycles() const {
        return execs ? std::round(totalCycles / execs) : 0; }
    }; // Timer

    explicit TimerMap() = default;

    bool isInitialized() const { return myList.size(); }

    /** Add new timer */
    uInt32 add(const uInt16 fromAddr, const uInt16 toAddr,
               const uInt8 fromBank, const uInt8 toBank);
    uInt32 add(const uInt16 addr, const uInt8 bank);

    /** Erase timer */
    bool erase(const uInt32 idx);

    /** Clear all timers */
    void clear();

    /** Reset all timers */
    void reset();

    /** Get timer */
    const Timer& get(const uInt32 idx) const { return myList[idx]; };
    uInt32 size() const { return static_cast<uInt32>(myList.size()); }

    /** Update timer */
    void update(const uInt16 addr, const uInt8 bank,
                const uInt64 cycles);

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
