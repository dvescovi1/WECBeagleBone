Copyright (c) 2014 David Vescovi


01.02.00	30-JAN-2014
	-Initial release
	
01.03.00	01-FEB-2014
	-Added EnableUnalignedAccess code.
	-Added always set OPP code
	-Added dynamic BB/BBB support
	-Added cape detect
	
01.04.00	08-FEB-2014
	-Code cleanup
	-Added SDK
	
01.05.00	18-MAY-2014
	-Code cleanup
	-Corrected BBB DCDC2 voltage at @1Ghz
	-Added PowerVR support
	-Added XAML,Silverlight and OpenGL support
	
01.06.00	23-MAY-2014
	-Rework I2C proxy to support I2C 1,2,3
	
01.07.00	04-JUN-2014
	-Rework and simplify CSP directory

01.08.00	13-JUN-2014
	-More rework and simplify CSP directory
	-OS tic now uses 32Khz osc.

01.09.00	25-JUN-2014
	-Added LCD4 support
	-Added touch screen support
	-GPIO pad init
	
01.10.00	14-JUL-2014
	-Rework xldr
	-eMMC support
	-custom DISKPARTEMMC utility to support eMMC
	-Corrected I2C proxy registry settings
	-Corrected SPI

01.11.00	23-AUG-2014
	-UART4 support
	-EDMA cleanup
	-SDHC3 support
	-Tests cleanup

01.12.00	27-SEP-2014
	-Power button support
	-Battery support
	=4D Systems LCD7 support 

01.13.00	7-NOV-2014
	-PWM support
	-Added PWM and GPIO test programs
	-Added boot splash screen and load progress bar support
	-Added screen orientation support
	-Corrected ReleasePad and ConfigurePad inUse flag
	-Added more GPIO pin support


NOTE: The same image file set (MLO, EBOOTSD.NB0 and NK.BIN) works on the BeagleBone White and Black as executed from a uSD card
or the BeagleBone Black executed from ether a 2Gb or 4Gb eMMC. 
Unlike other BSPs there is no need to build separate images or keep track of several file sets based on which device you are booting from!

Be cautious if using LCD capes as most capes I have seen use expansion IO normally dedicated to UARTS for buttons and other LCD related IO.
For example LCD4 uses UART1 RX (GPIO0_15) signal for the ENTER button. LCD7_4D uses both UART2's RX (GPIO0_2) and TX (GPIO0_3) for LCD_EN 
and ENTER. If using ether of these LCD displays COM1 and COM2 driver will dynamically be removed at run time.
UART4 (COM4) is still available for use and can remain selected in ether LCD case. 

GPIO pins on the expansion connector and those labeled UARTxx, TIMERx and EHRPWMxx are by default configured as GPIO (input with pull up)
on power up.
This configuration is overridden if the alternate use driver is selected and used. For example, if the UART1 is selected from the catalog
the UART1_TX and UART1_RX pins will be reconfigured for UART use upon driver loading.

Boot logo screens (logo.bmp), if used, must be properly formatted to the corrected resolution and use 16bit pixel depth. Also, the bmp image
MUST be formatted 565. Windows PAINT and some others image utilities DO NOT correctly format and pack pixels. The "GIMP" open source utility
imaging program does handle this. After loading the image in GIMP do "Export" to BMP file then click "Advanced Options" and select "16 bit R5 G6 B5". 


David Vescovi