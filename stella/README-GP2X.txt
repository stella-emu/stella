---------------------------------------
Stella 2.1 (revision 3):
Ported to GP2X by Alex Zaballa
Contact: azaballa@users.sourceforge.net
Homepage: http://stella.sourceforge.net
---------------------------------------

--------
Changes:
--------
>From 2.1-rv2
Scroll bug fixed (finally)
>From 2.1-rv1
Warlords graphics bug fixed
>From 2.1
Joystick fixed
Phosphor effect enabled
>From 2.0.1:
Stopped using the ROM Script (No more ROM limit. 'Single binary' zip
support available.)
Opened up the Native GUI for use through Joymouse Emulation
SOUND!
Diagonals
Snapshots
Some button mapping (but still comfortably familiar)

-----------
Known Bugs:
-----------
None reported.
Please report all bugs to me at azaballa@users.sourceforge.net.

------------------------------
Known GP2X Performance Issues:
------------------------------
Pitfall 2 runs slow due to extra hardware emulation. (Steve foresees this
	being fixed in the future.)
Stella will crash when trying to run a PAL ROM. This is a limitation of the
	GP2X as it cannot support resolutions higher than 320x240. We hope
	this will change with future kernel updates.


---------------------
Installation to GP2X:
---------------------
Place files stella and stella.gpe into a chosen directory. Upon first run,
Stella will ask for your ROM directory and snapshot directory. Remember that
the path to the sd card is '/mnt/sd'.

-----------
Navigation:
-----------
The joystick moves the mouse cursor in all menus except the ROM Launcher,
with any button acting as a click. In the ROM Launcher, up/down scrolls through
the ROM list and left/right selects options across the bottom, with any
button serving as a click.

-------------
GP2X Mapping:
-------------
  A      = Snapshot
  B      = Fire
  X      = Pause
  Y      = Settings Menu
  L      = Console Reset
  R      = Console Select
  START  = Launcher Menu
  SELECT = Command Menu

Note: Nothing has been taken away by changing this mapping. Save, Change, and
Load State are all available through the Command Menu (SELECT).

Caution: Changing video and audio settings could lead to a non-working Stella
setup. If you do feel the need to change the settings (as we all do) and happen
to cause a crash or hang, delete the 'stellarc' file and that should clear up
the problem. If anyone finds any settings that give better performance in
sound and gameplay, let me know and I might incorporate them into the next
build. Happy tinkering.

-------------
Compile Info:
-------------
Since the GP2X branch was so graciously added to Stella, GP2X compilation is
now built in. If you plan on doing a compile yourself, you need the GP2X devkit
pro and SDL Open2X libs with zlib support. I recommend theoddbot's Open2X
libraries which work very nicely for this. Any questions, email me.

Make sure your path includes your arm-linux compilers:
export PATH=<your arm-linux compiler path>:$PATH

This is what my configure line looks like:
./configure
  --prefix=<your destination folder>
  --enable-zlib
  --disable-cheats
  --disable-developer
  --host=gp2x
  --with-zlib-prefix=<your gp2x zlib path>
  --with-png-prefix=<your gp2x png path (configure appends '/include')>
  --with-sdl-prefix=<your sdl config path>
  --x-libraries=<your X11 libraries path>

Then:
make

Then strip the exec:
make gp2x-strip

You should get an exec ~2mb in size. Much better than 7mb right?

To gather all necessary files into directory './stella/gp2x' for easy
access, type:
make gp2x-organize

Note: /gp2x directory created in the main stella source directory
(next to configure, Makefile, etc.).

---------------
Special Thanks:
---------------
... to Steve Anthony for the major improvements in the port and incorporating
    the GP2X. Not to mention the wealth of information and help... did I
    mention patience? ;-)
... to Eckhard Stolberg for making me known to the Stella Team.
... to the Stella Team for the excellent emulator and the hospitality.
... to theoddbot for the excellent Open2X toolchain libs.
... to TelcoLou for his hardwork, playtesting ad infinitum.
... to the community for all of the interest.
