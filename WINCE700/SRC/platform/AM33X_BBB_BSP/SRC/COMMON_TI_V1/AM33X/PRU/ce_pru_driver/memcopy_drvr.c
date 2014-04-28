
#include <windows.h>
#include <winreg.h>
#include <devload.h>
#include <types.h>
#include <ceddk.h>
#include <math.h>
#include <pm.h>
#include <nkintr.h>

#include <pru.h>

#include <pru_drvr_api.h>
#include "am33x_base_regs.h"
#include "pruioctl.h"
#include "pru_drvr.h"


//-------------------------------
// common for all examples
//-------------------------------
#define PRU_NUM 	0

static void *pruDataMem;
static unsigned int *pruDataMem_int;
static unsigned char *pruDataMem_char;
//static PRUDEVICESTATE  pPDUDevice = NULL;

//-------------------------------
// memCopy example
//-------------------------------

#define SIZE 		8
#define	SOURCE_ADDR 0x00000010 
#define	DEST_ADDR   0x00000040

static unsigned int destPtr, srcPtr, src_offset, dest_offset;

typedef struct
{
    unsigned short pruNum;
    unsigned short exampleNum;
}  MEMCOPY_ARGS;


int memCopy_Init(PRUDEVICESTATE *pPRUDevice, unsigned short pruNum, unsigned short exampleNum)
{
    unsigned int i;  

    switch(exampleNum)
    {
        // Source and destination addresses are aligned
        case 0:
            RETAILMSG(1, (L"INFO: Initializing aligned example.\r\n"));
            src_offset = 0x00000000;
            dest_offset = 0x00000000;
            break;
        // Source and destination addresses not aligned, but offset 
        // from aligned address are same
        case 1:
            RETAILMSG(1, (L"INFO: Initializing same offset example.\r\n"));
            src_offset = 0x00000003;
            dest_offset = 0x00000003;
            break;
        // Source and destination addresses not aligned, and offset 
        // from aligned address are different
        case 2:
            RETAILMSG(1, (L"INFO: Initializing different offset example.\r\n"));
            src_offset = 0x00000001;
            dest_offset = 0x00000003;
            break;
        default:
            return -1;
    }
        
    srcPtr = SOURCE_ADDR + src_offset;
    destPtr = DEST_ADDR + dest_offset;

    //Initialize pointer to PRU data memory
    if (pruNum == 0)
    {
//        prussdrv_map_prumem (PRUSS0_PRU0_DATARAM, &pruDataMem);
        pruDataMem = pPRUDevice->pru_iomap.pru0_dataram_base;
    }
    else if (pruNum == 1)
    {
//        prussdrv_map_prumem (PRUSS0_PRU1_DATARAM, &pruDataMem);
        pruDataMem = pPRUDevice->pru_iomap.pru1_dataram_base;
    }  
    pruDataMem_int = (unsigned int*) pruDataMem;
    pruDataMem_char = (char*) pruDataMem;
    
    pruDataMem_int[0] = SIZE;
    pruDataMem_int[1] = srcPtr;
    pruDataMem_int[2] = destPtr;

    srand(GetTickCount());

    // Store values into source address
    for (i = 0; i < SIZE; i++)
    {
        pruDataMem_char[srcPtr+i] = rand() & 0xFF;
        pruDataMem_char[destPtr+i] = 0xFF;
    }

    RETAILMSG(1,(L"\r\n"));
    RETAILMSG(1,(L"Buffers before copy\r\n"));
    RETAILMSG(1,(L"src (@0x%08X+0x%08X): %02X %02X %02X %02X %02X %02X %02X %02X \r\n", 
        SOURCE_ADDR, src_offset,
        pruDataMem_char[srcPtr], pruDataMem_char[srcPtr+1], 
        pruDataMem_char[srcPtr+2], pruDataMem_char[srcPtr+3],
        pruDataMem_char[srcPtr+4], pruDataMem_char[srcPtr+5], 
        pruDataMem_char[srcPtr+6], pruDataMem_char[srcPtr+7] ));

    RETAILMSG(1,(L"dst (@0x%08X+0x%08X): %02X %02X %02X %02X %02X %02X %02X %02X \r\n", 
        DEST_ADDR, dest_offset,
        pruDataMem_char[destPtr], pruDataMem_char[destPtr+1], 
        pruDataMem_char[destPtr+2], pruDataMem_char[destPtr+3],
        pruDataMem_char[destPtr+4], pruDataMem_char[destPtr+5], 
        pruDataMem_char[destPtr+6], pruDataMem_char[destPtr+7]) );

    return(0);
}

unsigned short memCopy_Passed(unsigned short pruNum)
{
    unsigned int i;
    
    RETAILMSG(1,(L"Buffers after copy\r\n"));
    RETAILMSG(1,(L"src: %02X %02X %02X %02X %02X %02X %02X %02X \r\n", 
        pruDataMem_char[srcPtr], pruDataMem_char[srcPtr+1], 
        pruDataMem_char[srcPtr+2], pruDataMem_char[srcPtr+3],
        pruDataMem_char[srcPtr+4], pruDataMem_char[srcPtr+5], 
        pruDataMem_char[srcPtr+6], pruDataMem_char[srcPtr+7]) );


    RETAILMSG(1,(L"dst: %02X %02X %02X %02X %02X %02X %02X %02X \r\n", 
        pruDataMem_char[destPtr], pruDataMem_char[destPtr+1], 
        pruDataMem_char[destPtr+2], pruDataMem_char[destPtr+3],
        pruDataMem_char[destPtr+4], pruDataMem_char[destPtr+5], 
        pruDataMem_char[destPtr+6], pruDataMem_char[destPtr+7]) );

    for (i=0; i < SIZE; i++)
     {
       if (pruDataMem_char[srcPtr+i] != pruDataMem_char[destPtr+i])
       {
         RETAILMSG(1,(L"memCopy_Passed - differ at i=%d\r\n", i) );
         return 0;
       }
    }
    return 1;
}

unsigned long memCopy_PRU(void)
{
    return 0x00000001;
}

int memCopy_Init0(void *pArgs)
{
    return (memCopy_Init((PRUDEVICESTATE *)pArgs, 0, 0));
}

unsigned short memCopy_Passed0(void *pArgs)
{
    return (memCopy_Passed(0));
}

int memCopy_Init1(void *pArgs)
{
    return (memCopy_Init((PRUDEVICESTATE *)pArgs, 0, 1));
}

unsigned short memCopy_Passed1(void *pArgs)
{
    return (memCopy_Passed(0));
}


int memCopy_Init2(void *pArgs)
{
    return (memCopy_Init((PRUDEVICESTATE *)pArgs, 0, 2));
}

unsigned short memCopy_Passed2(void *pArgs)
{
    return (memCopy_Passed(0));
}


