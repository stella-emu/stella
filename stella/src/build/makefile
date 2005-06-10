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
## $Id: makefile,v 1.90 2005-06-10 17:46:05 stephena Exp $
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

### to include sound support
 SOUND_SUPPORT = 1

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
OBJECTS  =  mainSDL.o FrameBufferSoft.o
OPTIONS  =
EXE_NAME =
SMP      =

SRC      = ..
CORE     = $(SRC)/emucore
COMMON   = $(SRC)/common
GUI      = $(SRC)/gui

INCLUDES  = -I. -I$(CORE) -I$(CORE)/m6502/src \
            -I$(CORE)/m6502/src/bspf/src -I$(COMMON) -I$(GUI) -I$(SRC)/unix -I$(SRC)/win32

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

FLAGS = $(OPTIMIZATIONS) -Wall -Wno-multichar -Wunused $(INCLUDES) $(SYS_INCLUDES)

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

ifdef SOUND_SUPPORT
  OBJECTS += TIASound.o SoundSDL.o
  OPTIONS += -DSOUND_SUPPORT
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
          OPTIONS="$(OPTIONS) -DBSPF_UNIX -DUNIX -DHAVE_GETTIMEOFDAY -DHAVE_INTTYPES" \
          OBJS="$(OBJECTS) SettingsUNIX.o OSystemUNIX.o FSNodePOSIX.o"

linux-gl:
	$(MAKE) $(SMP) stella \
          EXE_NAME="stella" \
          LDFLAGS="$(LDFLAGS) -L/usr/X11R6/lib" \
          LDLIBS="$(LDLIBS) -lGL" \
          OPTIONS="$(OPTIONS) -DBSPF_UNIX -DUNIX -DHAVE_GETTIMEOFDAY -DHAVE_INTTYPES -DDISPLAY_OPENGL" \
          OBJS="$(OBJECTS) FrameBufferGL.o SettingsUNIX.o OSystemUNIX.o FSNodePOSIX.o"

win32:
	$(MAKE) $(SMP) stella \
          EXE_NAME="stella.exe" \
          OPTIONS="$(OPTIONS) -DBSPF_WIN32 -DWIN32 -DHAVE_INTTYPES" \
          OBJS="$(OBJECTS) SettingsWin32.o OSystemWin32.o FSNodeWin32.o"

win32-gl:
	$(MAKE) $(SMP) stella \
          EXE_NAME="stella.exe" \
          LDLIBS="$(LDLIBS) -lopengl32" \
          OPTIONS="$(OPTIONS) -DBSPF_WIN32 -DWIN32 -DHAVE_INTTYPES -DDISPLAY_OPENGL -DTEXTURES_ARE_LOST" \
          OBJS="$(OBJECTS) FrameBufferGL.o SettingsWin32.o OSystemWin32.o FSNodeWin32.o"


###############################################################################
## List of "core" object files
###############################################################################
M6502_OBJS = D6502.o Device.o M6502.o M6502Low.o M6502Hi.o NullDev.o System.o

GUI_OBJS = Font.o Menu.o Launcher.o Debugger.o \
           Widget.o PopUpWidget.o ScrollBarWidget.o ListWidget.o TabWidget.o \
           Dialog.o DialogContainer.o OptionsDialog.o VideoDialog.o AudioDialog.o \
           EventMappingDialog.o GameInfoDialog.o HelpDialog.o AboutDialog.o \
           LauncherDialog.o LauncherOptionsDialog.o BrowserDialog.o GameList.o \
           ProgressDialog.o \
           DebuggerDialog.o PromptWidget.o

CORE_OBJS = Booster.o Cart.o Cart2K.o Cart3F.o Cart4K.o CartAR.o CartDPC.o \
            CartE0.o CartE7.o CartF4.o CartF4SC.o CartF6.o CartF6SC.o \
            CartF8.o CartF8SC.o CartFASC.o CartFE.o CartMC.o CartCV.o \
            CartMB.o CartUA.o Console.o Control.o Driving.o \
            Event.o Joystick.o Keyboard.o M6532.o MD5.o MediaSrc.o Paddles.o \
            Props.o PropsSet.o Random.o SoundNull.o Switches.o Settings.o TIA.o \
            Serializer.o Deserializer.o EventHandler.o FrameBuffer.o \
            OSystem.o FSNode.o unzip.o \
				DebuggerParser.o DebuggerCommand.o DCmdQuit.o \
            $(M6502_OBJS) $(GUI_OBJS)

stella: $(CORE_OBJS) $(OBJS)
	$(LD) -o $(EXE_NAME) $(CORE_OBJS) $(OBJS) $(LDFLAGS) $(LDLIBS)
	strip $(EXE_NAME)

M6502Low.ins: $(CORE)/m6502/src/M6502Low.m4 $(CORE)/m6502/src/M6502.m4
	m4 $(CORE)/m6502/src/M6502Low.m4 $(CORE)/m6502/src/M6502.m4 > $(CORE)/m6502/src/M6502Low.ins

M6502Hi.ins: $(CORE)/m6502/src/M6502Hi.m4 $(CORE)/m6502/src/M6502.m4
	m4 $(CORE)/m6502/src/M6502Hi.m4 $(CORE)/m6502/src/M6502.m4 > $(CORE)/m6502/src/M6502Hi.ins

#M6502Low.o: M6502Low.ins
#M6502Hi.o: M6502Hi.ins

clean:
	rm -f *.o stella stella.exe core

Driving.o: $(CORE)/Driving.cxx $(CORE)/Driving.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/Driving.cxx

Event.o: $(CORE)/Event.cxx $(CORE)/Event.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/Event.cxx

EventHandler.o: $(CORE)/EventHandler.cxx $(CORE)/EventHandler.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(CORE)/EventHandler.cxx

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

CartCV.o: $(CORE)/CartCV.cxx $(CORE)/CartCV.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/CartCV.cxx

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

CartMB.o: $(CORE)/CartMB.cxx $(CORE)/CartMB.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/CartMB.cxx

CartMC.o: $(CORE)/CartMC.cxx $(CORE)/CartMC.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/CartMC.cxx

CartUA.o: $(CORE)/CartUA.cxx $(CORE)/CartUA.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/CartUA.cxx

M6532.o: $(CORE)/M6532.cxx $(CORE)/M6532.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/M6532.cxx

TIA.o: $(CORE)/TIA.cxx $(CORE)/TIA.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/TIA.cxx

TIASound.o: $(CORE)/TIASound.c $(CORE)/TIASound.h
	$(CXX) -c -DWIN32 $(FLAGS) $(OPTIONS) $(CORE)/TIASound.c

Console.o: $(CORE)/Console.cxx $(CORE)/Console.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(CORE)/Console.cxx

MD5.o: $(CORE)/MD5.cxx $(CORE)/MD5.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/MD5.cxx

MediaSrc.o: $(CORE)/MediaSrc.cxx $(CORE)/MediaSrc.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/MediaSrc.cxx

FrameBuffer.o: $(CORE)/FrameBuffer.cxx $(CORE)/FrameBuffer.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(CORE)/FrameBuffer.cxx

PropsSet.o: $(CORE)/PropsSet.cxx $(CORE)/PropsSet.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/PropsSet.cxx

Props.o: $(CORE)/Props.cxx $(CORE)/Props.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/Props.cxx

Random.o: $(CORE)/Random.cxx $(CORE)/Random.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/Random.cxx

Switches.o: $(CORE)/Switches.cxx $(CORE)/Switches.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/Switches.cxx

Serializer.o: $(CORE)/Serializer.cxx $(CORE)/Serializer.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/Serializer.cxx

Deserializer.o: $(CORE)/Deserializer.cxx $(CORE)/Deserializer.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/Deserializer.cxx

Settings.o: $(CORE)/Settings.cxx $(CORE)/Settings.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(CORE)/Settings.cxx

OSystem.o: $(CORE)/OSystem.cxx $(CORE)/OSystem.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(CORE)/OSystem.cxx

FSNode.o: $(CORE)/FSNode.cxx $(CORE)/FSNode.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(CORE)/FSNode.cxx

unzip.o: $(CORE)/unzip.c $(CORE)/unzip.h
	$(CXX) -c $(FLAGS) $(OPTIONS) $(CORE)/unzip.c

SettingsUNIX.o: $(SRC)/unix/SettingsUNIX.cxx $(SRC)/unix/SettingsUNIX.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(SRC)/unix/SettingsUNIX.cxx

OSystemUNIX.o: $(SRC)/unix/OSystemUNIX.cxx $(SRC)/unix/OSystemUNIX.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(SRC)/unix/OSystemUNIX.cxx

FSNodePOSIX.o: $(SRC)/unix/FSNodePOSIX.cxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(SRC)/unix/FSNodePOSIX.cxx

SettingsWin32.o: $(SRC)/win32/SettingsWin32.cxx $(SRC)/win32/SettingsWin32.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(SRC)/win32/SettingsWin32.cxx

OSystemWin32.o: $(SRC)/win32/OSystemWin32.cxx $(SRC)/win32/OSystemWin32.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(SRC)/win32/OSystemWin32.cxx

FSNodeWin32.o: $(SRC)/win32/FSNodeWin32.cxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(SRC)/win32/FSNodeWin32.cxx

SoundNull.o: $(COMMON)/SoundNull.cxx $(COMMON)/SoundNull.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(COMMON)/SoundNull.cxx

SoundSDL.o: $(COMMON)/SoundSDL.cxx $(COMMON)/SoundSDL.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(COMMON)/SoundSDL.cxx

mainSDL.o: $(COMMON)/mainSDL.cxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(COMMON)/mainSDL.cxx

FrameBufferSoft.o: $(COMMON)/FrameBufferSoft.cxx $(COMMON)/FrameBufferSoft.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(COMMON)/FrameBufferSoft.cxx

FrameBufferGL.o: $(COMMON)/FrameBufferGL.cxx $(COMMON)/FrameBufferGL.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(COMMON)/FrameBufferGL.cxx

Snapshot.o: $(COMMON)/Snapshot.cxx $(COMMON)/Snapshot.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(COMMON)/Snapshot.cxx

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

Font.o: $(GUI)/Font.cxx $(GUI)/Font.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(GUI)/Font.cxx

Menu.o: $(GUI)/Menu.cxx $(GUI)/Menu.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(GUI)/Menu.cxx

Launcher.o: $(GUI)/Launcher.cxx $(GUI)/Launcher.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(GUI)/Launcher.cxx

Debugger.o: $(GUI)/Debugger.cxx $(GUI)/Debugger.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(GUI)/Debugger.cxx

Widget.o: $(GUI)/Widget.cxx $(GUI)/Widget.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(GUI)/Widget.cxx

PopUpWidget.o: $(GUI)/PopUpWidget.cxx $(GUI)/PopUpWidget.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(GUI)/PopUpWidget.cxx

ScrollBarWidget.o: $(GUI)/ScrollBarWidget.cxx $(GUI)/ScrollBarWidget.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(GUI)/ScrollBarWidget.cxx

ListWidget.o: $(GUI)/ListWidget.cxx $(GUI)/ListWidget.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(GUI)/ListWidget.cxx

TabWidget.o: $(GUI)/TabWidget.cxx $(GUI)/TabWidget.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(GUI)/TabWidget.cxx

Dialog.o: $(GUI)/Dialog.cxx $(GUI)/Dialog.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(GUI)/Dialog.cxx

DialogContainer.o: $(GUI)/DialogContainer.cxx $(GUI)/DialogContainer.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(GUI)/DialogContainer.cxx

OptionsDialog.o: $(GUI)/OptionsDialog.cxx $(GUI)/OptionsDialog.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(GUI)/OptionsDialog.cxx

VideoDialog.o: $(GUI)/VideoDialog.cxx $(GUI)/VideoDialog.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(GUI)/VideoDialog.cxx

AudioDialog.o: $(GUI)/AudioDialog.cxx $(GUI)/AudioDialog.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(GUI)/AudioDialog.cxx

EventMappingDialog.o: $(GUI)/EventMappingDialog.cxx $(GUI)/EventMappingDialog.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(GUI)/EventMappingDialog.cxx

GameInfoDialog.o: $(GUI)/GameInfoDialog.cxx $(GUI)/GameInfoDialog.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(GUI)/GameInfoDialog.cxx

HelpDialog.o: $(GUI)/HelpDialog.cxx $(GUI)/HelpDialog.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(GUI)/HelpDialog.cxx

AboutDialog.o: $(GUI)/AboutDialog.cxx $(GUI)/AboutDialog.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(GUI)/AboutDialog.cxx

LauncherDialog.o: $(GUI)/LauncherDialog.cxx $(GUI)/LauncherDialog.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(GUI)/LauncherDialog.cxx

LauncherOptionsDialog.o: $(GUI)/LauncherOptionsDialog.cxx $(GUI)/LauncherOptionsDialog.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(GUI)/LauncherOptionsDialog.cxx

BrowserDialog.o: $(GUI)/BrowserDialog.cxx $(GUI)/BrowserDialog.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(GUI)/BrowserDialog.cxx

GameList.o: $(GUI)/GameList.cxx $(GUI)/GameList.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(GUI)/GameList.cxx

ProgressDialog.o: $(GUI)/ProgressDialog.cxx $(GUI)/ProgressDialog.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(GUI)/ProgressDialog.cxx

DebuggerDialog.o: $(GUI)/DebuggerDialog.cxx $(GUI)/DebuggerDialog.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(GUI)/DebuggerDialog.cxx

PromptWidget.o: $(GUI)/PromptWidget.cxx $(GUI)/PromptWidget.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(GUI)/PromptWidget.cxx

DebuggerParser.o: $(COMMON)/DebuggerParser.cxx $(COMMON)/DebuggerParser.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(COMMON)/DebuggerParser.cxx

DebuggerCommand.o: $(COMMON)/DebuggerCommand.cxx $(COMMON)/DebuggerCommand.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(COMMON)/DebuggerCommand.cxx

DCmdQuit.o: $(COMMON)/DCmdQuit.cxx $(COMMON)/DCmdQuit.hxx
	$(CXX) -c $(FLAGS) $(OPTIONS) $(LDFLAGS) $(COMMON)/DCmdQuit.cxx

