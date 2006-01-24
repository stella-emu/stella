PocketStella

... is the official Windows CE port of the Stella emulator. It is coded
and maintained by Kostas Nakos (knakos@gmail.com). Currently, supported
devices are all Smartphones, and Pocket PCs with resolutions up to 320x240.
Work is underway for VGA resolutions. This port makes use of the excellent
STLport cross platform C++ template library.


Installing and Running:
First create a directory for PocketStella, and copy the executable in it.
Get the zlib for Windows CE (use the link at the bottom of this file).
You should extract zlibce.dll (ARM targets) into the same directory.
You also need the stella.pro file. Also create a subdirectory called 
"Roms" (without the quotes, of course). You should place your rom files
there. You can run Stella now.
A Heads up ; PocketStella scans the rom files the first time it's run.
Depending on the number of rom files, this may take from a few seconds
to half an hour. Be patient. You can transfer the stella.cache file
from your PC to your device to save you this intial delay. Also,
for Smartphone users, there is no way to request a rescan of the rom
directory. Delete the stella.cache file and run the emulator to perform
a rescan action.


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


Links:
Stella project homepage: http://stella.sourceforge.net
contact the author     : knakos@gmail.com
zlib for Windows CE    : http://www.tenik.co.jp/~adachi/wince/
STLport library home   : http://www.stlport.org/
PocketStella home      : N/A yet
