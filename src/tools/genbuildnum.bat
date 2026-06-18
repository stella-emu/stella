@echo off
rem ===========================================================================
rem
rem   SSSS    tt          lll  lll
rem  SS  SS   tt           ll   ll
rem  SS     tttttt  eeee   ll   ll   aaaa
rem   SSSS    tt   ee  ee  ll   ll      aa
rem      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
rem  SS  SS   tt   ee      ll   ll  aa  aa
rem   SSSS     ttt  eeeee llll llll  aaaaa
rem
rem Copyright (c) 1995-2026 by Bradford W. Mott, Stephen Anthony
rem and the Stella Team
rem
rem See the file "License.txt" for information on usage and redistribution of
rem this file, and for a DISCLAIMER OF ALL WARRANTIES.
rem ===========================================================================

rem Generate src\common\VersionBuild.hxx with the build number taken from the
rem git commit count (the same value GitHub reports). The header is only
rem rewritten when the value actually changes, so the few objects that include
rem it aren't needlessly recompiled. When git is unavailable (e.g. building from
rem a release tarball), the existing file is left untouched and the fallback
rem value in Version.hxx is used instead.

setlocal
set "OUT=%~dp0..\common\VersionBuild.hxx"

set "BUILD="
for /f "delims=" %%i in ('git -C "%~dp0." rev-list --count HEAD 2^>nul') do set "BUILD=%%i"
if not defined BUILD exit /b 0

> "%OUT%.tmp" echo #define STELLA_BUILD_INCLUDED
>> "%OUT%.tmp" echo static constexpr std::string_view STELLA_BUILD_NUMBER = "%BUILD%";
if exist "%OUT%" (
  fc /b "%OUT%.tmp" "%OUT%" >nul 2>&1 && (
    del "%OUT%.tmp"
    exit /b 0
  )
)
move /y "%OUT%.tmp" "%OUT%" >nul
exit /b 0
