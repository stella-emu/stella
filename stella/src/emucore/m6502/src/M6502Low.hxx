//============================================================================
//
// MM     MM  6666  555555  0000   2222
// MMMM MMMM 66  66 55     00  00 22  22
// MM MMM MM 66     55     00  00     22
// MM  M  MM 66666  55555  00  00  22222  --  "A 6502 Microprocessor Emulator"
// MM     MM 66  66     55 00  00 22
// MM     MM 66  66 55  55 00  00 22
// MM     MM  6666   5555   0000  222222
//
// Copyright (c) 1995-1998 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: M6502Low.hxx,v 1.1.1.1 2001-12-27 19:54:31 bwmott Exp $
//============================================================================

#ifndef M6507LOW_HXX
#define M6507LOW_HXX

class M6507Low;

#include "bspf.hxx"
#include "M6502.hxx"

/**
  This class provides a low compatibility 6502 microprocessor emulator.  
  The memory accesses and cycle updates of this emulator are not 100% 
  accurate as shown below:

    1. Only memory accesses which are actually needed are done 
       (i.e. no "false" reads and writes are performed)

    2. Cycle counts are updated at the beginning of the instruction
       execution and not valid at the sub-instruction level

  If speed is the most important issue then use this class, however, if 
  better compatibility is neccessary use one of the other 6502 classes.
  
  @author  Bradford W. Mott
  @version $Id: M6502Low.hxx,v 1.1.1.1 2001-12-27 19:54:31 bwmott Exp $
*/
class M6502Low : public M6502
{
  public:
    /**
      Create a new low compatibility 6502 microprocessor with the specified 
      cycle multiplier.

      @param systemCyclesPerProcessorCycle The cycle multiplier
    */
    M6502Low(uInt32 systemCyclesPerProcessorCycle);

    /**
      Destructor
    */
    virtual ~M6502Low();

  public:
    /**
      Execute instructions until the specified number of instructions
      is executed, someone stops execution, or an error occurs.  Answers
      true iff execution stops normally.

      @param number Indicates the number of instructions to execute
      @return true iff execution stops normally
    */
    virtual bool execute(uInt32 number);

  protected:
    /**
      Called after an interrupt has be requested using irq() or nmi()
    */
    void interruptHandler();

  protected:
    /*
      Get the byte at the specified address 

      @return The byte at the specified address
    */
    inline uInt8 peek(uInt16 address);

    /**
      Change the byte at the specified address to the given value

      @param address The address where the value should be stored
      @param value The value to be stored at the address
    */
    inline void poke(uInt16 address, uInt8 value);
};
#endif

