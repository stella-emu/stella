EVDEV-JOYSTICK
--------------

This program is based on G25manage, located at:
  https://github.com/VDrift/vdrift/tree/master/tools/G25manage

It is developed by Stephen Anthony, and released under the GPLv/2.

evdev-joystick is used to set the deadzone for Linux 'evdev' joystick
devices.  Currently, other than G25manage there is no other standalone
program available to perform such calibration.  This program was
originally developed for Stella (stella.sf.net), an Atari 2600 emulator,
and as such much of this document refers to Stella.  The program itself
can be used to calibrate any joystick for any application, though, and
is not specific to Stella.


Short Explanation (Stella users with Stelladaptor, 2600-daptor, etc.)
-----------------

  - Decompress the archive
  - Build the application by typing 'make'
  - Install it by typing 'sudo make install'
  - Unplug your 'daptor device, re-plug them, and play a game.


Long Explanation (For the curious, or if something doesn't work, etc.)
----------------

  - Decompress the archive
  - Build the application by typing 'make'

  - Type './evdev-joystick --l'.  For me, it produces output as follows:


  - Make note of the name of the device.  For 2600-daptor II users, this
    would be:  
