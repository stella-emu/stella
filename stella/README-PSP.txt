
STELLA port for the Sony PSP contributed by
   David Voswinekl <david@optixx.org>


Building
--------
To build for the PSP, make sure psp-config is in the path and run:

   ./configure --host=psp --disable-debugger
   make
   make psp-upload
   make psp-layout

Dependencies
------------
    o psp-toolchain
    o pspsdk
    o libsdl
    o libpng

Status
------

   Video    - Support for scaled software mode and framebuffer hardware mode
   Audio    - Working
   Input    - Mouse emulation via Joystick
   Snapshot - Working
   Lauchner - Working
   Menu     - Working
   Debugger - Not useable


Keymap
------

    Menu
        Cross       - Left Mouse Button


    Emulation
        Cross         - Fire
        Circle        - Load State
        Square        - Save State
        Triangle      - Snapshot
        Select        - Console Select
        Start         - Console Reset
				Left Trigger  - Games Menu 
				Right Trigger - Command Menu


Known Bugs
----------
    o SDL video driver is in early stage, so expect flickering and update problems
    o Stella will only compile/run with lastest sdl, pspskd and toolchain.
    o Stella will crash if SDL is compiled with --disable-stdio-redirect.
      Also you need a custom libSDLmain which has a debugHandler for stdout.
			
