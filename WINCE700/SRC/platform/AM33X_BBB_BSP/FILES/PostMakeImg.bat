@REM
@REM Copy needed built files to "built" directory
@REM under the OSDESIGN folder
@REM
@echo off

if not "%WINCEDEBUG%"=="retail" goto TheEnd
pushd %_FLATRELEASEDIR%

cd "%_OSDESIGNROOT%"
cd ..\

if not exist built MD built
cd built

echo deleting old built files

if exist MLO del /f /q MLO
if exist MLO del /f /q MLOemmc
if exist EBOOTSD.nb0 del /f /q EBOOTSD.nb0
if exist NK.bin del /f /q NK.bin
if exist logo.bmp del /f /q logo.bmp

copy /y %_FLATRELEASEDIR%\MLO
copy /y %_FLATRELEASEDIR%\EBOOTSD.nb0
copy /y %_FLATRELEASEDIR%\NK.bin
if exist %_FLATRELEASEDIR%\logo.bmp copy /y %_FLATRELEASEDIR%\logo.bmp


popd

:TheEnd


