# Microsoft Developer Studio Project File - Name="Cyberstella" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=Cyberstella - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Cyberstella.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Cyberstella.mak" CFG="Cyberstella - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Cyberstella - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Cyberstella - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Cyberstella - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\..\build" /I "..\..\emucore" /I "..\..\emucore\m6502\src" /I "..\..\emucore\m6502\src\bspf\src" /I "..\sound" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "BSPF_WIN32" /FR /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x407 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "Cyberstella - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\..\build" /I "..\..\emucore" /I "..\..\emucore\m6502\src" /I "..\..\emucore\m6502\src\bspf\src" /I "..\sound" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "BSPF_WIN32" /FR /FD /GZ /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x407 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "Cyberstella - Win32 Release"
# Name "Cyberstella - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\AboutDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\AudioStream.cxx
# End Source File
# Begin Source File

SOURCE=.\BrowseForFolder.cxx
# End Source File
# Begin Source File

SOURCE=.\CRegBinding.cpp
# End Source File
# Begin Source File

SOURCE=.\Cyberstella.cpp
# End Source File
# Begin Source File

SOURCE=.\Cyberstella.rc
# End Source File
# Begin Source File

SOURCE=.\CyberstellaDoc.cpp
# End Source File
# Begin Source File

SOURCE=.\CyberstellaView.cpp
# End Source File
# Begin Source File

SOURCE=.\DirectDraw.cxx
# End Source File
# Begin Source File

SOURCE=.\DirectInput.cxx
# End Source File
# Begin Source File

SOURCE=.\DirectXFullScreen.cxx
# End Source File
# Begin Source File

SOURCE=.\DirectXWindow.cxx
# End Source File
# Begin Source File

SOURCE=.\GlobalData.cxx
# End Source File
# Begin Source File

SOURCE=.\HyperLink.cpp
# End Source File
# Begin Source File

SOURCE=.\ListData.cxx
# End Source File
# Begin Source File

SOURCE=.\MainFrm.cpp
# End Source File
# Begin Source File

SOURCE=.\pch.cxx
# End Source File
# Begin Source File

SOURCE=.\SoundWin32.cxx
# End Source File
# Begin Source File

SOURCE=.\StellaConfig.cpp
# End Source File
# Begin Source File

SOURCE=.\StellaXMain.cxx
# End Source File
# Begin Source File

SOURCE=.\Timer.cxx
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\AboutDlg.h
# End Source File
# Begin Source File

SOURCE=.\AudioStream.hxx
# End Source File
# Begin Source File

SOURCE=.\BrowseForFolder.hxx
# End Source File
# Begin Source File

SOURCE=.\CRegBinding.h
# End Source File
# Begin Source File

SOURCE=.\Cyberstella.h
# End Source File
# Begin Source File

SOURCE=.\CyberstellaDoc.h
# End Source File
# Begin Source File

SOURCE=.\CyberstellaView.h
# End Source File
# Begin Source File

SOURCE=.\DirectDraw.hxx
# End Source File
# Begin Source File

SOURCE=.\DirectInput.hxx
# End Source File
# Begin Source File

SOURCE=.\DirectXFullScreen.hxx
# End Source File
# Begin Source File

SOURCE=.\DirectXWindow.hxx
# End Source File
# Begin Source File

SOURCE=.\GlobalData.hxx
# End Source File
# Begin Source File

SOURCE=.\HyperLink.h
# End Source File
# Begin Source File

SOURCE=.\ListData.hxx
# End Source File
# Begin Source File

SOURCE=.\MainFrm.h
# End Source File
# Begin Source File

SOURCE=.\pch.hxx
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\SoundWin32.hxx
# End Source File
# Begin Source File

SOURCE=.\StellaConfig.h
# End Source File
# Begin Source File

SOURCE=.\StellaXMain.hxx
# End Source File
# Begin Source File

SOURCE=.\Timer.hxx
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\Cyberstella.ico
# End Source File
# Begin Source File

SOURCE=.\res\Cyberstella.rc2
# End Source File
# Begin Source File

SOURCE=.\res\CyberstellaDoc.ico
# End Source File
# Begin Source File

SOURCE=.\Stella.ico
# End Source File
# Begin Source File

SOURCE=.\res\Toolbar.bmp
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

SOURCE=..\..\emucore\CartCV.cxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\CartCV.hxx
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

SOURCE=..\..\emucore\CartMB.cxx
# End Source File
# Begin Source File

SOURCE=..\..\emucore\CartMB.hxx
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

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
