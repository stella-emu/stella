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
## $Id: makefile,v 1.14 2002-04-10 04:00:58 bwmott Exp $
##============================================================================

##============================================================================
## Development environment options
##
## The following options are used during compiling.
## Comment a line out to disable that option, remove comment to enable it.
##============================================================================

### this should work with all compilers 
OPTIMIZATIONS    =
### for normal optimization, full warnings
# OPTIMIZATIONS    = -O -Wall
### for common optimizations, full warnings except unused vars
# OPTIMIZATIONS    = -O2 -Wall -Wno-unused
### to get full optimization under gcc/x Intel based OS's..
# OPTIMIZATIONS    = -O3 -mcpu=pentiumpro -march=pentiumpro -Wall -Wno-unused \
#	-funroll-loops -fstrength-reduce -fomit-frame-pointer -ffast-math \
#	-malign-functions=2 -malign-jumps=2 -malign-loops=2
### to get full optimization under gcc/x Athlon based OS's..
# OPTIMIZATIONS    = -O3 -mcpu=athlon -march=athlon -Wall -Wno-unused \
#	-funroll-loops -fstrength-reduce -fomit-frame-pointer -ffast-math \
#	-malign-functions=2 -malign-jumps=2 -malign-loops=2

### if your C++ compiler doesn't support the bool type
# BSPF_BOOL = 1

### if your want some timing information displayed when you exit the program
# SHOW_TIMING = 1

### you want a 6507 trace written to stdout
# DEBUG = 1

### to include linux joystick driver support in the X11 version
### (requires you to install the joystick kernel module)
# LINUX_JOYSTICK = 1

### to include support for saving snapshots in png format
### (requires PNG library)
### Only X11 and SDL ports supported for now
# SNAPSHOT_SUPPORT = 1

### to enable EXPERIMENTAL TIMING code
### Enables emulator to not use 100% CPU
### Only X11 and SDL ports supported for now
# EXPERIMENTAL_TIMING = 1

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

OPTS.DOS   =

SRC	= ..
CORE	= $(SRC)/emucore
UI	= $(SRC)/ui

INCLUDES = -I. -I$(CORE) -I$(CORE)/m6502/src -I$(CORE)/m6502/src/bspf/src -I$(UI)/common

CXXFLAGS = $(OPTIMIZATIONS) $(INCLUDES) $(SYS_INCLUDES)

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

ifdef LINUX_JOYSTICK
OPTS.X11 += -DLINUX_JOYSTICK=1
endif

ifdef SNAPSHOT_SUPPORT
OBJS.X11 += Snapshot.o
OPTS.X11 += -DHAVE_PNG=1
LIBS.X11 += -lpng

OBJS.SDL += Snapshot.o
OPTS.SDL += -DHAVE_PNG=1
LIBS.SDL += -lpng
endif

ifdef EXPERIMENTAL_TIMING
OPTS.X11 += -DEXPERIMENTAL_TIMING=1
OPTS.SDL += -DEXPERIMENTAL_TIMING=1
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
          OBJS="mainDOS.o PCJoys.o SndDOS.o sbdrv.o TIASound.o"

unix-x:
	make stella.x11 \
          INCLUDES="$(INCLUDES) -I$(UI)/x11 -I$(UI)/sound" \
          SYS_INCLUDES="" \
          OPTIONS="-DBSPF_UNIX=1" \
          OPTIONS+="$(OPTS.X11)" \
          LDFLAGS="-L/usr/X11R6/lib" \
          LDFLAGS+="$(CFLAGS.X11)" \
          LDLIBS="-lX11 -lXext" \
          LDLIBS+="$(LIBS.X11)" \
          OBJS="mainX11.o SndUnix.o"

linux-x:
	make stella.x11 \
          INCLUDES="$(INCLUDES) -I$(UI)/x11 -I$(UI)/sound" \
          SYS_INCLUDES="" \
          OPTIONS="-DBSPF_UNIX=1" \
          OPTIONS+="$(OPTS.X11)" \
          LDFLAGS="-L/usr/X11R6/lib" \
          LDFLAGS+="$(CFLAGS.X11)" \
          LDLIBS="-lX11 -lXext" \
          LDLIBS+="$(LIBS.X11)" \
          OBJS="mainX11.o SndUnix.o" \
          OBJS+="$(OBJS.X11)"

linux-sdl:
	make stella.sdl \
          INCLUDES="$(INCLUDES) -I$(UI)/sdl -I$(UI)/sound" \
          SYS_INCLUDES="" \
          OPTIONS="-DBSPF_UNIX=1" \
          OPTIONS+="$(OPTS.SDL)" \
          LDFLAGS="-L/usr/X11R6/lib" \
          LDFLAGS+="$(CFLAGS.SDL)" \
          LDLIBS="-lX11 -lXext" \
          LDLIBS+="$(LIBS.SDL)" \
          OBJS="mainSDL.o SndUnix.o RectList.o" \
          OBJS+="$(OBJS.SDL)"
bsdi-x:
	make stella.x11 \
          INCLUDES="$(INCLUDES) -I$(UI)/x11 -I$(UI)/sound" \
          SYS_INCLUDES="-I/usr/X11R6/include" \
          OPTIONS="-DBSPF_UNIX=1" \
          OPTIONS+="$(OPTS.X11)" \
          LDFLAGS="-L/usr/X11R6/lib" \
          LDFLAGS+="$(CFLAGS.X11)" \
          LDLIBS="-lX11 -lXext" \
          LDLIBS+="$(LIBS.X11)" \
          OBJS="mainX11.o SndUnix.o"

solaris-x:
	make stella.x11 \
          INCLUDES="$(INCLUDES) -I$(UI)/x11 -I$(UI)/sound" \
          SYS_INCLUDES="-I/usr/openwin/include" \
          OPTIONS="-DBSPF_UNIX=1" \
          OPTIONS+="$(OPTS.X11)" \
          LDFLAGS="-L/usr/openwin/lib" \
          LDFLAGS+="$(CFLAGS.X11)" \
          LDLIBS="-lX11 -lXext" \
          LDLIBS+="$(LIBS.X11)" \
          OBJS="mainX11.o SndUnix.o"

###############################################################################
## List of "core" object files
###############################################################################
M6502_OBJS = D6502.o Device.o M6502.o M6502Low.o M6502Hi.o NullDev.o System.o

CORE_OBJS = Booster.o Cart.o Cart2K.o Cart3F.o Cart4K.o CartAR.o CartDPC.o \
            CartE0.o CartE7.o CartF4SC.o CartF6.o CartF6SC.o CartF8.o \
            CartF8SC.o CartFASC.o CartFE.o CartMC.o CartCV.o CartMB.o \
            Console.o Control.o Driving.o \
            Event.o Joystick.o Keyboard.o M6532.o MD5.o MediaSrc.o Paddles.o \
            Props.o PropsSet.o Random.o Sound.o Switches.o Settings.o TIA.o \
            $(M6502_OBJS)

stella.exe: $(CORE_OBJS) $(OBJS)
	$(LD) -o a.exe *.o $(LDFLAGS) $(LDLIBS)
	exe2coff a.exe
	strip a
	copy /B $(DJGPP:\DJGPP.ENV=)\bin\pmodstub.exe + a stella.exe
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

Driving.o: $(CORE)/Driving.cxx
	$(CXX) -c $(CXXFLAGS) $(OPTIONS) $(CORE)/Driving.cxx

Event.o: $(CORE)/Event.cxx
	$(CXX) -c $(CXXFLAGS) $(OPTIONS) $(CORE)/Event.cxx

Control.o: $(CORE)/Control.cxx
	$(CXX) -c $(CXXFLAGS) $(OPTIONS) $(CORE)/Control.cxx

Joystick.o: $(CORE)/Joystick.cxx
	$(CXX) -c $(CXXFLAGS) $(OPTIONS) $(CORE)/Joystick.cxx

Keyboard.o: $(CORE)/Keyboard.cxx
	$(CXX) -c $(CXXFLAGS) $(OPTIONS) $(CORE)/Keyboard.cxx

Paddles.o: $(CORE)/Paddles.cxx
	$(CXX) -c $(CXXFLAGS) $(OPTIONS) $(CORE)/Paddles.cxx

Booster.o: $(CORE)/Booster.cxx
	$(CXX) -c $(CXXFLAGS) $(OPTIONS) $(CORE)/Booster.cxx

Cart.o: $(CORE)/Cart.cxx
	$(CXX) -c $(CXXFLAGS) $(OPTIONS) $(CORE)/Cart.cxx

Cart2K.o: $(CORE)/Cart2K.cxx
	$(CXX) -c $(CXXFLAGS) $(OPTIONS) $(CORE)/Cart2K.cxx

Cart3F.o: $(CORE)/Cart3F.cxx
	$(CXX) -c $(CXXFLAGS) $(OPTIONS) $(CORE)/Cart3F.cxx

Cart4K.o: $(CORE)/Cart4K.cxx
	$(CXX) -c $(CXXFLAGS) $(OPTIONS) $(CORE)/Cart4K.cxx

CartAR.o: $(CORE)/CartAR.cxx
	$(CXX) -c $(CXXFLAGS) $(OPTIONS) $(CORE)/CartAR.cxx

CartDPC.o: $(CORE)/CartDPC.cxx
	$(CXX) -c $(CXXFLAGS) $(OPTIONS) $(CORE)/CartDPC.cxx

CartE0.o: $(CORE)/CartE0.cxx
	$(CXX) -c $(CXXFLAGS) $(OPTIONS) $(CORE)/CartE0.cxx

CartE7.o: $(CORE)/CartE7.cxx
	$(CXX) -c $(CXXFLAGS) $(OPTIONS) $(CORE)/CartE7.cxx

CartF4SC.o: $(CORE)/CartF4SC.cxx
	$(CXX) -c $(CXXFLAGS) $(OPTIONS) $(CORE)/CartF4SC.cxx

CartF6.o: $(CORE)/CartF6.cxx
	$(CXX) -c $(CXXFLAGS) $(OPTIONS) $(CORE)/CartF6.cxx

CartF6SC.o: $(CORE)/CartF6SC.cxx
	$(CXX) -c $(CXXFLAGS) $(OPTIONS) $(CORE)/CartF6SC.cxx

CartF8.o: $(CORE)/CartF8.cxx
	$(CXX) -c $(CXXFLAGS) $(OPTIONS) $(CORE)/CartF8.cxx

CartF8SC.o: $(CORE)/CartF8SC.cxx
	$(CXX) -c $(CXXFLAGS) $(OPTIONS) $(CORE)/CartF8SC.cxx

CartFASC.o: $(CORE)/CartFASC.cxx
	$(CXX) -c $(CXXFLAGS) $(OPTIONS) $(CORE)/CartFASC.cxx

CartFE.o: $(CORE)/CartFE.cxx
	$(CXX) -c $(CXXFLAGS) $(OPTIONS) $(CORE)/CartFE.cxx

CartMC.o: $(CORE)/CartMC.cxx
	$(CXX) -c $(CXXFLAGS) $(OPTIONS) $(CORE)/CartMC.cxx

CartMB.o: $(CORE)/CartMB.cxx
	$(CXX) -c $(CXXFLAGS) $(OPTIONS) $(CORE)/CartMB.cxx

CartCV.o: $(CORE)/CartCV.cxx
	$(CXX) -c $(CXXFLAGS) $(OPTIONS) $(CORE)/CartCV.cxx

M6532.o: $(CORE)/M6532.cxx
	$(CXX) -c $(CXXFLAGS) $(OPTIONS) $(CORE)/M6532.cxx

TIA.o: $(CORE)/TIA.cxx
	$(CXX) -c $(CXXFLAGS) $(OPTIONS) $(CORE)/TIA.cxx

Console.o: $(CORE)/Console.cxx
	$(CXX) -c $(CXXFLAGS) $(OPTIONS) $(CORE)/Console.cxx

MD5.o: $(CORE)/MD5.cxx
	$(CXX) -c $(CXXFLAGS) $(OPTIONS) $(CORE)/MD5.cxx

MediaSrc.o: $(CORE)/MediaSrc.cxx
	$(CXX) -c $(CXXFLAGS) $(OPTIONS) $(CORE)/MediaSrc.cxx

PropsSet.o: $(CORE)/PropsSet.cxx
	$(CXX) -c $(CXXFLAGS) $(OPTIONS) $(CORE)/PropsSet.cxx

Props.o: $(CORE)/Props.cxx
	$(CXX) -c $(CXXFLAGS) $(OPTIONS) $(CORE)/Props.cxx

Random.o: $(CORE)/Random.cxx
	$(CXX) -c $(CXXFLAGS) $(OPTIONS) $(CORE)/Random.cxx

Switches.o: $(CORE)/Switches.cxx
	$(CXX) -c $(CXXFLAGS) $(OPTIONS) $(CORE)/Switches.cxx

Sound.o: $(CORE)/Sound.cxx
	$(CXX) -c $(CXXFLAGS) $(OPTIONS) $(CORE)/Sound.cxx

Settings.o: $(UI)/common/Settings.cxx $(UI)/common/Settings.hxx
	$(CXX) -c $(CXXFLAGS) $(OPTIONS) $(LDFLAGS) $(UI)/common/Settings.cxx

Terminal.o: $(UI)/x11/Terminal.cxx
	$(CXX) -c $(CXXFLAGS) $(OPTIONS) $(UI)/x11/Terminal.cxx

mainDOS.o: $(UI)/dos/mainDOS.cxx
	$(CXX) -c $(CXXFLAGS) $(OPTIONS) $(UI)/dos/mainDOS.cxx

PCJoys.o: $(UI)/dos/PCJoys.cxx
	$(CXX) -c $(CXXFLAGS) $(OPTIONS) $(UI)/dos/PCJoys.cxx

SndDOS.o: $(UI)/dos/SndDOS.cxx
	$(CXX) -c $(CXXFLAGS) $(OPTIONS) $(UI)/dos/SndDOS.cxx

sbdrv.o: $(UI)/dos/sbdrv.c
	$(CXX) -c $(CXXFLAGS) $(OPTIONS) $(UI)/dos/sbdrv.c

TIASound.o: $(UI)/sound/TIASound.c
	$(CXX) -c -DWIN32 $(CXXFLAGS) $(OPTIONS) $(UI)/sound/TIASound.c

TermX11.o: $(UI)/x11/TermX11.cxx
	$(CXX) -c $(CXXFLAGS) $(OPTIONS) $(UI)/x11/TermX11.cxx

mainX11.o: $(UI)/x11/mainX11.cxx
	$(CXX) -c $(CXXFLAGS) $(OPTIONS) $(LDFLAGS) $(UI)/x11/mainX11.cxx

mainSDL.o: $(UI)/sdl/mainSDL.cxx
	$(CXX) -c $(CXXFLAGS) $(OPTIONS) $(LDFLAGS) $(UI)/sdl/mainSDL.cxx

RectList.o: $(UI)/sdl/RectList.cxx $(UI)/sdl/RectList.hxx
	$(CXX) -c $(CXXFLAGS) $(OPTIONS) $(LDFLAGS) $(UI)/sdl/RectList.cxx

Snapshot.o: $(UI)/common/Snapshot.cxx $(UI)/common/Snapshot.hxx
	$(CXX) -c $(CXXFLAGS) $(OPTIONS) $(LDFLAGS) $(UI)/common/Snapshot.cxx

SndUnix.o: $(UI)/sound/SndUnix.cxx
	$(CXX) -c $(CXXFLAGS) $(OPTIONS) $(UI)/sound/SndUnix.cxx

D6502.o: $(CORE)/m6502/src/D6502.cxx
	$(CXX) -c $(CXXFLAGS) $(OPTIONS) $(CORE)/m6502/src/D6502.cxx

Device.o: $(CORE)/m6502/src/Device.cxx
	$(CXX) -c $(CXXFLAGS) $(OPTIONS) $(CORE)/m6502/src/Device.cxx

M6502.o: $(CORE)/m6502/src/M6502.cxx
	$(CXX) -c $(CXXFLAGS) $(OPTIONS) $(CORE)/m6502/src/M6502.cxx

M6502Low.o: $(CORE)/m6502/src/M6502Low.cxx
	$(CXX) -c $(CXXFLAGS) $(OPTIONS) $(CORE)/m6502/src/M6502Low.cxx

M6502Hi.o: $(CORE)/m6502/src/M6502Hi.cxx
	$(CXX) -c $(CXXFLAGS) $(OPTIONS) $(CORE)/m6502/src/M6502Hi.cxx

NullDev.o: $(CORE)/m6502/src/NullDev.cxx
	$(CXX) -c $(CXXFLAGS) $(OPTIONS) $(CORE)/m6502/src/NullDev.cxx

System.o: $(CORE)/m6502/src/System.cxx
	$(CXX) -c $(CXXFLAGS) $(OPTIONS) $(CORE)/m6502/src/System.cxx

