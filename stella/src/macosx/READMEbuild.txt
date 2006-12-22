README for Building StellaOSX
-----------------------------

StellaOSX is configured to be built using Xcode.  The project file for doing
this is in this directory, named stella.pbproj.

The project links the application with SDL, a static library.

The project expects a copy of the SDL.Framework to be present in the macosx
directory.  It will then link against this framework, and place a copy of the
framework in the Application bundle. The current version of the SDL framework
may be downloaded from www.libsdl.org. The current release is linked against
version 1.2.11 for MacOSX.  The SDL framework may be located else where, but
the project include and copy framework settings  would need to be changed.

Mark Grebe

$Id: READMEbuild.txt,v 1.4 2006-12-22 23:14:39 stephena Exp $
