PocketStella

(Last updated: 20 Apr 06)
v.2.2: native VGA support, better backlight handling

... is the official Windows CE port of the Stella emulator. It is coded
and maintained by Kostas Nakos (knakos@gmail.com). Currently, supported
devices are all Smartphones (176x220 & QVGA), and Pocket PCs (QVGA & VGA).
This port makes use of the excellent STLport cross platform C++ template library.


Installing and Running:
First create a directory for PocketStella, and copy the executable in it.
Also create a subdirectory called "Roms" (without the quotes, of course).
You should place your rom files there. On first run, this is where
PocketStella will look for roms. On Pocket PCs you can set an alternate
path using the dialogs. On Smartphones, you can edit the "romdir" variable
in stella.ini. You can run Stella now.
A Heads up ; PocketStella scans the rom files the first time it's run.
Depending on the number of rom files, this may take from a few seconds
to half an hour. Be patient. You can transfer the stella.cache file
from your PC to your device to save you this intial delay. Also,
for Smartphone users, there is no way to request a rescan of the rom
directory (when for example you add more roms in the directory). 
Delete the stella.cache file and run the emulator to perform
a rescan action. 

Warning for users of previous versions:
There have been two significant changes in PocketStella. First, you no
longer need the zlibce.dll inside the emulator directory. The library
is statically linked in the executable and can be safely deleted.
Second, the stella.pro is also no longer needed as its functionality
has been included in the emulator core. Please delete this file as well
as it leads to increased load times.

Controls:
A. Smartphones
You get limited functionality in Smartphones due to lack of proper
input devices. But this doesn't mean you can't enjoy your favorite games
(quite the contrary if I may say so :)

DPad Up, Down, Left, Right : Launcher  -> Up, Down, Page Up, Page Down
                             Emulation -> Joystick 0 or Paddle 0 emulation
Dpad Start (or Enter)      : Launcher  -> Load rom
                             Emulation -> Reset
Softkey A                  : Emulation -> Fire Joystick/Paddle 0
Softkey B                  : Emulation -> Select
Softkey C (usually *)      : Launcher  -> Quit PocketStella
                             Emulation -> Back to Launcher
Call/Talk/Green button     : Emulation -> Rotate display

B. Pocket PCs
Pocket PC users get the same actions as Smartphone users attributed to
their keys, except the Call key. You can use the stylus to fully access
all menus and also:

Stylus drag                : Emulation -> Emulate Paddle 0
Tap top left               : Emulation -> Bring up menu
Tap bottom left            : Emulation -> Back to Launcher
Tap bottom right           : Emulation -> Rotate display


Build instructions:
The build environment is evc3 for maximum compatibility, but the emulator
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

NOTE: PocketStella has also been built with success under Visual Studio 2005.


Links:
Stella project homepage: http://stella.sourceforge.net
contact the author     : knakos@gmail.com
zlib for Windows CE    : http://www.tenik.co.jp/~adachi/wince/
STLport library home   : http://www.stlport.org/
PocketStella home      : N/A yet
Microsoft eMbedded Visual Tools 3.0 : http://msdn.microsoft.com/mobility/windowsmobile/downloads/default.aspx
GAPI emulation         : http://pocketfrog.droneship.com/
