@echo off & setlocal enableextensions

echo This will create an InnoSetup installer for 64-bit Stella.
echo.
echo  ! InnoSetup must be linked to this directory as 'iscc.lnk'
echo.
echo  !!! Make sure the code has already been compiled in Visual Studio
echo  !!! before launching this command.
echo.

:: Make sure InnoSetup is available
if not exist "iscc.lnk" (
	echo InnoSetup 'iscc.lnk' not found
	goto done
)

set RELEASE=x64\Release

echo.
set /p STELLA_VER=Enter Stella version:

if not exist %RELEASE% (
	echo The executable was not found in the '%RELEASE%' directory
	goto done
)

:: Create ZIP folder first
set STELLA_DIR=Stella-%STELLA_VER%
if exist %STELLA_DIR% (
	echo Removing old %STELLA_DIR% directory
	rmdir /s /q %STELLA_DIR%
)
echo Creating %STELLA_DIR% ...
mkdir %STELLA_DIR%
mkdir %STELLA_DIR%\docs

echo Copying executable files ...
copy %RELEASE%\Stella.exe   %STELLA_DIR%
copy %RELEASE%\*.dll        %STELLA_DIR%

echo Copying DOC files ...
xcopy ..\..\..\docs\* %STELLA_DIR%\docs /s /q
copy ..\..\..\Announce.txt   %STELLA_DIR%\docs
copy ..\..\..\Changes.txt    %STELLA_DIR%\docs
copy ..\..\..\Copyright.txt  %STELLA_DIR%\docs
copy ..\..\..\License.txt    %STELLA_DIR%\docs
copy ..\..\..\Readme.md      %STELLA_DIR%\docs
copy ..\..\..\README-SDL.txt %STELLA_DIR%\docs
copy ..\..\..\Todo.txt       %STELLA_DIR%\docs

:: Create output directory for release files
if not exist Output (
	echo Creating output directory ...
	mkdir Output
)

:: Create the Inno EXE files
echo Creating InnoSetup EXE ...
iscc.lnk "%CD%\stella.iss" /q "/dSTELLA_VER=%STELLA_VER%" "/dSTELLA_PATH=%STELLA_DIR%" "/dSTELLA_DOCPATH=%STELLA_DIR%\docs"

:: Cleanup time
echo Cleaning up files ...
rmdir %STELLA_DIR% /s /q

:done
echo.
pause 5
