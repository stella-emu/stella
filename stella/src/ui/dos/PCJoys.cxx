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
// Copyright (c) 1995-1999 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: PCJoys.cxx,v 1.1.1.1 2001-12-27 19:54:32 bwmott Exp $
//============================================================================

#include <dos.h>
#include <time.h>
#include "PCJoys.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PCJoysticks::PCJoysticks(bool useAxisMidpoint)
    : myGamePort(0x0201),
      myUseAxisMidpoint(useAxisMidpoint)
{
  for(uInt32 i = 0; i < 4; ++i)
  {
    myMinimum[i] = 0x0FFFFFFF;
    myMaximum[i] = 0;
    myMidpoint[i] = 0;
  }

  // Determine which joysticks are present
  myPresent = detect(); 

  // If we're using midpoints then calibrate them now
  if(myUseAxisMidpoint)
  {
    calibrate();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PCJoysticks::~PCJoysticks()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PCJoysticks::present() const
{
  return myPresent;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PCJoysticks::read(bool button[4], Int16 axis[4])
{
  // Get the state of each of the buttons
  uInt8 b = (~inportb(myGamePort)) >> 4;
  button[0] = b & 0x01 ? true : false;
  button[1] = b & 0x02 ? true : false;
  button[2] = b & 0x04 ? true : false;
  button[3] = b & 0x08 ? true : false;

  // Count how long it takes each axis to change states
  Int32 count[4] = {0, 0, 0, 0};
  disable();
  outportb(myGamePort, 0);
  for(uInt32 counter = 0; counter < 0x000FFFFF; ++counter)
  {
    uInt8 i = inportb(myGamePort);

    if(i & 0x01)
      ++count[0];
    if(i & 0x02)
      ++count[1];
    if(i & 0x04)
      ++count[2];
    if(i & 0x08)
      ++count[3];
    if(!(i & myPresent))
      break;
  }
  enable();

  // Determine the position of each of the axes
  for(uInt32 t = 0; t < 4; ++t)
  {
    // Is this axis being used?
    if(myPresent & (1 << t))
    {
      // Update the maximum for this axis if needed
      if(count[t] > myMaximum[t])
        myMaximum[t] = count[t];

      // Update the minimum for this axis if needed
      if(count[t] < myMinimum[t])
        myMinimum[t] = count[t];

      // If the minimum and maximum aren't far enough apart then assume 0
      if(myMaximum[t] - myMinimum[t] < (myMaximum[t] >> 3))
      {
        axis[t] = 0;
      }
      else
      { 
        // Are we using midpoints?
        if(myUseAxisMidpoint)
        {
          // Yes, so calculate axis value using midpoint
          if(count[t] < myMidpoint[t])
          {
            double n = myMidpoint[t] - count[t];
            double d = myMidpoint[t] - myMinimum[t];
            double f = ((d == 0.0) ? 0.0 : (n / d));
            if(d < (myMaximum[t] >> 3))
              axis[t] = 0;
            else
              axis[t] = (Int16)((Int32)(-32767L * f));
          }
          else
          {
            double n = count[t] - myMidpoint[t];
            double d = myMaximum[t] - myMidpoint[t];
            double f = ((d == 0.0) ? 0.0 : (n / d));
            if(d < (myMaximum[t] >> 3))
              axis[t] = 0;
            else
              axis[t] = (Int16)((Int32)(32767L * f));
          }
        }
        else
        {
          // No, so calculate axis value without using the midpoint
          double n = count[t] - myMinimum[t];
          double d = myMaximum[t] - myMinimum[t];
          double f = ((d == 0.0) ? 0.5 : (n / d));
          axis[t] = (Int16)((Int32)(65534L * f) - 32767);
        }
      }
    }
    else
    {
      axis[t] = 0;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 PCJoysticks::detect() const
{
  uInt8 present = 0;

  // Reset the game port bits to 1's
  outportb(myGamePort, 0);

  clock_t start = clock();

  // Wait for low on all four joystick bits or for time to expire
  do
  {
    present = inportb(myGamePort) & 0x0F;
  }
  while(((clock() - start) < (CLOCKS_PER_SEC / 2)) && present);
  
  // Setup joystick present mask that tells which joysticks are present
  present = (present ^ 0x0F) & 0x0F;
  present = present & (((present & 0x03) == 0x03) ? 0x0F : 0x0C); 
  present = present & (((present & 0x0C) == 0x0C) ? 0x0F : 0x03); 

  return present;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PCJoysticks::calibrate()
{
  // Make sure a joystick is present before we do a lot of work 
  if(present())
  { 
    // Count how long it takes each axis to change states
    Int32 count[4] = {0, 0, 0, 0};
    disable();
    outportb(myGamePort, 0);
    for(uInt32 counter = 0; counter < 0x000FFFFF; ++counter)
    {
      uInt8 i = inportb(myGamePort);

      if(i & 0x01)
        ++count[0];
      if(i & 0x02)
        ++count[1];
      if(i & 0x04)
        ++count[2];
      if(i & 0x08)
        ++count[3];
      if(!(i & myPresent))
        break;
    }
    enable();

    for(uInt32 t = 0; t < 4; ++t)
    {
      myMidpoint[t] = count[t];
    }
  }
}

