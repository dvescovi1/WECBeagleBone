// All rights reserved ADENEO EMBEDDED 2010
#include "bsp.h"
#include <initguid.h>
#include "sdk_gpio.h"
#include "bsp_cfg.h"
#include "ceddkex.h"

extern DEVICE_IFC_GPIO Am3xx_Gpio;

void BSPGpioInit()
{
//    BSPInsertGpioDevice( 0,&Am3xx_Gpio,L"GIO1:");   // Am3xx GPIOs
    BSPInsertGpioDevice(0,NULL,L"GIO1:");   // Omap GPIOs
    //BSPInsertGpioDevice(GPIO_EXPANDER_1_PINID_START,NULL,L"GIO2:");   // GPIO expander 1
}
