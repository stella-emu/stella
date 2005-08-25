README for Building StellaOSX
-----------------------------

StellaOSX is configured to be built using Xcode.  The project file for doing
this is in this directory, named stella.pbproj.

The project links the application with two external static libraries.  

The first of these is SDL.  The project expects a copy of the SDL.Framework to
be present in the macosx directory.  It will then link against this
framework, and place a copy of the framework in the Application bundle. The
current version of the SDL framework may be downloaded from www.libsdl.org. The
current release is linked against version 1.2.8 for MacOSX.  The SDL framework 
may be located else where, but the project include and copy framework settings 
would need to be changed.

The second library which the application is linked libpng.  The library source
may be downloaded from http://www.libpng.org/pub/png/libpng.html.  I am
currently using version 1.2.5.  The source will need to be built for MacOSX 
(but not necessarily installed).  The following files will then need to 
be placed in the stella/src/macosx diretory:

   libpng.a
   png.h
   pngconf.h
   
Finally, the application is going to 
   
Mark Grebe

$Id: READMEbuild.txt,v 1.2 2005-08-25 01:22:20 markgrebe Exp $
