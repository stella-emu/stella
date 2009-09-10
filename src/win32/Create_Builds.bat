@echo off & setlocal enableextensions

echo Stella build script for creating win32 and x64 builds.
echo This will create installers (based on InnoSetup) for both 32 and 64-bit,
echo as well as a ZIP archive containing both versions.
echo.
echo  ! The 'zip' utility must be installed in your path.
echo  ! InnoSetup should be linked to this directory as 'iscc.lnk'
echo.
echo  !!! Make sure the code has already been compiled in Visual Studio
echo  !!! before launching this command.
echo.

set RELEASE_32=Release
set RELEASE_64=x64\Release

set /p STELLA_VER=Enter Stella version: 
echo.
set /p TO_BUILD=Version to build (32/64/a=all): 

set BUILD_32=0
set BUILD_64=0
if %TO_BUILD% == 32 (
	set BUILD_32=1
)

if %TO_BUILD% == 64 (
	set BUILD_64=1
)

if %TO_BUILD% == a (
	set BUILD_32=1
	set BUILD_64=1
)

if %BUILD_32% == 1 (
	if not exist %RELEASE_32% (
		echo The 32-bit build was not found in the '%RELEASE_32%' directory
		goto done
	)
)

if %BUILD_64% == 1 (
	if not exist %RELEASE_64% (
		echo The 64-bit build was not found in the '%RELEASE_64%' directory
		goto done
	)
)

:: Create ZIP folder first
set STELLA_DIR=stella-%STELLA_VER%
if exist %STELLA_DIR% (
	echo Removing old %STELLA_DIR% directory
	rmdir /s /q %STELLA_DIR%
)
echo Creating %STELLA_DIR% ...
mkdir %STELLA_DIR%
mkdir %STELLA_DIR%\32-bit
mkdir %STELLA_DIR%\64-bit
mkdir %STELLA_DIR%\docs

if %BUILD_32% == 1 (
	echo Copying 32-bit files to ZIP ...
	copy %RELEASE_32%\Stella.exe  %STELLA_DIR%\32-bit
	copy %RELEASE_32%\SDL.dll     %STELLA_DIR%\32-bit
	copy %RELEASE_32%\zlibwapi.dll %STELLA_DIR%\32-bit
)

if %BUILD_64% == 1 (
	echo Copying 64-bit files to ZIP ...
	copy %RELEASE_64%\Stella.exe  %STELLA_DIR%\64-bit
	copy %RELEASE_64%\SDL.dll     %STELLA_DIR%\64-bit
	copy %RELEASE_64%\zlibwapi.dll %STELLA_DIR%\64-bit
)

echo Copying DOC files ...
xcopy ..\..\docs\* %STELLA_DIR%\docs /s /q
copy ..\..\Announce.txt   %STELLA_DIR%\docs
copy ..\..\Changes.txt    %STELLA_DIR%\docs
copy ..\..\Copyright.txt  %STELLA_DIR%\docs
copy ..\..\License.txt    %STELLA_DIR%\docs
copy ..\..\Readme.txt     %STELLA_DIR%\docs
copy ..\..\README-SDL.txt %STELLA_DIR%\docs
copy ..\..\Todo.txt       %STELLA_DIR%\docs
::for %%a in (%STELLA_DIR%\docs\*.txt) do (
::	flip -d "%%a"
::)

:: Actually create the ZIP file
echo Creating ZIP file ...
zip -q -r Output\%STELLA_DIR%-windows.zip %STELLA_DIR%

:: Now create the Inno EXE files
if not exist "iscc.lnk" (
	echo InnoSetup 'iscc.lnk' not found - EXE files not created
	goto done
)
if %BUILD_32% == 1 (
	echo Creating 32-bit EXE ...
	iscc.lnk %CD%\stella.iss /q "/dSTELLA_VER=%STELLA_VER%" "/dSTELLA_ARCH=win32" "/dSTELLA_PATH=%STELLA_DIR%\32-bit" "/dSTELLA_DOCPATH=%STELLA_DIR%\docs"
)
if %BUILD_64% == 1 (
	echo Creating 64-bit EXE ...
	iscc.lnk %CD%\stella.iss /q "/dSTELLA_VER=%STELLA_VER%" "/dSTELLA_ARCH=x64" "/dSTELLA_PATH=%STELLA_DIR%\64-bit" "/dSTELLA_DOCPATH=%STELLA_DIR%\docs"
)

:: Cleanup time
echo Removing %STELLA_DIR% ...
rmdir %STELLA_DIR% /s /q

:done
echo.
pause
