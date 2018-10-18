// we need commctrl v6 for LoadIconMetric()
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib, "comctl32.lib")

#include "resource.h"
#include <windows.h>
#include <shellapi.h>
#include <commctrl.h>
#include <strsafe.h>

HINSTANCE g_hInst = NULL;
HWND hTrackbar = NULL;
HBRUSH hbrBkgnd = NULL;
BYTE state;

UINT const WMAPP_NOTIFYCALLBACK = WM_APP + 1;
UINT const WMAPP_HIDEFLYOUT     = WM_APP + 2;

UINT_PTR const HIDEFLYOUT_TIMER_ID = 1;

wchar_t const szWindowClass[] = L"NotificationIconTest";
wchar_t const szFlyoutWindowClass[] = L"NotificationFlyout";
WCHAR regPath[530];
WCHAR keyName[265];

// Use a guid to uniquely identify our icon
class __declspec(uuid("8464caa8-4682-4c11-bf4d-f5d3ca74cf8b")) NightLightIcon;

// Forward declarations of functions included in this code module:
void                RegisterWindowClass();
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
void				RegisterFlyoutWindowClass();
LRESULT CALLBACK    FlyoutWndProc(HWND, UINT, WPARAM, LPARAM);
HWND                ShowFlyout(HWND hwnd);
void                HideFlyout(HWND hwndMainWindow, HWND hwndFlyout);
void                PositionFlyout(HWND hwnd, REFGUID guidIcon);
void                ShowContextMenu(HWND hwnd, POINT pt);
BOOL                AddNotificationIcon(HWND hwnd);
BOOL                DeleteNotificationIcon();
void				SetRegValue(BYTE level);
BOOL				LoadState();

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR /*lpCmdLine*/, int nCmdShow)
{
    g_hInst = hInstance;
    RegisterWindowClass();
    RegisterFlyoutWindowClass();
	LoadState();

    // Create the main window. This could be a hidden window if you don't need
    // any UI other than the notification icon.
    WCHAR szTitle[100];
    LoadString(hInstance, IDS_APP_TITLE, szTitle, ARRAYSIZE(szTitle));
    HWND hwnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, 250, 200, NULL, NULL, g_hInst, NULL);
    
	if (hwnd)
    {
		UNREFERENCED_PARAMETER(nCmdShow);
        //ShowWindow(hwnd, nCmdShow);

        // Main message loop:
        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return 0;
}

void RegisterWindowClass()
{
    WNDCLASSEX wcex = {sizeof(wcex)};
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.hInstance      = g_hInst;
    wcex.hIcon          = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_NOTIFICATIONICON));
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = NULL;
    wcex.lpszClassName  = szWindowClass;
    RegisterClassEx(&wcex);
}

void RegisterFlyoutWindowClass()
{
	WNDCLASSEX wcex = { sizeof(wcex) };
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = FlyoutWndProc;
	wcex.hInstance = g_hInst;
	wcex.hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_NOTIFICATIONICON));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	//wcex.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));//(HBRUSH)(BLACK_BRUSH);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = szFlyoutWindowClass;
	RegisterClassEx(&wcex);
}

BOOL AddNotificationIcon(HWND hwnd)
{
    NOTIFYICONDATA nid = {sizeof(nid)};
    nid.hWnd = hwnd;
    // add the icon, setting the icon, tooltip, and callback message.
    // the icon will be identified with the GUID
    nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE | NIF_SHOWTIP | NIF_GUID;
    nid.guidItem = __uuidof(NightLightIcon);
    nid.uCallbackMessage = WMAPP_NOTIFYCALLBACK;
    LoadIconMetric(g_hInst, MAKEINTRESOURCE(IDI_NOTIFICATIONICON), LIM_SMALL, &nid.hIcon);
    LoadString(g_hInst, IDS_TOOLTIP, nid.szTip, ARRAYSIZE(nid.szTip));
    bool success = Shell_NotifyIcon(NIM_ADD, &nid);
	if (!success) return false;

    // NOTIFYICON_VERSION_4 is prefered
    nid.uVersion = NOTIFYICON_VERSION_4;
    return Shell_NotifyIcon(NIM_SETVERSION, &nid);
}

BOOL DeleteNotificationIcon()
{
    NOTIFYICONDATA nid = {sizeof(nid)};
    nid.uFlags = NIF_GUID;
    nid.guidItem = __uuidof(NightLightIcon);
    return Shell_NotifyIcon(NIM_DELETE, &nid);
}

void PositionFlyout(HWND hwnd, REFGUID guidIcon)
{
    // find the position of our printer icon
    NOTIFYICONIDENTIFIER nii = {sizeof(nii)};
    nii.guidItem = guidIcon;
    RECT rcIcon;
    HRESULT hr = Shell_NotifyIconGetRect(&nii, &rcIcon);
    if (SUCCEEDED(hr))
    {
        // display the flyout in an appropriate position close to the printer icon
        POINT const ptAnchor = { (rcIcon.left + rcIcon.right) / 2, (rcIcon.top + rcIcon.bottom)/2 };

        RECT rcWindow;
        GetWindowRect(hwnd, &rcWindow);
        SIZE sizeWindow = {rcWindow.right - rcWindow.left, rcWindow.bottom - rcWindow.top};

        if (CalculatePopupWindowPosition(&ptAnchor, &sizeWindow, TPM_VERTICAL | TPM_VCENTERALIGN | TPM_CENTERALIGN | TPM_WORKAREA, &rcIcon, &rcWindow))
        {
            // position the flyout and make it the foreground window
            SetWindowPos(hwnd, HWND_TOPMOST, rcWindow.left, rcWindow.top, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
        }
    }
}

HWND ShowFlyout(HWND hwndMainWindow)
{
    // size of the bitmap image (which will be the client area of the flyout window).
    RECT rcWindow = {};
    rcWindow.right = 200;
    rcWindow.bottom = 30;
	DWORD const dwStyle = WS_POPUP | WS_OVERLAPPED;// WS_THICKFRAME;
    // adjust the window size to take the frame into account
    AdjustWindowRectEx(&rcWindow, dwStyle, FALSE, WS_EX_TOOLWINDOW);

    HWND hwndFlyout = CreateWindowEx(WS_EX_TOOLWINDOW, szFlyoutWindowClass, NULL, dwStyle,
        CW_USEDEFAULT, 0, rcWindow.right - rcWindow.left, rcWindow.bottom - rcWindow.top, hwndMainWindow, NULL, g_hInst, NULL);

	hTrackbar = CreateWindowEx(0, TRACKBAR_CLASS, L"Trackbar Control",
		WS_CHILD | WS_VISIBLE | TBS_HORZ,
		0, 0, rcWindow.right - rcWindow.left, rcWindow.bottom - rcWindow.top, hwndFlyout, NULL, g_hInst, NULL);

	SendMessage(hTrackbar, TBM_SETRANGE,
		(WPARAM)TRUE, (LPARAM)MAKELONG(18, 100));  // min. & max. positions*/

	SendMessage(hTrackbar, TBM_SETPOS,
		(WPARAM)TRUE, (LPARAM)state);  // current position

    if (hwndFlyout)
    {
        PositionFlyout(hwndFlyout, __uuidof(NightLightIcon));
        SetForegroundWindow(hwndFlyout);
    }
    return hwndFlyout;
}

void HideFlyout(HWND hwndMainWindow, HWND hwndFlyout)
{
    DestroyWindow(hwndFlyout);

    // immediately after hiding the flyout we don't want to allow showing it again, which will allow clicking
    // on the icon to hide the flyout. If we didn't have this code, clicking on the icon when the flyout is open
    // would cause the focus change (from flyout to the taskbar), which would trigger hiding the flyout
    // (see the WM_ACTIVATE handler). Since the flyout would then be hidden on click, it would be shown again instead
    // of hiding.
    SetTimer(hwndMainWindow, HIDEFLYOUT_TIMER_ID, GetDoubleClickTime(), NULL);
}

void ShowContextMenu(HWND hwnd, POINT pt)
{
    HMENU hMenu = LoadMenu(g_hInst, MAKEINTRESOURCE(IDC_CONTEXTMENU));
    if (hMenu)
    {
        HMENU hSubMenu = GetSubMenu(hMenu, 0);
        if (hSubMenu)
        {
            // our window must be foreground before calling TrackPopupMenu or the menu will not disappear when the user clicks away
            SetForegroundWindow(hwnd);

            // respect menu drop alignment
            UINT uFlags = TPM_RIGHTBUTTON;
            if (GetSystemMetrics(SM_MENUDROPALIGNMENT) != 0)
            {
                uFlags |= TPM_RIGHTALIGN;
            }
            else
            {
                uFlags |= TPM_LEFTALIGN;
            }

            TrackPopupMenuEx(hSubMenu, uFlags, pt.x, pt.y, hwnd, NULL);
        }
        DestroyMenu(hMenu);
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HWND s_hwndFlyout = NULL;
    static BOOL s_fCanShowFlyout = TRUE;

    switch (message)
    {
    case WM_CREATE:
        // add the notification icon
        if (!AddNotificationIcon(hwnd))
        {
            MessageBox(hwnd,
                L"Please read the ReadMe.txt file for troubleshooting",
                L"Error adding icon", MB_OK);
            return -1;
        }
        break;
    case WM_COMMAND:
        {
            int const wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_OPTIONS:
                // placeholder for an options dialog
                MessageBox(hwnd,  L"Display the options dialog here.", L"Options", MB_OK);
                break;

            case IDM_EXIT:
                DestroyWindow(hwnd);
                break;

            case IDM_FLYOUT:
                s_hwndFlyout = ShowFlyout(hwnd);
                break;

            default:
                return DefWindowProc(hwnd, message, wParam, lParam);
            }
        }
        break;

    case WMAPP_NOTIFYCALLBACK:
        switch (LOWORD(lParam))
        {
        case NIN_SELECT:
            // for NOTIFYICON_VERSION_4 clients, NIN_SELECT is prerable to listening to mouse clicks and key presses
            // directly.
            if (IsWindowVisible(s_hwndFlyout))
            {
                HideFlyout(hwnd, s_hwndFlyout);
                s_hwndFlyout = NULL;
                s_fCanShowFlyout = FALSE;
            }
            else if (s_fCanShowFlyout)
            {
                s_hwndFlyout = ShowFlyout(hwnd);
            }
            break;

        case WM_CONTEXTMENU:
            {
                POINT const pt = { LOWORD(wParam), HIWORD(wParam) };
                ShowContextMenu(hwnd, pt);
            }
            break;
        }
        break;

    case WMAPP_HIDEFLYOUT:
        HideFlyout(hwnd, s_hwndFlyout);
        s_hwndFlyout = NULL;
        s_fCanShowFlyout = FALSE;
        break;

    case WM_TIMER:
        if (wParam == HIDEFLYOUT_TIMER_ID)
        {
            // please see the comment in HideFlyout() for an explanation of this code.
            KillTimer(hwnd, HIDEFLYOUT_TIMER_ID);
            s_fCanShowFlyout = TRUE;
        }
        break;
    case WM_DESTROY:
        DeleteNotificationIcon();
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}

/*void FlyoutPaint(HWND hwnd, HDC hdc)
{
    // Since this is a DPI aware application (see DeclareDPIAware.manifest), if the flyout window
    // were to show text we would need to increase the size. We could also have multiple sizes of
    // the bitmap image and show the appropriate image for each DPI, but that would complicate the
    // sample.
    static HBITMAP hbmp = NULL;
    if (hbmp == NULL)
    {
        hbmp = (HBITMAP)LoadImage(g_hInst, MAKEINTRESOURCE(IDB_PRINTER), IMAGE_BITMAP, 0, 0, 0);
    }
    if (hbmp)
    {
        RECT rcClient;
        GetClientRect(hwnd, &rcClient);
        HDC hdcMem = CreateCompatibleDC(hdc);
        if (hdcMem)
        {
            HGDIOBJ hBmpOld = SelectObject(hdcMem, hbmp);
            BitBlt(hdc, 0, 0, rcClient.right, rcClient.bottom, hdcMem, 0, 0, SRCCOPY);
            SelectObject(hdcMem, hBmpOld);
            DeleteDC(hdcMem);
        }
    }
}*/
LRESULT CALLBACK FlyoutWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);
            //FlyoutPaint(hwnd, hdc);
            EndPaint(hwnd, &ps);
        }
        break;
    case WM_ACTIVATE:
        if (LOWORD(wParam) == WA_INACTIVE)
        {
            // when the flyout window loses focus, hide it.
            PostMessage(GetParent(hwnd), WMAPP_HIDEFLYOUT, 0, 0);
        }
        break;
	case WM_HSCROLL:
		if (hTrackbar != NULL) {
			auto dwPos = SendMessage(hTrackbar, TBM_GETPOS, 0, 0);
			SetRegValue((BYTE)dwPos);
		}
		break;
	case WM_CTLCOLORSTATIC:
			/*
		if (hwnd == hTrackbar)
		{
			HDC hdcStatic = (HDC)wParam;
			SetTextColor(hdcStatic, RGB(255, 0, 0));
			SetBkColor(hdcStatic, RGB(0, 255, 0));

			if (hbrBkgnd == NULL)
			{
				hbrBkgnd = CreateSolidBrush(RGB(0, 0, 0));
			}
			return (INT_PTR)hbrBkgnd;
			return (LRESULT)GetStockObject(BLACK_BRUSH);
		}
		break;
			*/
    default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}

void SetRegValue(BYTE level)
{
	HKEY key;
	auto status = RegOpenKeyEx(HKEY_CURRENT_USER, regPath, 0, KEY_QUERY_VALUE | KEY_SET_VALUE, &key);
	if (status != ERROR_SUCCESS) {return;}

	DWORD size;
	DWORD dataType;
	status = RegQueryValueEx(key, keyName, 0, &dataType, NULL, &size);
	if (status != ERROR_SUCCESS) { return; }

	LPBYTE allocated = (LPBYTE)HeapAlloc(GetProcessHeap(), 0, size);
	status = RegQueryValueEx(key, keyName, 0, &dataType, allocated, &size);
	if (status != ERROR_SUCCESS)
	{
		HeapFree(GetProcessHeap(), 0, allocated);
		return;
	}

	((BYTE*)allocated)[size - 16] = level;
	status = RegSetValueEx(key, keyName, 0,	dataType, (const BYTE*)allocated, size);
	if (status != ERROR_SUCCESS)
	{
		HeapFree(GetProcessHeap(), 0, allocated);
		return;
	}

	status = RegCloseKey(key);
	HeapFree(GetProcessHeap(), 0, allocated);
	state = level;
}

BOOL LoadState()
{
	LoadString(g_hInst, IDS_REG_SETTING_PATH, regPath, ARRAYSIZE(regPath));

	HKEY key;
	auto status = RegOpenKeyEx(HKEY_CURRENT_USER, regPath, 0, KEY_QUERY_VALUE | KEY_SET_VALUE, &key);
	if (status != ERROR_SUCCESS) { return false; }

	LoadString(g_hInst, IDS_REG_KEY_NAME, keyName, ARRAYSIZE(keyName));

	DWORD size;
	DWORD dataType;
	status = RegQueryValueEx(key, keyName, 0, &dataType, NULL, &size);
	if (status != ERROR_SUCCESS) { return false; }

	LPBYTE allocated = (LPBYTE)HeapAlloc(GetProcessHeap(), 0, size);
	status = RegQueryValueEx(key, keyName, 0, &dataType, allocated, &size);
	if (status != ERROR_SUCCESS)
	{
		HeapFree(GetProcessHeap(), 0, allocated);
		return false;
	}

	state = ((BYTE*)allocated)[size - 16];
	HeapFree(GetProcessHeap(), 0, allocated);
	return true;
}