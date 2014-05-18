// All rights reserved ADENEO EMBEDDED 2010
// main.cpp
//

// Includes
#include <windows.h>
#include <windowsx.h>

#include "bsp_def.h"
#include "image_cfg.h"
#include "bsp.h"
#include "oalex.h"

#define DEVICE_RAM_SIZE_512             0x20000000
#define DEVICE_RAM_SIZE_256             0x10000000

// Defines
#define UNUSED_PARAMETER(x) x
#define WINDOW_CLASS_NAME L"InfoWindowClass"


// Constants
const DWORD WINDOW_WIDTH = 180;
const DWORD WINDOW_HEIGHT = 160;

const DWORD LABEL_X = 10;
const DWORD LABEL_Y = 10;
const DWORD LABEL_WIDTH = 160;
const DWORD LABEL_HEIGHT = 20;
const DWORD LABEL_BSPVER_HEIGHT = 50;

const DWORD OK_BUTTON_X = 60;
const DWORD OK_BUTTON_Y = 100;
const DWORD OK_BUTTON_WIDTH = 60;
const DWORD OK_BUTTON_HEIGHT = 25;


// Global variables
HWND h_window;

HWND h_label_bsp;
HWND h_label_mem;
HWND h_label_cpu;

HWND h_okBtn;

#define PNL_MAX_STRING_LENGTH 255

TCHAR cLabel_bsp[PNL_MAX_STRING_LENGTH] = TEXT("BSP version: ");
TCHAR cLabel_mem[PNL_MAX_STRING_LENGTH] = TEXT("Memory size: ");
TCHAR cLabel_cpu[PNL_MAX_STRING_LENGTH] = TEXT("CPU speed: ");


// Window Procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_COMMAND:
            if (((HWND)lParam == h_okBtn) && (HIWORD(wParam) == BN_CLICKED))
            {
                DestroyWindow(hwnd);
            }
        break;
        case WM_CLOSE:
            DestroyWindow(hwnd);
        break;
        case WM_DESTROY:
            PostQuitMessage(0);
        break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	UNUSED_PARAMETER(hPrevInstance);
	UNUSED_PARAMETER(lpCmdLine);

    WNDCLASS wc;
    MSG Msg;
	TCHAR cItoAbuf[16];
    UINT32 CPUspeed=0;
	UINT32 ramSize;
	DWORD id;
    DWORD ret;

    // Register the Window Class
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc     = WndProc;
    wc.cbClsExtra     = 0;
    wc.cbWndExtra     = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = 0;
    wc.hCursor         = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = WINDOW_CLASS_NAME;
    if(!RegisterClass(&wc))
    {
        MessageBox(NULL, L"Window Registration Failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
        return 1;
    }

    // Find window coordinates
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int windowX = (screenWidth / 2) - (WINDOW_WIDTH / 2);
    int windowY = (screenHeight / 2) - (WINDOW_HEIGHT / 2);

	// Concat the BSP version with the initial string
	wcscat(cLabel_bsp,BSP_VERSION_STRING);

	KernelIoControl(IOCTL_HAL_GET_CPUSPEED,
			(LPVOID)&CPUspeed, sizeof(CPUspeed), (LPVOID)&CPUspeed, 4, &ret);

	// Concat the CPU speed with the initial string
	_ltow(CPUspeed,cItoAbuf,10);
	wcscat(cLabel_cpu,cItoAbuf);
	wcscat(cLabel_cpu,TEXT("Mhz"));

    KernelIoControl(IOCTL_HAL_GET_PLATFORM_ID,
                         NULL, 0, &id, sizeof(DWORD), &ret);

	if (id == AM33X_BOARDID_BBONEBLACK_BOARD)
	{
		ramSize = DEVICE_RAM_SIZE_512;
	}
	else
	{
		ramSize = DEVICE_RAM_SIZE_256;
	}

	// Concat the memory size with the initial string
	_ltow(ramSize/(1024*1024),cItoAbuf,10);
	wcscat(cLabel_mem,cItoAbuf);
	wcscat(cLabel_mem,TEXT("MB"));
	
	

    // Create GUI controls
    h_window = CreateWindow(WINDOW_CLASS_NAME, L"BSP Information", WS_OVERLAPPED, windowX, windowY, WINDOW_WIDTH, WINDOW_HEIGHT, NULL, NULL, hInstance, NULL);

    h_label_bsp = CreateWindow(L"STATIC", cLabel_bsp, WS_VISIBLE | WS_CHILD, LABEL_X, LABEL_Y, LABEL_WIDTH, LABEL_BSPVER_HEIGHT, h_window, 0, hInstance, NULL);
	h_label_cpu = CreateWindow(L"STATIC", cLabel_cpu, WS_VISIBLE | WS_CHILD, LABEL_X, LABEL_Y + (LABEL_BSPVER_HEIGHT), LABEL_WIDTH, LABEL_HEIGHT, h_window, 0, hInstance, NULL);
	h_label_mem = CreateWindow(L"STATIC", cLabel_mem, WS_VISIBLE | WS_CHILD, LABEL_X, LABEL_Y + (LABEL_BSPVER_HEIGHT+LABEL_HEIGHT), LABEL_WIDTH, LABEL_HEIGHT, h_window, 0, hInstance, NULL);

    h_okBtn = CreateWindow(L"BUTTON", L"OK", WS_VISIBLE | WS_CHILD, OK_BUTTON_X, OK_BUTTON_Y, OK_BUTTON_WIDTH, OK_BUTTON_HEIGHT, h_window, 0, hInstance, NULL);

	// Diplay the window
    ShowWindow(h_window, nCmdShow);
    UpdateWindow(h_window);

    // Message Loop
    while(GetMessage(&Msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }

    return Msg.wParam;
}