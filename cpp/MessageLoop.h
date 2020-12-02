#pragma once

int GetActiveDeviceCount();

PWSTR g_pszAppName;

HDEVNOTIFY  g_hdevnotify = NULL;

HMODULE hModuleG;
HWND hwndMain;

GUID WceusbshGUID = { 0x25dbce51, 0x6c8f, 0x4a72,
                      0x8a,0x6d,0xb5,0x4c,0x2b,0x4f,0xc8,0x35 }; // all USB devices


// forward decl.
HRESULT CheckVsAvailDevices(DEV_BROADCAST_HDR* pHdr);
boolean Uninitialise();
void JavaNotifyError(int errorCode, string msg);


BOOL DoRegisterDeviceInterfaceToHwnd(
    IN GUID InterfaceClassGuid,
    IN HWND hWnd,
    OUT HDEVNOTIFY* hDeviceNotify
)
{
    DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;

    ZeroMemory(&NotificationFilter, sizeof(NotificationFilter));
    NotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
    NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    NotificationFilter.dbcc_classguid = KSCATEGORY_CAPTURE; // for any/all USB devices use: 'InterfaceClassGuid;'

    *hDeviceNotify = RegisterDeviceNotification(
        hWnd,                       // events recipient
        &NotificationFilter,        // type of device
        DEVICE_NOTIFY_WINDOW_HANDLE // type of recipient handle
    );

    if (NULL == *hDeviceNotify)
    {
        JavaNotifyError(0, "Unable to register for device notification");
        return FALSE;
    }

    return TRUE;
}


LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

    LRESULT lRet = 1;
    static HDEVNOTIFY hDeviceNotify;
    static HWND hEditWnd;
    static ULONGLONG msgCount = 0;

    switch (message)
    {
    case WM_CREATE:
        //
        // This is the actual registration., In this example, registration 
        // should happen only once, at application startup when the window
        // is created.
        //
        // If you were using a service, you would put this in your main code 
        // path as part of your service initialization.
        //
        if (!DoRegisterDeviceInterfaceToHwnd(
            WceusbshGUID,
            hWnd,
            &hDeviceNotify))
        {
            // Terminate on failure.
            JavaNotifyError(0, "Unable to register for device notification (2)");
            ExitProcess(1);
        }

        break;

    case WM_DEVICECHANGE:
    {

        // This is the actual message from the interface via Windows messaging.
        // This code includes some additional decoding for this particular device type
        // and some common validation checks.
        //
        // Note that not all devices utilize these optional parameters in the same
        // way. Refer to the extended information for your particular device type 
        // specified by your GUID.

        PDEV_BROADCAST_DEVICEINTERFACE b = (PDEV_BROADCAST_DEVICEINTERFACE)lParam;

        switch (wParam)
        {
            case DBT_DEVICEARRIVAL:
            
                if (lParam != 0)
                {
                    CheckVsAvailDevices((PDEV_BROADCAST_HDR)lParam);
                }

                break;

            case DBT_DEVICEREMOVECOMPLETE:
            
                if (lParam != 0)
                {
                    CheckVsAvailDevices((PDEV_BROADCAST_HDR)lParam);
                }

                break;

            //case DBT_DEVNODES_CHANGED:
            //    notifyJava("device nodes changed");
            //    break;
            //default:
            //    notifyJava("Unknown device change");
            //    break;
        }

    }
    break;

    case WM_CLOSE:
        if (!UnregisterDeviceNotification(hDeviceNotify))
        {
            JavaNotifyError(0, "Unable to unregister for device notification");
        }
        DestroyWindow(hWnd);

        //Uninitialise();

        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        // Send all other messages on to the default windows handler.
        lRet = DefWindowProc(hWnd, message, wParam, lParam);
        break;
    }

    return lRet;
}

void MessagePump(HWND hWnd)
{

    MSG msg;
    int retVal;

    // Get all messages for any window that belongs to this thread,
    // without any filtering. Potential optimization could be
    // obtained via use of filter values if desired.

    while ((retVal = GetMessage(&msg, NULL, 0, 0)) != 0)
    {
        if (retVal == -1)
        {
            break;
        }
        else
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}

#define WND_CLASS_NAME TEXT("DeviceNotificationWindowClass")

boolean initMessageLoop()
{
    // Create the main window. 

    MSG Msg;
    WNDCLASSEX wc;

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = 0;
    wc.hInstance = hModuleG;
    wc.lpfnWndProc = reinterpret_cast<WNDPROC>(WindowProc);
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hIcon = NULL;
    wc.hbrBackground = NULL;
    wc.hCursor = NULL;
    wc.lpszClassName = WND_CLASS_NAME;
    wc.lpszMenuName = NULL;
    wc.hIconSm = wc.hIcon;

    if (!RegisterClassEx(&wc))
    {
        JavaNotifyError(0, "Unable to unregister for device notification (3)");
        return FALSE;
    }

    hwndMain = CreateWindowEx(
        WS_EX_CLIENTEDGE | WS_EX_APPWINDOW,
        WND_CLASS_NAME,
        L"DeviceNotificationWindow",
        WS_DISABLED,
        0, 0, // x,y
        0, 0, // w,h
        NULL,// HWND_MESSAGE, // for some reason tis HWND_MESSAGE option doesn't work
        NULL,
        hModuleG,
        NULL);

    if (!hwndMain) 
    {
        JavaNotifyError(0, "Unable to unregister for device notification (4)");
        return FALSE;
    }

    MessagePump(hwndMain);

    return TRUE;
}