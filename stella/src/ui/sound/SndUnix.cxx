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
// Copyright (c) 1995-1998 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: SndUnix.cxx,v 1.1.1.1 2001-12-27 19:54:35 bwmott Exp $
//============================================================================

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>

#include "SndUnix.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundUnix::SoundUnix()
    : myDisabled(false),
      myMute(false)
{
  // Initialize to impossible values so they will be reset 
  // on the first call to the set method
  myAUDC0 = myAUDC1 = myAUDF0 = myAUDF1 = myAUDV0 = myAUDV1 = 255;

  int pfd[2];

  // Create pipe for interprocess communication
  if(pipe(pfd))
  {
    // Oops. We were not able to create pipe so disable myself and return
    myDisabled = true;
    return;
  }

  // Create new process, setup pipe, and start stella-sound
  myProcessId = fork();

  if(myProcessId == 0)
  {
    // Close STDIN and put the read end of the pipe in its place
    dup2(pfd[0], 0);

    // Close unused file descriptors in child process
    close(pfd[0]);
    close(pfd[1]);
 
    // Execute the stella-sound server
    if(execlp("stella-sound", "stella-sound", (char*)0))
    {
      exit(1);
    }
  }
  else if(myProcessId > 0)
  {
    // Close unused file descriptors in parent process
    close(pfd[0]);

    // Save the pipe's write file descriptor
    myFd = pfd[1];
  }
  else
  {
    // Couldn't fork so cleanup and disabled myself
    close(pfd[0]);
    close(pfd[1]);

    myDisabled = true;
  }
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundUnix::~SoundUnix()
{
  if(!myDisabled)
  {
    // Send quit command to the sound server
    unsigned char command = 0xC0;
    write(myFd, &command, 1);

    // Kill the sound server
    kill(myProcessId, SIGHUP);

    // Close descriptors
    close(myFd);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundUnix::set(Sound::Register reg, uInt8 value)
{
  // Return if I'm currently disabled or if the stella-sound process has died
  if(myDisabled || (waitpid(myProcessId, 0, WNOHANG) == myProcessId))
  {
    myDisabled = true;
    return;
  }

  uInt8 command;

  switch(reg)
  {
    case AUDC0:
    {
      if(myAUDC0 != (value & 0x0f))
      {
        myAUDC0 = (value & 0x0f);
        command = myAUDC0 | 0x00;
      }
      else
      {
        return;
      } 
      break;
    }

    case AUDC1:
    {
      if(myAUDC1 != (value & 0x0f))
      {
        myAUDC1 = (value & 0x0f);
        command = myAUDC1 | 0x20;
      }
      else
      {
        return;
      } 
      break;
    }

    case AUDF0:
    {
      if(myAUDF0 != (value & 0x1f))
      {
        myAUDF0 = (value & 0x1f);
        command = myAUDF0 | 0x40;
      }
      else
      {
        return;
      } 
      break;
    }

    case AUDF1:
    {
      if(myAUDF1 != (value & 0x1f))
      {
        myAUDF1 = (value & 0x1f);
        command = myAUDF1 | 0x60;
      }
      else
      {
        return;
      } 
      break;
    }

    case AUDV0:
    {
      if(myAUDV0 != (value & 0x0f))
      {
        myAUDV0 = (value & 0x0f);
        command = myAUDV0 | 0x80;
      }
      else
      {
        return;
      } 
      break;
    }

    case AUDV1:
    {
      if(myAUDV1 != (value & 0x0f))
      {
        myAUDV1 = (value & 0x0f);
        command = myAUDV1 | 0xA0;
      }
      else
      {
        return;
      } 
      break;
    }

    default:
    {
      return;
      break;
    }
  }

  // Send sound command to the stella-sound process
  write(myFd, &command, 1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundUnix::mute(bool state)
{
  // Return if I'm currently disabled or if the stella-sound process has died
  if(myDisabled || (waitpid(myProcessId, 0, WNOHANG) == myProcessId))
  {
    myDisabled = true;
    return;
  }

  myMute = state;

  uInt8 command;
  if(myMute)
  {
    // Setup the mute enable command
    command = 0x01 | 0xE0;
  }
  else
  {
    // Setup the mute disable command
    command = 0x00 | 0xE0;
  }

  // Send sound command to the stella-sound process
  write(myFd, &command, 1);
}

