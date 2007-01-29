-=-=-=-=-=-=-=-=-=-=-=-=-=-=  PocketStella -=-=-=-=-=-=-=-=-=-=-=-=-=-=

... is the official Windows CE port of the Stella emulator. It is coded
and maintained by Kostas Nakos (knakos.AT.gmail.com). Currently, supported
devices are all Smartphones (176x220 & QVGA), and Pocket PCs (QVGA & VGA).
This port makes use of the excellent STLport cross platform C++ template library.
Here are some news for you:

(This file last updated: 29 Jan 07)

v.2.3.5 : support for QGVA landscape smartphones,
          greatly improved UI rendering for low res smartphones,
          keyboard events can be now arbitrarily remapped,
          UI menus accessibility in smartphones
v.2.2   : native VGA support, better backlight handling


Installing and Running:
First create a directory for PocketStella, and copy the executable in it.
Also create a subdirectory called "Roms" (without the quotes, of course).
You should place your rom files there. On first run, this is where
PocketStella will look for roms. You can set an alternate path using the 
dialogs.

Warning for users of previous versions:
There have been three significant changes in PocketStella. First, you no
longer need the zlibce.dll inside the emulator directory. The library
is statically linked in the executable and can be safely deleted.
Second, the stella.pro is also no longer needed as its functionality
has been included in the emulator core. Please delete this file as well
as it leads to increased load times. The stella.cache is also no longer
needed. You can delete this as well.
Third, it is best to delete your already existing stella.ini file when
upgrading to this version.

Controls:
You can remap the action each button performs by using the UI menus.
The default keymap is:

A. Smartphones

DPad Up, Down, Left, Right : Menus     -> Up, Down, Page Up, Page Down      $
                             Emulation -> Joystick 0 or Paddle 0 emulation  $
Dpad Start (or Enter)      : Emulation -> Reset
Softkey A                  : Menus     -> Select (mouse click)
                             Emulation -> Fire Joystick/Paddle 0
Softkey B                  : Menus     -> Focus next item (tab)             #
                             Emulation -> In game menu                      #
Softkey C (usually *)      : Emulation -> Select
Backspace (arrow key)      : Menus     -> Cancel
                             Emulation -> Back to launcher
Call/Talk/Green button     : Emulation -> Rotate display

($): These buttons follow screen orientation.
(#): This key cannot be reassigned.

B. Pocket PCs
Pocket PC users get the same actions as Smartphone users attributed to
their keys, except the Call key. You can use the stylus to fully access
all menus and also:

Stylus drag                : Emulation -> Emulate Paddle 0
Tap top left               : Emulation -> Bring up menu
Tap bottom left            : Emulation -> Back to Launcher
Tap bottom right           : Emulation -> Rotate display


Build instructions:
The build environment of evc3 has maximum compatibility, but the emulator
can certainly be built using evc4 of higher. The following instructions
pertain to evc3. wince2002 does not include support for C++ STL, so I use 
STLport. Get the latest release (I used the CVS version which has wince
support) and build it. Copy the two folders under the bin directory into 
a folder named stlport in the src\wince of stella (the .lib files). 
Also get the SDL library and copy the include files into a folder named sdl 
in src\wince. Get zconf.h and zlib.h of the zlib library and copy them in a 
folder named zlib in src\wince. Further, copy the arm and x86 zlibce.lib files 
into their respective src\wince\zlib\arm and src\wince\zlib\x86 folders. 
Finally, you need to add the includes directory of stlport (stlport\) to 
the default searched include directories. To do this use the dialog box: 
Tools->Options->Directories->Include files. The stlport include dir should 
be listed here before all other paths. To be able to emulate in the x86
emulator you also need the GAPI emulation.
Things are much easier in Visual Studio 2005. No external C++ STL library
is needed.

Have fun :-)

Links:
Stella project homepage: http://stella.sourceforge.net
contact the author     : knakos.AT.gmail.com
STLport library home   : http://www.stlport.org/
PocketStella home      : N/A yet
Microsoft eMbedded Visual Tools 3.0 : http://msdn.microsoft.com/mobility/windowsmobile/downloads/default.aspx
GAPI emulation         : http://pocketfrog.droneship.com/
