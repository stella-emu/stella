##============================================================================
##
##   SSSS    tt          lll  lll       
##  SS  SS   tt           ll   ll        
##  SS     tttttt  eeee   ll   ll   aaaa 
##   SSSS    tt   ee  ee  ll   ll      aa
##      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
##  SS  SS   tt   ee      ll   ll  aa  aa
##   SSSS     ttt  eeeee llll llll  aaaaa
##
## Copyright (c) 1995-1999 by Bradford W. Mott
##
## See the file "license" for information on usage and redistribution of
## this file, and for a DISCLAIMER OF ALL WARRANTIES.
##
## $Id: makefile,v 1.51 2004-04-21 16:27:14 stephena Exp $
##============================================================================

##============================================================================
## Development environment options
##
## The following options are used during compiling.
## Comment a line out to disable that option, remove comment to enable it.
##============================================================================

### add your own compiler optimizations here
### if none are provided, the defaults will be used
OPTIMIZATIONS  = 

### to include joystick support
 JOYSTICK_SUPPORT = 1

### to include support for saving snapshots in png format
### (requires PNG library)
 SNAPSHOT_SUPPORT = 1

### to include support for game developers
### enables some extra commandline options that allow the user
### to override some emulation defaults
 DEVELOPER_SUPPORT = 1

### to build on SMP (or distcc-based) machines
### change to number of CPU's you have
 NUMBER_CPU = 1

##============================================================================
## All done, type make to get a list of frontends
## No configurable options below this line ...
##============================================================================

CXX	= g++
LD	= g++

LDFLAGS  = `sdl-config --cflags`
LDLIBS   = `sdl-config --libs`
OBJECTS  =  mainSDL.o SoundSDL.o FrameBufferSDL.o FrameBufferSoft.o
OPTIONS  =
EXE_NAME =
SMP      =

SRC      = ..
CORE     = $(SRC)/emucore
UI       = $(SRC)/ui
COMMON   = $(SRC)/ui/common

INCLUDES  = -I. -I$(CORE) -I$(CORE)/m6502/src \
            -I$(CORE)/m6502/src/bspf/src -I$(COMMON) -I$(UI)/sdl -I$(UI)/sound \
            -I/sw/include

## set some sane optimizations if none have been provided
ifndef OPTIMIZATIONS
  ifdef CXXFLAGS
    OPTIMIZATIONS = $(CXXFLAGS)
  else
    OPTIMIZATIONS = -O2
  endif
endif

ifndef NUMBER_CPU
  SMP = -j1
else
  SMP = -j$(NUMBER_CPU)
endif

FLAGS = $(OPTIMIZATIONS) -Wall -Wunused $(INCLUDES) $(SYS_INCLUDES)

ifdef JOYSTICK_SUPPORT
  OPTIONS += -DJOYSTICK_SUPPORT
endif

ifdef SNAPSHOT_SUPPORT
  OBJECTS += Snapshot.o
  OPTIONS += -DSNAPSHOT_SUPPORT
  LDLIBS  += -lpng -lz
endif

ifdef DEVELOPER_SUPPORT
  OPTIONS += -DDEVELOPER_SUPPORT
endif


default:
	@echo ""
	@echo "To build Stella type: 'make <version>'"
	@echo "You should edit the makefile for extra options"
	@echo ""
	@echo "<version> is one of:"
	@echo ""
	@echo "  linux     Linux/UNIX version"
	@echo "  linux-gl  Linux/UNIX version with OpenGL support"
	@echo "  win32     Windows 9x/ME/2000/XP version"
	@echo "  win32-gl  Windows 9x/ME/2000/XP version with OpenGL support"
	@echo ""
	@echo "Hopefully new versions will be added soon!"
	@echo ""

linux:
	$(MAKE) $(SMP) stella \
          EXE_NAME="stella" \
          OPTIONS="$(OPTIONS) -DBSPF_UNIX -DUNIX -DHAVE_GETTIMEOFDAY" \
          OBJS="$(OBJECTS) SettingsUNIX.o"

linux-gl:
	$(MAKE) $(SMP) stella \
          EXE_NAME="stella" \
          LDFLAGS="$(LDFLAGS) -L/usr/X11R6/lib" \
          LDLIBS="$(LDLIBS) -lGL" \
          OPTIONS="$(OPTIONS) -DBSPF_UNIX -DUNIX -DHAVE_GETTIMEOFDAY -DDISPLAY_OPENGL" \
          OBJS="$(OBJECTS) FrameBufferGL.o SettingsUNIX.o"

win32:
	$(MAKE) $(SMP) stella \
          EXE_NAME="stella.exe" \
          OPTIONS="$(OPTIONS) -DBSPF_WIN32 -DWIN32" \
          OBJS="$(OBJECTS) SettingsWin32.o"

win32-gl:
	$(MAKE) $(SMP) stella \
          EXE_NAME="stella.exe" \
          LDLIBS="$(LDLIBS) -lopengl32" \
          OPTIONS="$(OPTIONS) -DBSPF_WIN32 -DWIN32 -DDISPLAY_OPENGL -DTEXTURES_ARE_LOST" \
          OBJS="$(OBJECTS) FrameBufferGL.o SettingsWin32.o"


###############################################################################
## List of "core" object files
###############################################################################
M6502_OBJS = D6502.o Device.o M6502.o M6502Low.o M6502Hi.o NullDev.o System.o

CORE_OBJS = Booster.o Cart.o Cart2K.o Cart3F.o Cart4K.o CartAR.o CartDPC.o \
            CartE0.o CartE7.o CartF4.o CartF4SC.o CartF6.o CartF6SC.o \
            CartF8.o CartF8SC.o CartFASC.o CartFE.o CartMC.o CartCV.o \
            CartMB.o Console.o Control.o Driving.o \
            Event.o Joystick.o Keyboard.o M6532.o MD5.o MediaSrc.o Paddles.o \
            Props.o PropsSet.o Random.o Sound.o Switches.o Settings.o TIA.o \
            Serializer.o Deserializer.o TIASound.o EventHandler.o FrameBuffer.o \
            $(M6502_OBJS)

stella: $(CORE_OBJS) $(OBJS)
	$(LD) -o $(EXE_NAME) $(CORE_OBJS) $(OBJS) $(LDFLAGS) $(LDLIBS)
	strip $(EXE_NAME)

M6502Low.ins: $(CORE)/m6502/src/M6502Low.m4 $(CORE)/m6502/src/M6502.m4
	m4 $(CORE)/m6502/src/M6502Low.m4 $(CORE)/m6502/src/M6502.m4 > M6502Low.ins

M6502Hi.ins: $(CORE)/m6502/src/M6502Hi.m4 $(CORE)/m6502/src/M6502.m4
	m4 $(CORE)/m6502/src/M6502Hi.m4 $(CORE)/m6502/src/M6502.m4 > M6502Hi.ins

M6502Low.o: M6502Low.ins
M6502Hi.o: M6502Hi.ins

clean:
	rm -f *.o stella stella.exe core

cleanall: clean
	rm -f M6502Low.ins M6502Hi.ins

Driving.o: $(CORE)/Driving.cxx $(CORE)/Driving.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/Driving.cxx

Event.o: $(CORE)/Event.cxx $(CORE)/Event.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/Event.cxx

EventHandler.o: $(CORE)/EventHandler.cxx $(CORE)/EventHandler.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/EventHandler.cxx

Control.o: $(CORE)/Control.cxx $(CORE)/Control.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/Control.cxx

Joystick.o: $(CORE)/Joystick.cxx $(CORE)/Joystick.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/Joystick.cxx

Keyboard.o: $(CORE)/Keyboard.cxx $(CORE)/Keyboard.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/Keyboard.cxx

Paddles.o: $(CORE)/Paddles.cxx $(CORE)/Paddles.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/Paddles.cxx

Booster.o: $(CORE)/Booster.cxx $(CORE)/Booster.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/Booster.cxx

Cart.o: $(CORE)/Cart.cxx $(CORE)/Cart.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/Cart.cxx

Cart2K.o: $(CORE)/Cart2K.cxx $(CORE)/Cart2K.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/Cart2K.cxx

Cart3F.o: $(CORE)/Cart3F.cxx $(CORE)/Cart3F.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/Cart3F.cxx

Cart4K.o: $(CORE)/Cart4K.cxx $(CORE)/Cart4K.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/Cart4K.cxx

CartAR.o: $(CORE)/CartAR.cxx $(CORE)/CartAR.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/CartAR.cxx

CartDPC.o: $(CORE)/CartDPC.cxx $(CORE)/CartDPC.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/CartDPC.cxx

CartE0.o: $(CORE)/CartE0.cxx $(CORE)/CartE0.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/CartE0.cxx

CartE7.o: $(CORE)/CartE7.cxx $(CORE)/CartE7.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/CartE7.cxx

CartF4.o: $(CORE)/CartF4.cxx $(CORE)/CartF4.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/CartF4.cxx

CartF4SC.o: $(CORE)/CartF4SC.cxx $(CORE)/CartF4SC.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/CartF4SC.cxx

CartF6.o: $(CORE)/CartF6.cxx $(CORE)/CartF6.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/CartF6.cxx

CartF6SC.o: $(CORE)/CartF6SC.cxx $(CORE)/CartF6SC.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/CartF6SC.cxx

CartF8.o: $(CORE)/CartF8.cxx $(CORE)/CartF8.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/CartF8.cxx

CartF8SC.o: $(CORE)/CartF8SC.cxx $(CORE)/CartF8SC.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/CartF8SC.cxx

CartFASC.o: $(CORE)/CartFASC.cxx $(CORE)/CartFASC.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/CartFASC.cxx

CartFE.o: $(CORE)/CartFE.cxx $(CORE)/CartFE.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/CartFE.cxx

CartMC.o: $(CORE)/CartMC.cxx $(CORE)/CartMC.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/CartMC.cxx

CartMB.o: $(CORE)/CartMB.cxx $(CORE)/CartMB.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/CartMB.cxx

CartCV.o: $(CORE)/CartCV.cxx $(CORE)/CartCV.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/CartCV.cxx

M6532.o: $(CORE)/M6532.cxx $(CORE)/M6532.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/M6532.cxx

TIA.o: $(CORE)/TIA.cxx $(CORE)/TIA.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/TIA.cxx

TIASound.o: $(CORE)/TIASound.c $(CORE)/TIASound.h
	$(CXX) -c -DWIN32 $(FLAGS) $(OPTIONS) $(CORE)/TIASound.c

Console.o: $(CORE)/Console.cxx $(CORE)/Console.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/Console.cxx

MD5.o: $(CORE)/MD5.cxx $(CORE)/MD5.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/MD5.cxx

MediaSrc.o: $(CORE)/MediaSrc.cxx $(CORE)/MediaSrc.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/MediaSrc.cxx

FrameBuffer.o: $(CORE)/FrameBuffer.cxx $(CORE)/FrameBuffer.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/FrameBuffer.cxx

PropsSet.o: $(CORE)/PropsSet.cxx $(CORE)/PropsSet.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/PropsSet.cxx

Props.o: $(CORE)/Props.cxx $(CORE)/Props.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/Props.cxx

Random.o: $(CORE)/Random.cxx $(CORE)/Random.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/Random.cxx

Sound.o: $(CORE)/Sound.cxx $(CORE)/Sound.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(CORE)/Sound.cxx

Switches.o: $(CORE)/Switches.cxx $(CORE)/Switches.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/Switches.cxx

Serializer.o: $(CORE)/Serializer.cxx $(CORE)/Serializer.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/Serializer.cxx

Deserializer.o: $(CORE)/Deserializer.cxx $(CORE)/Deserializer.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/Deserializer.cxx

Settings.o: $(CORE)/Settings.cxx $(CORE)/Settings.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(CORE)/Settings.cxx

SettingsUNIX.o: $(UI)/sdl/SettingsUNIX.cxx $(UI)/sdl/SettingsUNIX.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(UI)/sdl/SettingsUNIX.cxx

SettingsWin32.o: $(UI)/sdl/SettingsWin32.cxx $(UI)/sdl/SettingsWin32.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(UI)/sdl/SettingsWin32.cxx

Terminal.o: $(UI)/x11/Terminal.cxx $(UI)/x11/Terminal.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(UI)/x11/Terminal.cxx

mainDOS.o: $(UI)/dos/mainDOS.cxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(UI)/dos/mainDOS.cxx

PCJoys.o: $(UI)/dos/PCJoys.cxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(UI)/dos/PCJoys.cxx

SndDOS.o: $(UI)/dos/SndDOS.cxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(UI)/dos/SndDOS.cxx

dos_sb.o: $(UI)/dos/dos_sb.c
	$(CXX) -c $(FLAGS) $(OPTIONS) $(UI)/dos/dos_sb.c

vga.o: $(UI)/dos/vga.cxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(UI)/dos/vga.cxx

SoundALSA.o: $(UI)/sound/SoundALSA.cxx $(UI)/sound/SoundALSA.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(UI)/sound/SoundALSA.cxx

SoundOSS.o: $(UI)/sound/SoundOSS.cxx $(UI)/sound/SoundOSS.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(UI)/sound/SoundOSS.cxx

SoundSDL.o: $(UI)/sound/SoundSDL.cxx $(UI)/sound/SoundSDL.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(UI)/sound/SoundSDL.cxx

mainX11.o: $(UI)/x11/mainX11.cxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(UI)/x11/mainX11.cxx

mainSDL.o: $(UI)/sdl/mainSDL.cxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(UI)/sdl/mainSDL.cxx

RectList.o: $(UI)/sdl/RectList.cxx $(UI)/sdl/RectList.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(UI)/sdl/RectList.cxx

FrameBufferSDL.o: $(UI)/sdl/FrameBufferSDL.cxx $(UI)/sdl/FrameBufferSDL.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(UI)/sdl/FrameBufferSDL.cxx

FrameBufferSoft.o: $(UI)/sdl/FrameBufferSoft.cxx $(UI)/sdl/FrameBufferSoft.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(UI)/sdl/FrameBufferSoft.cxx

FrameBufferGL.o: $(UI)/sdl/FrameBufferGL.cxx $(UI)/sdl/FrameBufferGL.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(UI)/sdl/FrameBufferGL.cxx

Snapshot.o: $(UI)/common/Snapshot.cxx $(UI)/common/Snapshot.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(UI)/common/Snapshot.cxx

D6502.o: $(CORE)/m6502/src/D6502.cxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/m6502/src/D6502.cxx

Device.o: $(CORE)/m6502/src/Device.cxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/m6502/src/Device.cxx

M6502.o: $(CORE)/m6502/src/M6502.cxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/m6502/src/M6502.cxx

M6502Low.o: $(CORE)/m6502/src/M6502Low.cxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/m6502/src/M6502Low.cxx

M6502Hi.o: $(CORE)/m6502/src/M6502Hi.cxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/m6502/src/M6502Hi.cxx

NullDev.o: $(CORE)/m6502/src/NullDev.cxx $(CORE)/m6502/src/NullDev.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/m6502/src/NullDev.cxx

System.o: $(CORE)/m6502/src/System.cxx $(CORE)/m6502/src/System.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/m6502/src/System.cxx
