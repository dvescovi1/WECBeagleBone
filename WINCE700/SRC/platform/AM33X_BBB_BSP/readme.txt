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



NOTE: The same image file set (MLO, EBOOTSD.NB0 and NK.BIN) works on the BeagleBone White and Black as executed from a uSD card
or the BeagleBone Black executed from ether a 2Gb or 4Gb eMMC. 
Unlike other BSPs there is no need build separate images or keep track of several file sets based on which device you are booting from!

David Vescovi