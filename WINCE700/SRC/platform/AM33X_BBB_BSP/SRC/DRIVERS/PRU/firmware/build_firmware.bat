@echo off

rem [variables to firmware source and local directories]
set SOC_PRU_PATH=%_TARGETPLATROOT%\SRC\COMMON_TI_V1\AM33X\PRU
set PROJECT_PLAT_PATH=%_TARGETPLATROOT%\SRC\DRIVERS\PRU

rem path relative to %SOC_PRU_PATH%
set APP_PATH=firmware\example_apps



rem file names without extension
set APPS=PRU_multiply, PRU_memCopy

@FOR %%A IN (%APPS%) DO @(

rem [create the #define to select which MCASP to use in firmware assembly]
@echo Assembling PRU firmware %%A

rem [assemble firmware]
%SOC_PRU_PATH%\utils\pru-as\windows\pasm -b %SOC_PRU_PATH%\%APP_PATH%\%%A\%%A.p %PROJECT_PLAT_PATH%\firmware\bin\%%A

)

rem [copy firmwares from local bin directory to the target]
copy %PROJECT_PLAT_PATH%\firmware\bin\*.bin %_TARGETPLATROOT%\FILES\*.fw

@echo Done
