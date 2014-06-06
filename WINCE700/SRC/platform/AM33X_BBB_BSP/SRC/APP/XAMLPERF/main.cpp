//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this sample source code is subject to the terms of the Microsoft
// license agreement under which you licensed this sample source code. If
// you did not accept the terms of the license agreement, you are not
// authorized to use this sample source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the LICENSE.RTF on your install media or the root of your tools installation.
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
// Copyright (c) 2010 Texas Instruments Incorporated - http://www.ti.com/
// All rights reserved.
//

#include <windows.h>
#include <XamlRuntime.h>
#include <XRDelegate.h>
#include <XRPtr.h>
#include <Ehm.h>
#include "cpumon.h"


#define GetLastErrorHR() HRESULT_FROM_WIN32(GetLastError())

// This helps avoid a linker error
const IID IID_IUnknown = __uuidof(IUnknown);

const WCHAR c_FrameRateMessageName[]   = L"XR_FRAMERATE_MONITOR";
UINT g_InitialMemUsage = 0;
BOOL g_bRotation = FALSE;
BOOL g_VRFB = FALSE;

#define RK_VRFB L"Drivers\\BuiltIn\\VRFB"

// Will get the system memory usage
//
UINT GetMemoryUsage()
{
    MEMORYSTATUS statex;

    statex.dwLength = sizeof (statex);

    GlobalMemoryStatus(&statex);

    return (UINT)statex.dwAvailPhys;
    
}

//this class will initialize and uninitialize XamlRuntime
//
class XamlRuntimeManager
{
private:
    BOOL m_IsInitialized;

public:
    XamlRuntimeManager()
    {
        g_InitialMemUsage = GetMemoryUsage();
        m_IsInitialized = XamlRuntimeInitialize();
    }

    ~XamlRuntimeManager()
    {
        if(m_IsInitialized)
        {
            XamlRuntimeUninitialize();
        }
    }

    BOOL IsInitialized() { return m_IsInitialized; }

};

// FrameRateManager gets the frames per second and hooks up the statistics xaml
// into the application
//
class FrameRateManager
{

    enum FrameRateMonitorMessage
    {
        FRM_START = 0,
        FRM_STOP,
        FRM_GET_FRAMES_PER_SECOND
    };
    
public:
    FrameRateManager()
    {
        m_MinFrameRate = (UINT)-1;
        m_MaxFrameRate = 0;
        m_AvgFrameRate = 0;
        m_FrameRateCount = 0;
        m_RunningSum = 0;
        m_Hwnd = NULL;
        m_pHost = NULL;
        m_pTextBlock = NULL;
        m_pCpuTextBlock = NULL;
        m_TestMessage = 0;
        m_DisplayWidth = 0;
        m_DisplayHeight = 0;        

        m_MostRecentFramerate = -1;
    }  

    // Add the stats xaml to the user provided xaml
    // also register the FPS messages with the host
    HRESULT Init(IXRApplicationPtr pApp, IXRVisualHost* pHost)
    {
        HRESULT hr = S_OK;
        
        m_pHost = pHost;

        //lets set up the FPS
        CHR(m_pHost->GetContainerHWND(&m_Hwnd));
        CHR(RegisterMessage());

        //now lets set up CPU usage
        CHR(CalibrateCPUMon());
        CHR(StartCPUMonitor());
        
        //lets add our stats grid now
        CHR(AddStatisticsGridToHost(pApp));       

    Error:
        return hr;
    }

    // cleanup member variables
    void Cleanup()
    {
        StopCPUMonitor();
        m_pHost = NULL;
        m_pTextBlock = NULL;
        m_pCpuTextBlock = NULL;
    }

    //average of FPS
    float Average()
    {
        if(m_FrameRateCount > 0)
        {
            return ((float)m_RunningSum / (float)m_FrameRateCount);
        }
        else
        {
            return 0;
        }
    }


    //Register the message with the host
    HRESULT RegisterMessage()
    {
        HRESULT hr = S_OK;

        m_TestMessage = RegisterWindowMessage(c_FrameRateMessageName);
        CBR(0 != m_TestMessage);

        CHR(static_cast<HRESULT>(SendMessage(m_Hwnd, m_TestMessage, FRM_START, 0)));
        
       Error:
        return hr;
    }

    //Get the latest FPS and update the xaml text to display this value
    void UpdateFPS()
    {
        HRESULT hr = S_OK;
        
        UINT FPS;
        GetFrameRate(&FPS);

        // update statistics
        UpdateRunningData(FPS);

        // set text
        if (FPS != m_MostRecentFramerate)
        {
            WCHAR Text[MAX_PATH] = L"";
            StringCchPrintf(Text, MAX_PATH, L"%d FPS (%dx%d)", FPS, m_DisplayWidth, m_DisplayHeight);

            m_pTextBlock->SetText(Text);

            m_MostRecentFramerate = FPS;
        }
    }

    //Collect data that we'll use when the application exits
    void UpdateRunningData(UINT FPS)
    {  
        if(FPS > 0)
        {
            m_FrameRateCount++;
    
            if(FPS < m_MinFrameRate) // we initialized this to -1 which is HUGE as an unsigned
            {
                m_MinFrameRate = FPS;
            }
            
            if(FPS > m_MaxFrameRate)
            {
                m_MaxFrameRate = FPS;
            }
            
            m_RunningSum += FPS;
        }
    
    }

    //Add a click event to the main canvas in order to close the application
    void AttachMouseDownEvent(IXRPanelPtr pPanel)
    {
        pPanel->AddMouseLeftButtonDownEventHandler(CreateDelegate(this, &FrameRateManager::LeftButtonDown));
    }

    //on left button down print the stats and end the application
    HRESULT LeftButtonDown(__in IXRDependencyObject* pDO, __in XRMouseButtonEventArgs *pArgs)
    {
        PrintStats();
        m_pHost->EndDialog(0);
        
        if(g_bRotation)
        {
            DEVMODE DevMode;
            memset(&DevMode, 0, sizeof (DevMode));
            DevMode.dmSize               = sizeof (DevMode);
            DevMode.dmFields             = DM_DISPLAYORIENTATION;
            DevMode.dmDisplayOrientation = DMDO_0;
            
            if (ChangeDisplaySettingsEx(NULL, &DevMode, NULL, CDS_RESET, NULL) != DISP_CHANGE_SUCCESSFUL)
            {
                RETAILMSG(1, (L"ChangeDisplaySettingsEx failed to change the rotation angle (error=%d).\r\n", GetLastError()));
            }
            
            g_bRotation = FALSE;
        }
        
        return S_OK;
    }

    // get the current frame rate
    HRESULT
    GetFrameRate(UINT * pFPS)
    {
        HRESULT hr = S_OK;
        DWORD ProcessId = NULL;
    
        HANDLE hRemoteProcess = NULL;
        VOID* pResultBuffer = NULL;
        float Result = 0.0;
                
        // Get a handle to the EXR process containing the target HWND
        GetWindowThreadProcessId(m_Hwnd, &ProcessId);
        hRemoteProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, ProcessId);
        
        // Allocate memory in the EXR process to marshal the fps counter over to this process
        pResultBuffer = VirtualAllocEx(hRemoteProcess, NULL, sizeof(FLOAT), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        CPR(pResultBuffer);
    
        hr = static_cast<HRESULT>(SendMessage(m_Hwnd, m_TestMessage, FRM_GET_FRAMES_PER_SECOND, (LPARAM)pResultBuffer));
        CHR(hr);
    
        // Read back EXR's response
        CBREx(ReadProcessMemory(hRemoteProcess, pResultBuffer, (LPVOID)(&Result), sizeof(FLOAT), NULL), GetLastErrorHR());
    
        if (pFPS)
        {
            *pFPS = (UINT)Result;
        }
        
    Error:
        if (pResultBuffer)
        {
            VirtualFreeEx(hRemoteProcess, pResultBuffer, 0, MEM_RELEASE);
        }
        return hr;
    }

    //add the statistics xaml to the user xaml
    HRESULT AddStatisticsGridToHost(IXRApplicationPtr pApplication)
    {
    
        HRESULT hr = S_OK;
    
        IXRUIElementCollectionPtr pUserXamlChildren;
        IXRGridPtr  pGrid;
        IXRDependencyObjectPtr pDO;
        IXRPanelPtr pPanel;
        IXRFrameworkElementPtr pRoot;
        IXRUserControlPtr pUserControl;
    
        UINT panelWidth                         = 0;
        UINT panelHeight                        = 0;
        XRXamlSource XamlSource;
    
        //our FPS xaml box
        static const WCHAR statsXaml[] = 
            L"<Grid " \
            L"xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\" " \
            L"xmlns:x=\"http://schemas.microsoft.com/winfx/2006/xaml\" " \
            L"x:Name=\"StatsRoot\" VerticalAlignment=\"Top\" HorizontalAlignment=\"Left\" CacheMode=\"BitmapCache\">" \
            L"<TextBlock Name=\"FPSText\" Margin=\"60.595,28,57.405,7\" Foreground=\"#FFFFFFFF\" FontSize=\"18\" HorizontalAlignment=\"Left\" VerticalAlignment=\"Top\" Text=\" \"/>" \
            L"<TextBlock Name=\"CPUText\" Margin=\"60.595,28,57.405,7\" Foreground=\"#FFFFFFFF\" FontSize=\"18\" HorizontalAlignment=\"Right\" VerticalAlignment=\"Top\" Text=\" \"/>" \
            L"</Grid>";        

        CHR(m_pHost->GetRootElement(&pRoot));
        CHR(m_pHost->GetSize(&panelWidth, &panelHeight));
    
        //there are two possible combinations of top-most elements, UserControl and Elements that derive from Panel
        //first we'll try usercontrol
        pUserControl = pRoot; //QI for user control
    
        if(pUserControl)
        {
            pUserControl->GetContent(&pPanel); //lets get the content out which should have a base class as panel
        }
        else
        {
            pPanel = pRoot; //QI for the panel, which should be the base class of the root element
        }
    
        if(pPanel)
        {
    
            //add mouse down event handler to close the app
            AttachMouseDownEvent(pPanel);
    
            XamlSource.SetString(statsXaml);
            CHR(pApplication->ParseXaml(&XamlSource, &pDO)); //parses into a DO
    
            pGrid = pDO; //QI the dependency object for the grid
    
            if(pGrid)
            {            
                //add new elements to display the data
                CHR(pPanel->GetChildren(&pUserXamlChildren));    
                CHR(pUserXamlChildren->Add(pGrid, NULL));    

                pGrid->SetWidth((float)panelWidth);
                pGrid->SetHeight((float)panelHeight);
                
                CHR(pGrid->FindName(L"FPSText", &m_pTextBlock)); 
                CHR(pGrid->FindName(L"CPUText", &m_pCpuTextBlock));
            }            
        }

        Error:
            return hr;
    }

    void PrintStats()
    {
        WCHAR buffer[MAX_PATH] = {0};
        UINT memUsage = (g_InitialMemUsage - GetMemoryUsage()) / 1024; // in KB
        float cpuUsage = GetCurrentCPUUtilization(); 

        RETAILMSG(1, (L"------------------------------------------------------"));
        RETAILMSG(1, (L"[SAMPLE XAML PERF]: Summary:"));
        RETAILMSG(1, (L"[SAMPLE XAML PERF]: Max Frame Per Second: %d", m_MaxFrameRate));
        RETAILMSG(1, (L"[SAMPLE XAML PERF]: Min Frame Per Second: %d", m_MinFrameRate));

        StringCchPrintf(buffer, MAX_PATH, L"[SAMPLE XAML PERF]: Average Frame Per Second: %.02f", Average());
        RETAILMSG(1, (buffer));
        StringCchPrintf(buffer, MAX_PATH, L"[SAMPLE XAML PERF]: Application Memory Use: %d KB", memUsage);
        RETAILMSG(1, (buffer));
        StringCchPrintf(buffer, MAX_PATH, L"[SAMPLE XAML PERF]: CPU Usage: %.02f%s", cpuUsage, L"%%");
        RETAILMSG(1, (buffer));
        RETAILMSG(1, (L"------------------------------------------------------"));
    }
    void EmitMemUsageData()
    {
        UINT memUsage = (g_InitialMemUsage - GetMemoryUsage()) / 1024; // in KB
        float cpuUsage = GetCurrentCPUUtilization();        
        
        WCHAR buffer[MAX_PATH] = {0};

        //StringCchPrintf(buffer, MAX_PATH, L"[SAMPLE XAML PERF]: Application Memory Use: %d KB", memUsage);
        //RETAILMSG(1, (buffer));

        //StringCchPrintf(buffer, MAX_PATH, L"[SAMPLE XAML PERF]: CPU Usage: %.02f%s", cpuUsage, L"%%");
        //RETAILMSG(1, (buffer));
        StringCchPrintf(buffer, MAX_PATH, L"%.02f %% of CPU", cpuUsage);
        m_pCpuTextBlock->SetText(buffer);
    }

public:
    UINT m_MinFrameRate;
    UINT m_MaxFrameRate;
    UINT m_AvgFrameRate;
    UINT m_FrameRateCount;
    UINT m_RunningSum;
    UINT m_TestMessage;
    UINT m_MostRecentFramerate;
    UINT m_DisplayWidth;
    UINT m_DisplayHeight;

    HWND m_Hwnd;
    
    IXRVisualHostPtr m_pHost;
    IXRTextBlockPtr m_pTextBlock;
    IXRTextBlockPtr m_pCpuTextBlock;
};

FrameRateManager g_FRM;

void CALLBACK TimerProc(HWND hwnd, UINT, UINT, DWORD dwTime)
{
    g_FRM.UpdateFPS();
    g_FRM.EmitMemUsageData();
}

void CALLBACK EmitMemUsageData(HWND hwnd, UINT, UINT, DWORD dwTime)
{
    g_FRM.EmitMemUsageData();
}

void
Usage()
{
    RETAILMSG(1, (L"---------------------------------------------------"));
    RETAILMSG(1, (L"--- XamlPerf [filename] ---------------------------"));
    RETAILMSG(1, (L"------ Click anywhere on the application to exit --"));
    RETAILMSG(1, (L"---------------------------------------------------"));
}

//Read in the command line parameter, load the xaml and start the application
HRESULT Run(LPWSTR lpCmdLine)
{
    HRESULT hr                              = S_OK; 
    HKEY	hKey;

    XRXamlSource XamlSource;
    XRWindowCreateParams WindowParameters   = {0};
    
    size_t CmdLength                        = 0;
    UINT Timer                              = 0;

    IXRApplicationPtr pApplication;
    IXRVisualHostPtr pHost;
    
    UINT Height                             = GetSystemMetrics(SM_CYSCREEN);
    UINT Width                              = GetSystemMetrics(SM_CXSCREEN);

    ShowCursor(false);
    // Get the Alchemy factory
    //
    CHR(GetXRApplicationInstance(&pApplication));

    StringCchLength(lpCmdLine, MAX_PATH, &CmdLength);

    if(0 == CmdLength)
    {
        //we have to get a filename from the cmd line
        hr = XR_E_INVALID_STATE;
        goto Error;
    }
    
    // load our specified Xaml
    //
    ZeroMemory(&XamlSource, sizeof(XamlSource));
    XamlSource.SetFile(lpCmdLine);

    // setup the default window (size is the full screen)
    //
    ZeroMemory(&WindowParameters, sizeof(WindowParameters));
    WindowParameters.pTitle      = L"XAML App (TI Modified)";
    WindowParameters.Style       = WS_POPUP;

    // check for VRFB 
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, RK_VRFB, 0, KEY_ALL_ACCESS, &hKey))
	{
	    RegCloseKey(hKey);
		g_VRFB = TRUE;
	}
	
    WindowParameters.Height      = Height;
    WindowParameters.Width       = Width;
    g_FRM.m_DisplayWidth = Width;
    g_FRM.m_DisplayHeight = Height;
    
    // check if rotation needed and supported
    //
    if(Height > Width && g_VRFB)
    {        
        DEVMODE DevMode;
        memset(&DevMode, 0, sizeof (DevMode));
        DevMode.dmSize               = sizeof (DevMode);
        DevMode.dmFields             = DM_DISPLAYORIENTATION;
        
        if (DISP_CHANGE_SUCCESSFUL != ChangeDisplaySettingsEx(NULL, &DevMode, NULL, CDS_TEST, NULL))
        {
            RETAILMSG(1, (L"ChangeDisplaySettingsEx failed to get the supported rotation angles (error=%d).\n", GetLastError()));
        }
        else
        {
            if(DevMode.dmDisplayOrientation == 0)
            {    
                DevMode.dmDisplayOrientation = DMDO_90;
                if (ChangeDisplaySettingsEx(NULL, &DevMode, NULL, CDS_RESET, NULL) != DISP_CHANGE_SUCCESSFUL)
                {
                    RETAILMSG(1, (L"ChangeDisplaySettingsEx failed to change the rotation angle (error=%d).\r\n", GetLastError()));
                }
                else
                {
                    g_bRotation = TRUE;
                    WindowParameters.Height      = Width;
                    WindowParameters.Width       = Height;
                    g_FRM.m_DisplayWidth = Height;
                    g_FRM.m_DisplayHeight = Width;
                }
            }
        }
    }
    
    
    // create a host for this XAML scene
    //
    CHR(pApplication->CreateHostFromXaml(&XamlSource, &WindowParameters, &pHost));
    CHR(g_FRM.Init(pApplication, pHost));

    if(SUCCEEDED(hr))
    {
        Timer = SetTimer(0, 0, 1000, TimerProc);
        pHost->StartDialog(NULL);
    }
        
    // success
    hr = S_OK;

Error:

    if(FAILED(hr))
    {
        Usage();
        if(g_bRotation)
        {
            DEVMODE DevMode;
            memset(&DevMode, 0, sizeof (DevMode));
            DevMode.dmSize               = sizeof (DevMode);
            DevMode.dmFields             = DM_DISPLAYORIENTATION;
            DevMode.dmDisplayOrientation = DMDO_0;
            
            if (ChangeDisplaySettingsEx(NULL, &DevMode, NULL, CDS_RESET, NULL) != DISP_CHANGE_SUCCESSFUL)
            {
                RETAILMSG(1, (L"ChangeDisplaySettingsEx failed to change the rotation angle (error=%d).\r\n", GetLastError()));
            }
            
            g_bRotation = FALSE;
        }
    }

    g_FRM.Cleanup();
    if (pHost)
    {
        pHost->DestroyWindow();
        pHost = NULL;        
    }
    ShowCursor(true);

    //kill the timer and exit the application        
    KillTimer(NULL, Timer);

return hr;

}


//
// entry point for the application.
INT WINAPI WinMain(
    HINSTANCE hInstance, 
    HINSTANCE hPrevInstance, 
    LPWSTR lpCmdLine, 
    int nCmdShow
    )
{
    int             exitCode = -1;
    HRESULT         hr = S_OK;

    XamlRuntimeManager xr;

    if(xr.IsInitialized())
    {
        if(SUCCEEDED(Run(lpCmdLine)))
        {
            exitCode = 0;
        }
    }
    
    return exitCode;
}
