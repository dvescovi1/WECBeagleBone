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



NOTE: The same image file set (MLO, EBOOTSD.NB0 and NK.BIN) works on the BeagleBone White and Black as executed from a uSD card
or the BeagleBone Black executed from ether a 2Gb or 4Gb eMMC. 
Unlike other BSPs there is no need to build separate images or keep track of several file sets based on which device you are booting from!

Be cautious if using LCD capes as most capes I have seen use expansion IO normally dedicated to UARTS for buttons and other LCD related IO.
For example LCD4 uses UART1 RX (GPIO0_15) signal for the ENTER button. LCD7_4D uses both UART2's RX (GPIO0_2) and TX (GPIO0_3) for LCD_EN 
and ENTER. If using ether of these LCD displays COM1 and COM2 driver will dynamically be removed at run time.
UART4 (COM4) is still available for use and can remain selected in ether LCD case. 


David Vescovi