# Microsoft Developer Studio Project File - Name="Stella" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=Stella - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Stella.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Stella.mak" CFG="Stella - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Stella - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Stella - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Stella - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "..\..\build" /I "..\..\emucore" /I "..\..\emucore\m6502\src" /I "..\..\emucore\m6502\src\bspf\src" /I "..\sound" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "BSPF_WIN32" /Fr /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o /win32 "NUL"
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o /win32 "NUL"
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib /nologo /subsystem:windows /debug /machine:I386

!ELSEIF  "$(CFG)" == "Stella - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /W3 /Gm /GX /Zi /Od /I "..\..\build" /I "..\..\emucore" /I "..\..\emucore\m6502\src" /I "..\..\emucore\m6502\src\bspf\src" /I "..\sound" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "BSPF_WIN32" /FR /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o /win32 "NUL"
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o /win32 "NUL"
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib /nologo /subsystem:windows /profile /debug /machine:I386

!ENDIF 

# Begin Target

# Name "Stella - Win32 Release"
# Name "Stella - Win32 Debug"
# Begin Group "M6502"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\emucore\m6502\src\D6502.cxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\m6502\src\D6502.hxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\m6502\src\Device.cxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\m6502\src\Device.hxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\m6502\src\M6502.cxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\m6502\src\M6502.hxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\m6502\src\M6502Hi.cxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\m6502\src\M6502Hi.hxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\m6502\src\M6502Low.cxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\m6502\src\M6502Low.hxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\m6502\src\NullDev.cxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\m6502\src\NullDev.hxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\m6502\src\System.cxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\m6502\src\System.hxx
# End Source File
# End Group
# Begin Group "Core"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\emucore\Booster.cxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\Booster.hxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\Cart.cxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\Cart.hxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\Cart2K.cxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\Cart2K.hxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\Cart3F.cxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\Cart3F.hxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\Cart4K.cxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\Cart4K.hxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\CartAR.cxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\CartAR.hxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\CartDPC.cxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\CartDPC.hxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\CartE0.cxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\CartE0.hxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\CartE7.cxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\CartE7.hxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\CartF4SC.cxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\CartF4SC.hxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\CartF6.cxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\CartF6.hxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\CartF6SC.cxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\CartF6SC.hxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\CartF8.cxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\CartF8.hxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\CartF8SC.cxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\CartF8SC.hxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\CartFASC.cxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\CartFASC.hxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\CartFE.cxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\CartFE.hxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\CartMC.cxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\CartMC.hxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\Console.cxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\Console.hxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\Control.cxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\Control.hxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\DefProps.cxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\DefProps.hxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\Driving.cxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\Driving.hxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\Event.cxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\Event.hxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\Joystick.cxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\Joystick.hxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\Keyboard.cxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\Keyboard.hxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\M6532.cxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\M6532.hxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\MD5.cxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\MD5.hxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\MediaSrc.cxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\MediaSrc.hxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\Paddles.cxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\Paddles.hxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\Props.cxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\Props.hxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\PropsSet.cxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\PropsSet.hxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\Random.cxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\Random.hxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\Sound.cxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\Sound.hxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\Switches.cxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\Switches.hxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\TIA.cxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\TIA.hxx
# End Source File
# End Group
# Begin Group "Win32"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\AboutPage.cxx
# End Source File
# Begin Source File

SOURCE=.\AboutPage.hxx
# End Source File
# Begin Source File

SOURCE=.\AudioStream.cxx
# End Source File
# Begin Source File

SOURCE=.\AudioStream.hxx
# End Source File
# Begin Source File

SOURCE=.\BrowseForFolder.cxx
# End Source File
# Begin Source File

SOURCE=.\BrowseForFolder.hxx
# End Source File
# Begin Source File

SOURCE=.\ConfigPage.cxx
# End Source File
# Begin Source File

SOURCE=.\ConfigPage.hxx
# End Source File
# Begin Source File

SOURCE=.\ControlHost.cxx
# End Source File
# Begin Source File

SOURCE=.\ControlHost.hxx
# End Source File
# Begin Source File

SOURCE=.\CoolCaption.cxx
# End Source File
# Begin Source File

SOURCE=.\CoolCaption.hxx
# End Source File
# Begin Source File

SOURCE=.\debug.cxx
# End Source File
# Begin Source File

SOURCE=.\debug.hxx
# End Source File
# Begin Source File

SOURCE=.\DirectDraw.cxx
# End Source File
# Begin Source File

SOURCE=.\DirectDraw.hxx
# End Source File
# Begin Source File

SOURCE=.\DirectInput.cxx
# End Source File
# Begin Source File

SOURCE=.\DirectInput.hxx
# End Source File
# Begin Source File

SOURCE=.\DirectXFullScreen.cxx
# End Source File
# Begin Source File

SOURCE=.\DirectXFullScreen.hxx
# End Source File
# Begin Source File

SOURCE=.\DirectXWindow.cxx
# End Source File
# Begin Source File

SOURCE=.\DirectXWindow.hxx
# End Source File
# Begin Source File

SOURCE=.\DocPage.cxx
# End Source File
# Begin Source File

SOURCE=.\DocPage.hxx
# End Source File
# Begin Source File

SOURCE=.\FileDialog.cxx
# End Source File
# Begin Source File

SOURCE=.\FileDialog.hxx
# End Source File
# Begin Source File

SOURCE=.\GlobalData.cxx
# End Source File
# Begin Source File

SOURCE=.\GlobalData.hxx
# End Source File
# Begin Source File

SOURCE=.\HeaderCtrl.cxx
# End Source File
# Begin Source File

SOURCE=.\HeaderCtrl.hxx
# End Source File
# Begin Source File

SOURCE=.\HyperLink.cxx
# End Source File
# Begin Source File

SOURCE=.\HyperLink.hxx
# End Source File
# Begin Source File

SOURCE=.\main.cxx
# End Source File
# Begin Source File

SOURCE=.\MainDlg.cxx
# End Source File
# Begin Source File

SOURCE=.\MainDlg.hxx
# End Source File
# Begin Source File

SOURCE=.\pch.cxx
# End Source File
# Begin Source File

SOURCE=.\pch.hxx
# End Source File
# Begin Source File

SOURCE=.\PropertySheet.cxx
# End Source File
# Begin Source File

SOURCE=.\PropertySheet.hxx
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\RoundButton.cxx
# End Source File
# Begin Source File

SOURCE=.\RoundButton.hxx
# End Source File
# Begin Source File

SOURCE=.\SoundWin32.cxx
# End Source File
# Begin Source File

SOURCE=.\SoundWin32.hxx
# End Source File
# Begin Source File

SOURCE=.\stella.rc
# End Source File
# Begin Source File

SOURCE=.\StellaXMain.cxx
# End Source File
# Begin Source File

SOURCE=.\StellaXMain.hxx
# End Source File
# Begin Source File

SOURCE=.\TextButton3d.cxx
# End Source File
# Begin Source File

SOURCE=.\TextButton3d.hxx
# End Source File
# Begin Source File

SOURCE=.\Timer.cxx
# End Source File
# Begin Source File

SOURCE=.\Timer.hxx
# End Source File
# Begin Source File

SOURCE=.\Wnd.cxx
# End Source File
# Begin Source File

SOURCE=.\Wnd.hxx
# End Source File
# End Group
# Begin Group "Sound"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sound\TIASound.c
# End Source File
# Begin Source File

SOURCE=..\sound\TIASound.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\emucore\m6502\src\bspf\src\bspf.hxx
# End Source File
# Begin Source File

SOURCE=..\..\build\M6502Hi.ins
# End Source File
# Begin Source File

SOURCE=..\..\build\M6502Low.ins
# End Source File
# Begin Source File

SOURCE=.\STELLA.ICO
# End Source File
# Begin Source File

SOURCE=.\tile.bmp
# End Source File
# End Target
# End Project
