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
## $Id: makefile,v 1.1.1.1 2001-12-27 19:54:18 bwmott Exp $
##============================================================================

##============================================================================
##
## The following options are used during compiling.  You may need to
## define some of these on the CXXFLAGS line if you're running on an
## "unsupported" system
##
##   -DBSPF_BOOL         if your C++ compiler doesn't support the bool type
##
##   -DSHOW_TIMING       if your want some timing information displayed when
##                       you exit the program
##
##   -DDEBUG             if you want a 6507 trace written to stdout
##
##   -DLINUX_JOYSTICK    to include linux joystick driver support
##                       (requires you to install the joystick kernel module)
##
##============================================================================

CXX      = g++
LD       = g++

LDFLAGS  = 
LDLIBS   = 

SRC      = ..
CORE     = $(SRC)/emucore
UI       = $(SRC)/ui

INCLUDES = -I. -I$(CORE) -I$(CORE)/m6502/src -I$(CORE)/m6502/src/bspf/src

CXXFLAGS = -O -Wall $(INCLUDES) $(SYS_INCLUDES)

default:
	@echo ""
	@echo "To build Stella type: 'make <version>'"
	@echo ""
	@echo "where <version> is one of:"
	@echo ""
	@echo "  dos                  DOS version using DJGPP"
	@echo "  unix-x               Generic Unix X windows version"
	@echo "  linux-x              Linux X windows version"
	@echo "  linux-x-joy          Linux X windows version with joystick"
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
          OPTIONS="-DBSPF_DOS" \
          LDFLAGS="" \
          LDLIBS="" \
          OBJS="mainDOS.o PCJoys.o SndDOS.o sbdrv.o TIASound.o"

unix-x:
	make xstella \
          INCLUDES="$(INCLUDES) -I$(UI)/x11 -I$(UI)/sound" \
          SYS_INCLUDES="" \
          OPTIONS="-DBSPF_UNIX" \
          LDFLAGS="-L/usr/X11R6/lib" \
          LDLIBS="-lX11 -lXext" \
          OBJS="mainX11.o SndUnix.o"

linux-x:
	make xstella \
          INCLUDES="$(INCLUDES) -I$(UI)/x11 -I$(UI)/sound" \
          SYS_INCLUDES="" \
          OPTIONS="-DBSPF_UNIX" \
          LDFLAGS="-L/usr/X11R6/lib" \
          LDLIBS="-lX11 -lXext" \
          OBJS="mainX11.o SndUnix.o"

linux-x-joy:
	make xstella \
          INCLUDES="$(INCLUDES) -I$(UI)/x11 -I$(UI)/sound" \
          SYS_INCLUDES="" \
          OPTIONS="-DBSPF_UNIX -DLINUX_JOYSTICK" \
          LDFLAGS="-L/usr/X11R6/lib" \
          LDLIBS="-lX11 -lXext" \
          OBJS="mainX11.o SndUnix.o"

bsdi-x:
	make xstella \
          INCLUDES="$(INCLUDES) -I$(UI)/x11 -I$(UI)/sound" \
          SYS_INCLUDES="-I/usr/X11R6/include" \
          OPTIONS="-DBSPF_UNIX" \
          LDFLAGS="-L/usr/X11R6/lib" \
          LDLIBS="-lX11 -lXext" \
          OBJS="mainX11.o SndUnix.o"

solaris-x:
	make xstella \
          INCLUDES="$(INCLUDES) -I$(UI)/x11 -I$(UI)/sound" \
          SYS_INCLUDES="-I/usr/openwin/include" \
          OPTIONS="-DBSPF_UNIX" \
          LDFLAGS="-L/usr/openwin/lib" \
          LDLIBS="-lX11 -lXext" \
          OBJS="mainX11.o SndUnix.o"

###############################################################################
## List of "core" object files
###############################################################################
M6502_OBJS = D6502.o Device.o M6502.o M6502Low.o M6502Hi.o NullDev.o System.o

CORE_OBJS = Booster.o Cart.o Cart2K.o Cart3F.o Cart4K.o CartAR.o CartE0.o \
            CartE7.o CartF4SC.o CartF6.o CartF6SC.o CartF8.o CartF8SC.o \
            CartFASC.o CartFE.o CartMC.o Console.o \
            Control.o DefProps.o Driving.o \
            Event.o Joystick.o Keyboard.o M6532.o MD5.o MediaSrc.o Paddles.o \
            Props.o PropsSet.o Random.o Sound.o Switches.o TIA.o \
            $(M6502_OBJS)

stella.exe: $(CORE_OBJS) $(OBJS)
	$(LD) -o a.exe *.o $(LDFLAGS) $(LDLIBS)
	exe2coff a.exe
	strip a
	copy /B \djgpp\bin\pmodstub.exe + a stella.exe
	del a
	del a.exe

xstella: $(CORE_OBJS) $(OBJS)
	$(LD) -o xstella $(CORE_OBJS) $(OBJS) $(LDFLAGS) $(LDLIBS)

M6502Low.ins: $(CORE)/m6502/src/M6502Low.m4 $(CORE)/m6502/src/M6502.m4
	m4 $(CORE)/m6502/src/M6502Low.m4 $(CORE)/m6502/src/M6502.m4 > M6502Low.ins

M6502Hi.ins: $(CORE)/m6502/src/M6502Hi.m4 $(CORE)/m6502/src/M6502.m4
	m4 $(CORE)/m6502/src/M6502Hi.m4 $(CORE)/m6502/src/M6502.m4 > M6502Hi.ins

DefProps.def: $(CORE)/stella.pro
	sed 's/"/\\\"/g;s/^/"/g;s/$$/",/g' $(CORE)/stella.pro > DefProps.def

DefProps.o: DefProps.def
M6502Low.o: M6502Low.ins
M6502Hi.o: M6502Hi.ins

cleandos:
	del *.o
	del stella.exe 
	del DefProps.def 
	del M6502Low.ins 
	del M6502Hi.ins

clean:
	rm -f *.o stella xstella stella.exe core

cleanall: clean
	rm -f DefProps.def M6502Low.ins M6502Hi.ins

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

DefProps.o: $(CORE)/DefProps.cxx
	$(CXX) -c $(CXXFLAGS) $(OPTIONS) $(CORE)/DefProps.cxx

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
	$(CXX) -c $(CXXFLAGS) $(OPTIONS) $(UI)/x11/mainX11.cxx

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

