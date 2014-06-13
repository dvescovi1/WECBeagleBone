
#define NUM_PRU_HOSTIRQS 8

// Structure for loading the firmware from a file
typedef struct
{
	DWORD  dwSize;
	LPVOID lpData;

} PRU_FIRMWARE;


// This structure keeps track of the device instance data
typedef struct _PRUDeviceState_tag
{
    CRITICAL_SECTION        csDevice;       // serialize access to this device's state
    HANDLE                  hevStop;        // when signaled, the device thread exits
    CEDEVICE_POWER_STATE    CurrentDx;      // current power level

	// Registry settings
	WCHAR					FirmwareFile[MAX_PATH];
	DWORD					irq_base;		// First IRQ of sequential range
    DWORD                   dwBaseAddress;  // Base address of PRU peripheral registers
	DWORD					ist_priority;	// IST thread priority

	// PRU control
	PRU_FIRMWARE			Firmware;		// PRU firmware loaded from file system
	arm_pru_iomap			pru_iomap;		// Configuration of PRU subsystem
    HANDLE                  irq_thread[NUM_PRU_HOSTIRQS];
    DWORD                   active_thread;

} PRUDEVICESTATE, *PPRUDEVICESTATE;


typedef struct _FirmwareControl_tag
{
    PWSTR pFname;
    // returns which PRU needed by example: OR'd of 0x00000001 and 0x00000002
    unsigned long (* LOCAL_examplePru)(void);
    int (* LOCAL_exampleInit) ( void *pArgs );
    unsigned short (* LOCAL_examplePassed) ( void *pArgs );
    PWSTR pDesc;
}  FIRMWARE_CONTROL;

