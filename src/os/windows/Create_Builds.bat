@echo off & setlocal enableextensions

echo This will create an InnoSetup installer for 64-bit Stella
echo as well as a ZIP archive.
echo.
echo  ! InnoSetup must be linked to this directory as 'iscc.lnk'
echo  ! 'zip.exe' must be installed in this directory (for ZIP files)
echo.
echo  !!! Make sure the code has already been compiled in Visual Studio
echo  !!! before launching this command.
echo.

:: Make sure all tools are available
set HAVE_ISCC=1
set HAVE_ZIP=1

:: Make sure InnoSetup and/or ZIP are available
if not exist "iscc.lnk" (
	echo InnoSetup 'iscc.lnk' not found - EXE files will not be created
	set HAVE_ISCC=0
)
if not exist "zip.exe" (
	echo ZIP command not found - ZIP files will not be created
	set HAVE_ZIP=0
)
if %HAVE_ISCC% == 0 (
	if %HAVE_ZIP% == 0 (
		echo Both EXE and ZIP files cannot be created, exiting
		goto done
	)
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
copy %RELEASE%\Stella.exe %STELLA_DIR%
copy %RELEASE%\*.dll      %STELLA_DIR%

echo Copying DOC files ...
xcopy ..\..\..\docs\* %STELLA_DIR%\docs /s /q
copy ..\..\..\*.txt   %STELLA_DIR%\docs
copy ..\..\..\*.md    %STELLA_DIR%\docs

:: Create output directory for release files
if not exist Output (
	echo Creating output directory ...
	mkdir Output
)

:: Actually create the ZIP file
if %HAVE_ZIP% == 1 (
	echo Creating ZIP file ...
	zip -q -r Output\%STELLA_DIR%-windows.zip %STELLA_DIR%
)
:: Create the Inno EXE files
if %HAVE_ISCC% == 1 (
	echo Creating InnoSetup EXE ...
	iscc.lnk "%CD%\stella.iss" /q "/dSTELLA_VER=%STELLA_VER%" "/dSTELLA_PATH=%STELLA_DIR%" "/dSTELLA_DOCPATH=%STELLA_DIR%\docs"
)

:: Cleanup time
echo Cleaning up files ...
rmdir %STELLA_DIR% /s /q

:done
echo.
pause 5
