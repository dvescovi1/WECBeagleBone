REM --------------------------------------------------------------------------
REM Build Environment
REM --------------------------------------------------------------------------

REM Always copy binaries to flat release directory
set WINCEREL=1
REM Generate .cod, .lst files
set WINCECOD=1

REM ----OS SPECIFIC VERSION SETTINGS----------

if "%SG_OUTPUT_ROOT%" == "" (set SG_OUTPUT_ROOT=%_PROJECTROOT%\cesysgen) 

set BSP_WCE=1
set BUILDDATETIME=%DATE% %TIME%

set BSP_AM33X=1
set BSP_AM33X_BB=1
set BSP_AM33X_BBB=1

REM TI BSP builds its own ceddk.dll. Setting this IMG var excludes default CEDDK from the OS image.
rem set IMGNODFLTDDK=1

REM --------------------------------------------------------------------------
REM Initial Operating Point - VDD1 voltage, MPU (CPU) speeds
REM --------------------------------------------------------------------------

REM Select initial operating point (CPU speed, VDD1 voltage).
REM Note that this controls the operating point selected by the bootloader.
REM If the power management subsystem is enabled, the initial operating point 
REM it uses is controlled by registry entries.
REM The following are choices for AM33x family
REM Use 5 for MPU[1000Mhz @ 1.325V],	L3/L4[200/100Mhz],	CORE @ 1.1V (Nitro)
REM Use 4 for MPU[720Mhz  @ 1.26V], 	L3/L4[200/100Mhz],	CORE @ 1.1V (Turbo)
REM Use 3 for MPU[600Mhz  @ 1.2V], 		L3/L4[200/100Mhz],	CORE @ 1.1V (OPM120)
REM Use 2 for MPU[500Mhz  @ 1.1V], 		L3/L4[200/100Mhz],	CORE @ 1.1V (OPM100)
REM Use 1 for MPU[275Mhz  @ 0.95V],		L3/L4[200/100Mhz],	CORE @ 1.1V (OPM50)
set BSP_OPM_SELECT=4

set BSP_NO_NAND_IN_SDBOOT=1
set BSP_SAVE_EBOOTCFG_TO_SD=1
set BSP_NOTOUCH=1

REM Set to assemble NEON library which requires ARM assembler with arm arch7 support
if "%_WINCEOSVER%" == "700" (set ASSEMBLER_ARM_ARCH7_SUPPORT=1)

