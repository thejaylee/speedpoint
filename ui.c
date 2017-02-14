#include "ui.h"
#include "device.h"
#include "persist.h"
#include "resource.h"

#define TRAY_UID 0x8cec64ba
#define SWM_TRAY WM_APP + 0x00

#define WINDOW_WIDTH 450
#define PANE_HEIGHT 55

#define WINDOW_COLOR   RGB(240,240,240)
#define INACTIVE_COLOR RGB(245,230,230)
#define ACTIVE_COLOR   RGB(230,245,230)

typedef struct {
	device_info_t *devinfo;
	TCHAR  nice_name[NAME_MAX_LENGTH];
	struct {
		HWND pane;
		WNDPROC oldproc;
		struct {
			HWND device;
			HWND speed;
			HWND accel;
		} label;
		struct {
			HWND speed;
			HWND accel[3];
		} edit;
		struct {
			HWND set;
		} button;
	} ui;
} _device_pane_t;

static struct {
	HBRUSH window;
	HBRUSH active;
	HBRUSH inactive;
} _fills;

static _device_pane_t _devpanes[MAX_DEVICES] = { 0 };
static _device_pane_t *_active_pane = NULL;
static UINT _num_devices = 0;
static HWND _window;
static NOTIFYICONDATA _tray;

/*************
* PROTOTYPES *
**************/
static LRESULT CALLBACK _WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK _paneWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

/**********
* STATIC *
**********/

static _device_pane_t *_devPaneByBtnHwnd(HWND hWnd) {
	for (UINT c = 0; c<_num_devices; c++) {
		if (hWnd == _devpanes[c].ui.button.set)
			return &_devpanes[c];
	}
	return NULL;
}

static _device_pane_t *_devPaneByHwnd(HWND hWnd) {
	for (UINT c = 0; c < _num_devices; c++) {
		if (_devpanes[c].ui.pane == hWnd)
			return &_devpanes[c];
	}

	return NULL;
}

static _device_pane_t *_devPaneByDevInfo(device_info_t *devinfo) {
	for (UINT c = 0; c < _num_devices; c++) {
		if (_devpanes[c].devinfo == devinfo)
			return &_devpanes[c];
	}

	return NULL;
}

static _device_pane_t* _addDevicePane(device_info_t *devinfo) {
	TCHAR *end;
	_device_pane_t *devpane = &_devpanes[_num_devices++];
	devpane->devinfo = devinfo;
	// set the (display) device name to only the relevant bits
	for (end = devinfo->name; *end != L'{' && end < devinfo->name+ sizeof(devinfo->name); end++);
	_tcsncpy_s(devpane->nice_name, _countof(devpane->nice_name), &devinfo->name[4], (end - &devinfo->name[4]));
	return devpane;
}

/**
* @returns FALSE is device is already active device. TRUE if set as new active device
*/
static BOOL _setActivePane(_device_pane_t *devpane) {
	if (_active_pane == devpane)
		return FALSE;
	
	_active_pane = devpane;
	for (UINT c = 0; c < _num_devices; c++) {
		if (_devpanes[c].ui.pane) {
			InvalidateRect(_devpanes[c].ui.pane, NULL, TRUE);
			UpdateWindow(_devpanes[c].ui.pane);
		}
	}
	dprintf(L"active pane: (%x) %s\n", (UINT)devpane->ui.pane, devpane->nice_name);
	return TRUE;
}

static void _createDevicePane(_device_pane_t *devpane) {
#define ROFF ((_num_devices - 1) * PANE_HEIGHT) // row offset
	HINSTANCE hInstance = (HINSTANCE)GetWindowLong(_window, GWL_HINSTANCE);
	TCHAR buf[16];

	// border
	/*WNDCLASS wc;
	wc.lpszClassName = L"STATIC";
	wc.hInstance = hInstance;
	wc.lpfnWndProc = _paneWndProc;
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	RegisterClass(&wc);*/
	devpane->ui.pane = CreateWindow(L"STATIC", L"", WS_VISIBLE | WS_CHILD, 5, ROFF + 5, WINDOW_WIDTH - 22, PANE_HEIGHT, _window, NULL, hInstance, NULL);
	devpane->ui.oldproc = (WNDPROC)SetWindowLongPtr(devpane->ui.pane, GWLP_WNDPROC, (LONG)_paneWndProc);
	dprintf(L"devpane pane: %x\n", (UINT)devpane->ui.pane);
	// labels
	devpane->ui.label.device = CreateWindow(L"STATIC", devpane->nice_name, WS_VISIBLE | WS_CHILD | SS_NOPREFIX | SS_LEFTNOWORDWRAP, 5, 5, WINDOW_WIDTH - 25, 18, devpane->ui.pane, NULL, hInstance, NULL);
	devpane->ui.label.speed = CreateWindow(L"STATIC", L"speed", WS_VISIBLE | WS_CHILD | SS_NOPREFIX | SS_LEFTNOWORDWRAP, 15, 28, 40, 18, devpane->ui.pane, NULL, hInstance, NULL);
	devpane->ui.label.accel = CreateWindow(L"STATIC", L"accel", WS_VISIBLE | WS_CHILD | SS_NOPREFIX | SS_LEFTNOWORDWRAP, 100, 28, 35, 18, devpane->ui.pane, NULL, hInstance, NULL);
	// edits
	_stprintf_s(buf, _countof(buf), L"%d", devpane->devinfo->speed);
	devpane->ui.edit.speed = CreateWindow(L"EDIT", buf, WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_BORDER | ES_NUMBER | ES_CENTER, 60, 27, 25, 20, devpane->ui.pane, NULL, hInstance, NULL);
	_stprintf_s(buf, _countof(buf), L"%d", devpane->devinfo->accel[0]);
	devpane->ui.edit.accel[0] = CreateWindow(L"EDIT", buf, WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_BORDER | ES_NUMBER | ES_CENTER, 140, 27, 25, 20, devpane->ui.pane, NULL, hInstance, NULL);
	_stprintf_s(buf, _countof(buf), L"%d", devpane->devinfo->accel[1]);
	devpane->ui.edit.accel[1] = CreateWindow(L"EDIT", buf, WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_BORDER | ES_NUMBER | ES_CENTER, 170, 27, 25, 20, devpane->ui.pane, NULL, hInstance, NULL);
	_stprintf_s(buf, _countof(buf), L"%d", devpane->devinfo->accel[2]);
	devpane->ui.edit.accel[2] = CreateWindow(L"EDIT", buf, WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_BORDER | ES_NUMBER | ES_CENTER, 200, 27, 25, 20, devpane->ui.pane, NULL, hInstance, NULL);
	// buttons
	devpane->ui.button.set = CreateWindow(L"BUTTON", L"Set", WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON | BS_FLAT, 325, 27, 40, 20, devpane->ui.pane, NULL, hInstance, NULL);

	RECT r;
	GetWindowRect(_window, &r);
	//dprintf(L"%d %d %d %d\n", r.left, r.right, r.top, r.bottom);
	MoveWindow(_window, r.left, r.top, WINDOW_WIDTH, r.bottom - r.top + PANE_HEIGHT, FALSE);
	InvalidateRect(_window, NULL, TRUE);
	UpdateWindow(_window);
	/*InvalidateRect(device->ui.pane, NULL, TRUE);
	UpdateWindow(device->ui.pane);*/
	
	dprintf(L"added device to ui: [%d] (%x) %s\n", _num_devices, (UINT)devpane->devinfo->hDevice, devpane->nice_name);
#undef VOFF
}

static void _updateSettings(_device_pane_t *devpane) {
	TCHAR buf[16];
	UINT speed, accel[3];

	GetWindowText(devpane->ui.edit.speed, buf, _countof(buf));
	speed = _tstoi(buf);
	GetWindowText(devpane->ui.edit.accel[0], buf, _countof(buf));
	accel[0] = _tstoi(buf);
	GetWindowText(devpane->ui.edit.accel[1], buf, _countof(buf));
	accel[1] = _tstoi(buf);
	GetWindowText(devpane->ui.edit.accel[2], buf, _countof(buf));
	accel[2] = _tstoi(buf);

	devSetMouseParams(devpane->devinfo, speed, accel);
	perSetMouseParams(devpane->devinfo->name, speed, accel);
}

static void _initTray(HINSTANCE hInst, HWND hWnd) {
	HICON icon;
	icon = (HICON) LoadImage(hInst, MAKEINTRESOURCE(IDI_TRAYICON), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);

	memset(&_tray, 0, sizeof(NOTIFYICONDATA));
	_tray.cbSize = NOTIFYICONDATA_V1_SIZE;
	_tray.hWnd = hWnd;
	_tray.uID = TRAY_UID;
	_tray.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	_tray.uCallbackMessage = SWM_TRAY;
	_tray.hIcon = icon;
	_tcscpy_s(_tray.szTip, sizeof(_tray.szTip), L"SpeedPoint");

	Shell_NotifyIcon(NIM_ADD, &_tray);
	DeleteObject(icon);
}

/***************
* WINDOW PROCS *
****************/
static LRESULT CALLBACK _WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	//dprintf(L"wndproc: (%x) %x %x %x\n", (UINT)hWnd, uMsg, wParam, lParam);

	switch (uMsg) {
	case WM_CREATE:
		CHECK_ERROR_EXIT(devRegisterMice(hWnd) == FALSE, -3, L"could not register raw input device");
		break;
	case WM_CTLCOLORSTATIC:
		//dprintf(L"WM_CTLCOLORSTATIC: (%x) %x %x %x\n", (UINT)hWnd, uMsg, wParam, lParam);
		_device_pane_t *devpane = _devPaneByHwnd((HWND)lParam);
		if (devpane) {
			if (devpane == _active_pane) {
				//dprintf(L"pane dev: %x\n", dev->nice_name);
				SetBkColor((HDC)wParam, ACTIVE_COLOR);
				return (INT_PTR)_fills.active;
			} else {
				SetBkColor((HDC)wParam, INACTIVE_COLOR);
				return (INT_PTR)_fills.inactive;
			}
		} else {
			SetBkColor((HDC)wParam, WINDOW_COLOR);
			return (INT_PTR)_fills.active;
		}
		//SetTextColor((HDC)wParam, RGB(0, 0, 0));
	case WM_INPUT:
		devProcessRawInput((HRAWINPUT)lParam);
		break;
	case SWM_TRAY:
		switch (lParam) {
		/*case WM_LBUTTONDOWN:
			break;
		case WM_LBUTTONDBLCLK:
			break;*/
		case WM_RBUTTONUP:
			uiTerminate();
			PostQuitMessage(0);
			break;
		}
		break;
	case WM_DESTROY:
		uiTerminate();
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	return 0;
}

static LRESULT CALLBACK _paneWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	//dprintf(L"paneproc: (%x) %x %x %x\n", (UINT)hWnd, uMsg, wParam, lParam);

	_device_pane_t *devpane = _devPaneByHwnd(hWnd);
	if (!devpane)
		return CallWindowProc(devpane->ui.oldproc, hWnd, uMsg, wParam, lParam);

	switch (uMsg) {
	case WM_COMMAND:
		//dprintf(L"sWM_COMMAND: (%x) %x %x %x\n", (UINT)hWnd, uMsg, wParam, lParam);
		if ((HWND)lParam == devpane->ui.button.set) {
			dprintf(L"saving %s\n", devpane->nice_name);
			_updateSettings(devpane);
		}
		break;
	case WM_CTLCOLORSTATIC:
		//dprintf(L"sWM_CTLCOLORSTATIC: (%x) %x %x %x\n", (UINT)hWnd, uMsg, wParam, lParam);
		if (devpane == _active_pane) {
			SetBkColor((HDC)wParam, ACTIVE_COLOR);
			return (INT_PTR)_fills.active;
		} else {
			SetBkColor((HDC)wParam, INACTIVE_COLOR);
			return (INT_PTR)_fills.inactive;
		}
	//case WM_CTLCOLOREDIT:
	//case WM_CTLCOLORBTN:
	//	dprintf(L"sWM_CTLCOLOR*: (%x) %x %x %x\n", (UINT)hWnd, uMsg, wParam, lParam);
	//	break;
	//case WM_PAINT:
	//	dprintf(L"sWM_PAINT: (%x) %x %x %x\n", (UINT)hWnd, uMsg, wParam, lParam);
	//	break;
	//case WM_ERASEBKGND:
	//	dprintf(L"sWM_ERASEBKGND: (%x) %x %x %x\n", (UINT)hWnd, uMsg, wParam, lParam);
	//	SetBkColor((HDC)wParam, ACTIVE_COLOR);
	//	return 0;
	}
	return CallWindowProc(devpane->ui.oldproc, hWnd, uMsg, wParam, lParam);
}

/**********
 * GLOBAL *
 **********/

HWND uiInit(HINSTANCE hInstance) {
	WNDCLASS wc = { 0 };

	_num_devices = 0;
	_active_pane = NULL;
	_fills.window = CreateSolidBrush(WINDOW_COLOR);
	_fills.active = CreateSolidBrush(ACTIVE_COLOR);
	_fills.inactive = CreateSolidBrush(INACTIVE_COLOR);

	wc.lpfnWndProc = _WndProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = UI_WINDOW_CLASS_NAME;
	wc.hbrBackground = (HBRUSH) COLOR_BACKGROUND;
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TRAYICON));
	CHECK_ERROR_EXIT(RegisterClass(&wc) == 0, -1, L"could not register class"); // now that the class is registered we create the window
	_window = CreateWindowEx(
		0,
		wc.lpszClassName,
		UI_WINDOW_NAME,
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		WINDOW_WIDTH,
		46,
		NULL,
		NULL,
		hInstance,
		NULL);
	CHECK_ERROR_EXIT(_window == NULL, -2, L"could not create window");

	_initTray(hInstance, _window);

	return _window;
}

void uiTerminate() {
	DestroyWindow(_window);
	Shell_NotifyIcon(NIM_DELETE, &_tray);
}

BOOL uiSetActive(device_info_t *devinfo) {
	if (!devinfo)
		return FALSE;

	_device_pane_t *devpane = _devPaneByDevInfo(devinfo);
	if (!devpane) {
		devpane = _addDevicePane(devinfo);
		_setActivePane(devpane);
		_createDevicePane(devpane);
	} else {
		_setActivePane(devpane);
	}

	return TRUE;
}