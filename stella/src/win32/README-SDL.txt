
Please distribute this file with the SDL runtime environment:

The Simple DirectMedia Layer (SDL for short) is a cross-platfrom library
designed to make it easy to write multi-media software, such as games and
emulators.

The Simple DirectMedia Layer library source code is available from:
http://www.libsdl.org/

This library is distributed under the terms of the GNU LGPL license:
http://www.gnu.org/copyleft/lesser.html

---------------------------------------------------------------------------

The SDL.dll library included in this package has been modified to
automatically work with the Stelladaptor device.

As per the GPL, all modifications to SDL must be provided and documented.
The included patchfile 'SDL_mmjoystick_1.2.7.diff' can be used with
SDL-1.2.7.zip (located on the main SDL web page).

To rebuild the SDL.dll file, follow the instructions below:

1)  Get the 'SDL-1.2.7.zip' package from the main SDL website and
    unzip it.

2)  Apply the patchfile 'SDL_mmjoystick_1.2.7.diff' to the SDL-1.2.7
    directory (under Linux:  patch -p0 < SDL_mmjoystick_1.2.7.diff).

3)  Build the SDL.dll library from within Visual C++ (you may need the DX5
    development files).


Notes:

1)  Credit for the bulk of the patchfile goes to Glenn Maynard.

2)  Future versions of SDL will hopefully have this patch integrated.

3)  The included SDL.dll file has not been extensively tested.  It is
    believed to be compatible with the most common input devices,
    but support may not be present for buttons on esoteric devices
    (hats, sliders, etc).  If the included version of SDL.dll doesn't
    support your particular input device (and you have no need for
    Stelladaptor support), you can use the standard SDL.dll from
    the main SDL website.
