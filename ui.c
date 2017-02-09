#include "ui.h"
#include "input.h"

#define WINDOW_WIDTH 400
#define PANE_HEIGHT 55

#define WINDOW_COLOR   RGB(240,240,240)
#define INACTIVE_COLOR RGB(245,230,230)
#define ACTIVE_COLOR   RGB(230,245,230)

typedef struct {
	BOOL isActive;
	HANDLE hDevice;
	TCHAR  name[256];
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
} _ui_device_t;

static struct {
	HBRUSH window;
	HBRUSH active;
	HBRUSH inactive;
} _fills;

static _ui_device_t _devices[MAX_DEVICES] = { 0 };
static _ui_device_t *_active_device = NULL;
static UINT _num_devices = 0;
static HWND _window;

/*************
* PROTOTYPES *
**************/
static LRESULT CALLBACK _WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK _paneWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

/**********
* STATIC *
**********/

static _ui_device_t* _getDeviceByBtnHwnd(HWND hWnd) {
	for (UINT c = 0; c<_num_devices; c++) {
		if (hWnd == _devices[c].ui.button.set)
			return &_devices[c];
	}
	return NULL;
}

static _ui_device_t* _getDeviceByPane(HWND hWnd) {
	for (UINT c = 0; c < _num_devices; c++) {
		if (_devices[c].ui.pane == hWnd)
			return &_devices[c];
	}

	return NULL;
}

static _ui_device_t* _getDeviceByHandle(HANDLE hDevice) {
	for (UINT c = 0; c < _num_devices; c++) {
		if (_devices[c].hDevice == hDevice)
			return &_devices[c];
	}

	return NULL;
}

static _ui_device_t* _addDevice(HANDLE hDevice) {
	TCHAR fullname[256], *end;
	_ui_device_t *dev = &_devices[_num_devices++];
	dev->hDevice = hDevice;
	// set the (display) device name to only the relevant bits
	inGetDeviceName(hDevice, fullname, _tsizeof(fullname));
	for (end = fullname; *end != L'{' && end < fullname + sizeof(fullname); end++);
	_tcsncpy_s(dev->name, _tsizeof(dev->name), &fullname[4], (end - &fullname[4]));
	return dev;
}

/**
* @returns FALSE is device is already active device. TRUE if set as new active device
*/
static BOOL _setActive(_ui_device_t *device) {
	if (_active_device == device)
		return FALSE;
	
	_active_device = device;
	for (UINT c = 0; c < _num_devices; c++) {
		if (_devices[c].ui.pane) {
			InvalidateRect(_devices[c].ui.pane, NULL, TRUE);
			UpdateWindow(_devices[c].ui.pane);
		}
	}
	dprintf(L"active device: %s\n", device->name);
	return TRUE;
}

static void _createDevicePane(_ui_device_t *device) {
#define ROFF ((_num_devices - 1) * PANE_HEIGHT) // row offset
	HINSTANCE hInstance = (HINSTANCE)GetWindowLong(_window, GWL_HINSTANCE);

	// border
	/*WNDCLASS wc;
	wc.lpszClassName = L"STATIC";
	wc.hInstance = hInstance;
	wc.lpfnWndProc = _paneWndProc;
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	RegisterClass(&wc);*/
	device->ui.pane = CreateWindow(L"STATIC", L"", WS_VISIBLE | WS_CHILD, 5, ROFF + 5, WINDOW_WIDTH - 22, PANE_HEIGHT, _window, NULL, hInstance, NULL);
	device->ui.oldproc = (WNDPROC)SetWindowLongPtr(device->ui.pane, GWLP_WNDPROC, (LONG)_paneWndProc);
	dprintf(L"device pane: %x\n", (UINT)device->ui.pane);
	/*dprintf(L"myproc %x\n", _paneWndProc);
	dprintf(L"oldproc %x\n", device->ui.oldproc);*/
	// labels
	device->ui.label.device = CreateWindow(L"STATIC", device->name, WS_VISIBLE | WS_CHILD | SS_NOPREFIX | SS_LEFTNOWORDWRAP, 5, 5, WINDOW_WIDTH - 25, 18, device->ui.pane, NULL, hInstance, NULL);
	device->ui.label.speed = CreateWindow(L"STATIC", L"speed", WS_VISIBLE | WS_CHILD | SS_NOPREFIX | SS_LEFTNOWORDWRAP, 15, 26, 40, 18, device->ui.pane, NULL, hInstance, NULL);
	device->ui.label.accel = CreateWindow(L"STATIC", L"accel", WS_VISIBLE | WS_CHILD | SS_NOPREFIX | SS_LEFTNOWORDWRAP, 100, 26, 35, 18, device->ui.pane, NULL, hInstance, NULL);
	// edits
	device->ui.edit.speed = CreateWindow(L"EDIT", L"6", WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_BORDER | ES_NUMBER | ES_CENTER, 60, 25, 25, 20, device->ui.pane, NULL, hInstance, NULL);
	device->ui.edit.accel[0] = CreateWindow(L"EDIT", L"4", WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_BORDER | ES_NUMBER | ES_CENTER, 140, 25, 25, 20, device->ui.pane, NULL, hInstance, NULL);
	device->ui.edit.accel[1] = CreateWindow(L"EDIT", L"10", WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_BORDER | ES_NUMBER | ES_CENTER, 170, 25, 25, 20, device->ui.pane, NULL, hInstance, NULL);
	device->ui.edit.accel[2] = CreateWindow(L"EDIT", L"0", WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_BORDER | ES_NUMBER | ES_CENTER, 200, 25, 25, 20, device->ui.pane, NULL, hInstance, NULL);
	// buttons
	device->ui.button.set = CreateWindow(L"BUTTON", L"Set", WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON | BS_FLAT, 250, 25, 40, 20, device->ui.pane, NULL, hInstance, NULL);

	RECT r;
	GetWindowRect(_window, &r);
	//dprintf(L"%d %d %d %d\n", r.left, r.right, r.top, r.bottom);
	MoveWindow(_window, r.left, r.top, WINDOW_WIDTH, r.bottom - r.top + PANE_HEIGHT, FALSE);
	InvalidateRect(_window, NULL, TRUE);
	UpdateWindow(_window);
	/*InvalidateRect(device->ui.pane, NULL, TRUE);
	UpdateWindow(device->ui.pane);*/
	
	dprintf(L"added device to ui: [%d] (%x) %s\n", _num_devices, (UINT)device->hDevice, device->name);
#undef VOFF
}

static void _updateSettings(_ui_device_t *device) {
	TCHAR buf[16];
	UINT speed, accel[3];

	GetWindowText(device->ui.edit.speed, buf, _tsizeof(buf));
	speed = _tstoi(buf);
	GetWindowText(device->ui.edit.accel[0], buf, _tsizeof(buf));
	accel[0] = _tstoi(buf);
	GetWindowText(device->ui.edit.accel[1], buf, _tsizeof(buf));
	accel[1] = _tstoi(buf);
	GetWindowText(device->ui.edit.accel[2], buf, _tsizeof(buf));
	accel[2] = _tstoi(buf);

	inSetDeviceSpeed(device->hDevice, speed, accel[0], accel[1], accel[2]);
}

/***************
* WINDOW PROCS *
****************/
static LRESULT CALLBACK _WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	//dprintf(L"wndproc: (%x) %x %x %x\n", (UINT)hWnd, uMsg, wParam, lParam);

	switch (uMsg) {
	case WM_CREATE:
		CHECK_ERROR_EXIT(inRegisterMice(hWnd) == FALSE, -3, L"could not register raw input device");
		break;
	case WM_CTLCOLORSTATIC:
		//dprintf(L"WM_CTLCOLORSTATIC: (%x) %x %x %x\n", (UINT)hWnd, uMsg, wParam, lParam);
		_ui_device_t *dev = _getDeviceByPane((HWND)lParam);
		if (dev) {
			if (dev == _active_device) {
				//dprintf(L"pane dev: %x\n", dev->name);
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
		inProcessRawInput((HRAWINPUT)lParam);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	return 0;
}

static LRESULT CALLBACK _paneWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	//dprintf(L"paneproc: (%x) %x %x %x\n", (UINT)hWnd, uMsg, wParam, lParam);

	_ui_device_t *dev = _getDeviceByPane(hWnd);
	if (!dev)
		return CallWindowProc(dev->ui.oldproc, hWnd, uMsg, wParam, lParam);

	switch (uMsg) {
	case WM_COMMAND:
		//dprintf(L"sWM_COMMAND: (%x) %x %x %x\n", (UINT)hWnd, uMsg, wParam, lParam);
		if ((HWND)lParam == dev->ui.button.set) {
			_updateSettings(dev);
			dprintf(L"saving %s\n", dev->name);
		}
		break;
	case WM_CTLCOLORSTATIC:
		//dprintf(L"sWM_CTLCOLORSTATIC: (%x) %x %x %x\n", (UINT)hWnd, uMsg, wParam, lParam);
		if (dev == _active_device) {
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
	return CallWindowProc(dev->ui.oldproc, hWnd, uMsg, wParam, lParam);
}

/**********
 * GLOBAL *
 **********/

HWND uiInit(HINSTANCE hInstance) {
	WNDCLASS wc = { 0 };

	_num_devices = 0;
	_active_device = NULL;
	_fills.window = CreateSolidBrush(WINDOW_COLOR);
	_fills.active = CreateSolidBrush(ACTIVE_COLOR);
	_fills.inactive = CreateSolidBrush(INACTIVE_COLOR);

	wc.lpfnWndProc = _WndProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = UI_WINDOW_CLASS_NAME;
	wc.hbrBackground = (HBRUSH) COLOR_BACKGROUND;
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

	return _window;
}

BOOL uiSetDevice(HANDLE hDevice) {
	if (!hDevice)
		return FALSE;

	_ui_device_t *dev = _getDeviceByHandle(hDevice);
	if (!dev) {
		dev = _addDevice(hDevice);
		_setActive(dev);
		_createDevicePane(dev);
	} else {
		_setActive(dev);
	}

	return TRUE;
}