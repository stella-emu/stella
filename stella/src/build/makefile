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
## $Id: makefile,v 1.40 2003-09-20 13:41:36 stephena Exp $
##============================================================================

##============================================================================
## Development environment options
##
## The following options are used during compiling.
## Comment a line out to disable that option, remove comment to enable it.
##============================================================================

OPTIMIZATIONS  = $(CXXFLAGS) -Wall -Wno-unused

### which sound drivers to compile for the X11 and SDL versions
### OSS is most compatible, SDL for platforms where OSS not available
### SDL sound not yet supported in the X11 version
### comment out all lines to completely disable sound
###
SOUND_ALSA = 1
SOUND_OSS  = 1
SOUND_SDL  = 1

### if your C++ compiler doesn't support the bool type
# BSPF_BOOL = 1

### if your want some timing information displayed when you exit the program
# SHOW_TIMING = 1

### you want a 6507 trace written to stdout
# DEBUG = 1

### to include joystick support in the X11 and SDL versions
 JOYSTICK_SUPPORT = 1

### to include support for saving snapshots in png format
### (requires PNG library) FIXME
### Only X11 and SDL ports supported for now FIXME
 SNAPSHOT_SUPPORT = 1

### comment this out if your system doesn't
### have the gettimeofday function
### Currently, the X11 version won't compile without it
HAVE_GETTIMEOFDAY = 1

### to include support for game developers
### enables some extra commandline options that allow the user
### to override some emulation defaults
### Only X11 and SDL ports supported for now
 DEVELOPER_SUPPORT = 1

##============================================================================
## All done, type make to get a list of frontends
## No configurable options below this line ...
##============================================================================

CXX	= g++
LD	= g++

LDFLAGS = 
LDLIBS	= 

OBJS.X11   = 
OPTS.X11   =
LIBS.X11   =
CFLAGS.X11 =

OBJS.SDL   =
OPTS.SDL   =
LIBS.SDL   = `sdl-config --libs`
CFLAGS.SDL = `sdl-config --cflags`

OPTS.DOS   = -DDOS=1

SRC	= ..
CORE	= $(SRC)/emucore
UI	= $(SRC)/ui

INCLUDES = -I. -I$(CORE) -I$(CORE)/m6502/src -I$(CORE)/m6502/src/bspf/src -I$(UI)/common -I$(UI)/frontend

FLAGS = $(OPTIMIZATIONS) $(INCLUDES) $(SYS_INCLUDES)

## set the user-defined options
ifdef BSPF_BOOL
OPTS.X11 += -DBSPF_BOOL=1
OPTS.DOS += -DBSPF_BOOL=1   
endif

ifdef SHOW_TIMING
OPTS.X11 += -DSHOW_TIMING=1
OPTS.SDL += -DSHOW_TIMING=1
OPTS.DOS += -DSHOW_TIMING=1
endif

ifdef DEBUG
OPTS.X11 += -DDEBUG=1
OPTS.SDL += -DDEBUG=1
OPTS.DOS += -DDEBUG=1
endif

ifdef JOYSTICK_SUPPORT
OPTS.X11 += -DHAVE_JOYSTICK=1
OPTS.SDL += -DHAVE_JOYSTICK=1
endif

ifdef SNAPSHOT_SUPPORT
OBJS.X11 += Snapshot.o
OPTS.X11 += -DSNAPSHOT_SUPPORT=1
LIBS.X11 += -lpng -lz

OBJS.SDL += Snapshot.o
OPTS.SDL += -DSNAPSHOT_SUPPORT=1
LIBS.SDL += -lpng -lz
endif

ifdef HAVE_GETTIMEOFDAY
OPTS.X11 += -DHAVE_GETTIMEOFDAY=1
OPTS.SDL += -DHAVE_GETTIMEOFDAY=1
endif

ifdef DEVELOPER_SUPPORT
OPTS.X11 += -DDEVELOPER_SUPPORT=1
OPTS.SDL += -DDEVELOPER_SUPPORT=1
OPTS.DOS += -DDEVELOPER_SUPPORT=1
endif

ifeq ($(SOUND_OSS), 1)
  OPTS.X11 += -DSOUND_OSS=1
  OBJS.X11 += SoundOSS.o
  OPTS.SDL += -DSOUND_OSS=1
  OBJS.SDL += SoundOSS.o
endif

ifeq ($(SOUND_SDL), 1)
#  OPTS.X11 += -DSOUND_OSS=1
#  OBJS.X11 += SoundOSS.o
  OPTS.SDL += -DSOUND_SDL=1
  OBJS.SDL += SoundSDL.o
endif

ifeq ($(SOUND_ALSA), 1)
  OPTS.X11 += -DSOUND_ALSA=1
  OBJS.X11 += SoundALSA.o
  LIBS.X11 += -lasound
  OPTS.SDL += -DSOUND_ALSA=1
  OBJS.SDL += SoundALSA.o
  LIBS.SDL += -lasound
endif


default:
	@echo ""
	@echo "To build Stella type: 'make <version>'"
	@echo "You should edit the makefile for extra options"
	@echo ""
	@echo "<version> is one of:"
	@echo ""
	@echo "  dos                  DOS version using DJGPP"
	@echo "  unix-x               Generic Unix X windows version"
	@echo "  linux-x              Linux X windows version"
	@echo "  linux-sdl            Linux SDL version"
	@echo "  bsdi-x               BSD/OS 4.0 X Windows version"
	@echo "  solaris-x            Solaris X windows version"
	@echo ""
	@echo "Hopefully new versions will be added soon!"
	@echo ""

dos:
	make stella.exe \
          LD="gxx" \
          CXX="gcc" \
          INCLUDES="$(INCLUDES) -I$(UI)/dos -I$(UI)/sound" \
          OPTIONS="-DBSPF_DOS=1" \
          OPTIONS+="$(OPTS.DOS)" \
          LDFLAGS="" \
          LDLIBS="" \
          OBJS="mainDOS.o PCJoys.o SndDOS.o dos_sb.o vga.o"

unix-x:
	make stella.x11 \
          INCLUDES="$(INCLUDES) -I$(UI)/x11 -I$(UI)/sound" \
          SYS_INCLUDES="" \
          OPTIONS="-DBSPF_UNIX=1 -DUNIX=1" \
          OPTIONS+="$(OPTS.X11)" \
          LDFLAGS="-L/usr/X11R6/lib" \
          LDFLAGS+="$(CFLAGS.X11)" \
          LDLIBS="-lX11 -lXext" \
          LDLIBS+="$(LIBS.X11)" \
          OBJS="mainX11.o SettingsUNIX.o"
          OBJS+="$(OBJS.X11)"

linux-x:
	make stella.x11 \
          INCLUDES="$(INCLUDES) -I$(UI)/x11 -I$(UI)/sound" \
          SYS_INCLUDES="" \
          OPTIONS="-DBSPF_UNIX=1 -DUNIX=1" \
          OPTIONS+="$(OPTS.X11)" \
          LDFLAGS="-L/usr/X11R6/lib" \
          LDFLAGS+="$(CFLAGS.X11)" \
          LDLIBS="-lX11 -lXext" \
          LDLIBS+="$(LIBS.X11)" \
          OBJS="mainX11.o SettingsUNIX.o" \
          OBJS+="$(OBJS.X11)"

linux-sdl:
	make stella.sdl \
          INCLUDES="$(INCLUDES) -I$(UI)/sdl -I$(UI)/sound" \
          SYS_INCLUDES="" \
          OPTIONS="-DBSPF_UNIX=1 -DUNIX=1" \
          OPTIONS+="$(OPTS.SDL)" \
          LDFLAGS="-L/usr/X11R6/lib" \
          LDFLAGS+="$(CFLAGS.SDL)" \
          LDLIBS="-lX11 -lXext" \
          LDLIBS+="$(LIBS.SDL)" \
          OBJS="mainSDL.o RectList.o SettingsUNIX.o" \
          OBJS+="$(OBJS.SDL)"

bsdi-x:
	make stella.x11 \
          INCLUDES="$(INCLUDES) -I$(UI)/x11 -I$(UI)/sound" \
          SYS_INCLUDES="-I/usr/X11R6/include" \
          OPTIONS="-DBSPF_UNIX=1 -DUNIX=1" \
          OPTIONS+="$(OPTS.X11)" \
          LDFLAGS="-L/usr/X11R6/lib" \
          LDFLAGS+="$(CFLAGS.X11)" \
          LDLIBS="-lX11 -lXext" \
          LDLIBS+="$(LIBS.X11)" \
          OBJS="mainX11.o FrontendUNIX.o SettingsUNIX.o"
          OBJS+="$(OBJS.X11)"

solaris-x:
	make stella.x11 \
          INCLUDES="$(INCLUDES) -I$(UI)/x11 -I$(UI)/sound" \
          SYS_INCLUDES="-I/usr/openwin/include" \
          OPTIONS="-DBSPF_UNIX=1 -DUNIX=1" \
          OPTIONS+="$(OPTS.X11)" \
          LDFLAGS="-L/usr/openwin/lib" \
          LDFLAGS+="$(CFLAGS.X11)" \
          LDLIBS="-lX11 -lXext" \
          LDLIBS+="$(LIBS.X11)" \
          OBJS="mainX11.o FrontendUNIX.o SettingsUNIX.o"
          OBJS+="$(OBJS.X11)"

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
            Serializer.o Deserializer.o TIASound.o EventHandler.o \
            $(M6502_OBJS)

stella.exe: $(CORE_OBJS) $(OBJS)
	$(LD) -o a.exe *.o $(LDFLAGS) $(LDLIBS)
	exe2coff a.exe
	strip a
	del stella.exe
	copy /B $(DJGPP:\DJGPP.ENV=)\bin\pmodstub.exe+a stella.exe
	del a
	del a.exe

stella.x11: $(CORE_OBJS) $(OBJS)
	$(LD) -o stella.x11 $(CORE_OBJS) $(OBJS) $(LDFLAGS) $(LDLIBS)

stella.sdl: $(CORE_OBJS) $(OBJS)
	$(LD) -o stella.sdl $(CORE_OBJS) $(OBJS) $(LDFLAGS) $(LDLIBS)

M6502Low.ins: $(CORE)/m6502/src/M6502Low.m4 $(CORE)/m6502/src/M6502.m4
	m4 $(CORE)/m6502/src/M6502Low.m4 $(CORE)/m6502/src/M6502.m4 > M6502Low.ins

M6502Hi.ins: $(CORE)/m6502/src/M6502Hi.m4 $(CORE)/m6502/src/M6502.m4
	m4 $(CORE)/m6502/src/M6502Hi.m4 $(CORE)/m6502/src/M6502.m4 > M6502Hi.ins

M6502Low.o: M6502Low.ins
M6502Hi.o: M6502Hi.ins

cleandos:
	del *.o
	del stella.exe 
	del M6502Low.ins 
	del M6502Hi.ins

clean:
	rm -f *.o stella stella.x11 stella.sdl stella.exe core

cleanall: clean
	rm -f M6502Low.ins M6502Hi.ins

Driving.o: $(CORE)/Driving.cxx $(CORE)/Driving.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/Driving.cxx

Event.o: $(CORE)/Event.cxx $(CORE)/Event.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/Event.cxx

EventHandler.o: $(CORE)/EventHandler.cxx $(CORE)/EventHandler.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/EventHandler.cxx

Frontend.o: $(CORE)/Frontend.cxx $(CORE)/Frontend.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/Frontend.cxx

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

SettingsUNIX.o: $(UI)/common/SettingsUNIX.cxx $(UI)/common/SettingsUNIX.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(UI)/common/SettingsUNIX.cxx

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

FrontendUNIX.o: $(UI)/frontend/FrontendUNIX.cxx $(UI)/frontend/FrontendUNIX.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(UI)/frontend/FrontendUNIX.cxx

mainX11.o: $(UI)/x11/mainX11.cxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(UI)/x11/mainX11.cxx

mainSDL.o: $(UI)/sdl/mainSDL.cxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(UI)/sdl/mainSDL.cxx

RectList.o: $(UI)/sdl/RectList.cxx $(UI)/sdl/RectList.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(UI)/sdl/RectList.cxx

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
