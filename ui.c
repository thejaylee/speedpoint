#include "ui.h"
#include "input.h"

#define ROW_HEIGHT 100

typedef struct {
	HANDLE hDevice;
	TCHAR  name[256];
	struct {
		HWND container;
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
			HWND save;
		} button;
	} ui;
} _ui_device_t;

_ui_device_t _devices[MAX_DEVICES] = { 0 };
UINT _num_devices;
HWND _window;

/**********
* STATIC *
**********/

_ui_device_t *_getOrAddDevice(HANDLE hDevice) {
#define ROFF ((_num_devices - 1) * ROW_HEIGHT) // row offset
	if (!hDevice)
		return NULL;

	for (UINT c = 0; c < _num_devices; c++) {
		if (_devices[c].hDevice == hDevice)
			return &_devices[c];
	}

	_ui_device_t *dev;
	HINSTANCE hInstance = (HINSTANCE) GetWindowLong(_window, GWL_HINSTANCE);
	dev = &_devices[_num_devices++];
	dev->hDevice = hDevice;
	inGetDeviceName(hDevice, dev->name, _tsizeof(dev->name));
	dprintf(L"added device to ui: [%d] (%x) %s\n", _num_devices, hDevice, dev->name);

	// border
	dev->ui.container = CreateWindow(L"STATIC", dev->name, WS_VISIBLE | WS_CHILD | SS_ETCHEDFRAME, 5, ROFF + 5, 680, 100, _window, NULL, hInstance, NULL);
	// labels
	dev->ui.label.device = CreateWindow(L"STATIC", dev->name, WS_VISIBLE | WS_CHILD | SS_LEFTNOWORDWRAP,  5, ROFF +  5, 670, 20, dev->ui.container, NULL, hInstance, NULL);
	dev->ui.label.speed  = CreateWindow(L"STATIC", L"speed" , WS_VISIBLE | WS_CHILD | SS_LEFTNOWORDWRAP, 15, ROFF + 26,  50, 20, dev->ui.container, NULL, hInstance, NULL);
	dev->ui.label.accel  = CreateWindow(L"STATIC", L"accel" , WS_VISIBLE | WS_CHILD | SS_LEFTNOWORDWRAP, 100, ROFF + 26,  50, 20, dev->ui.container, NULL, hInstance, NULL);
	// edits
	dev->ui.edit.speed    = CreateWindow(L"EDIT", L"6", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER | ES_CENTER, 60, ROFF + 25, 25, 20, dev->ui.container, NULL, hInstance, NULL);
	dev->ui.edit.accel[0] = CreateWindow(L"EDIT", L"6", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER | ES_CENTER, 140, ROFF + 25, 25, 20, dev->ui.container, NULL, hInstance, NULL);
	dev->ui.edit.accel[1] = CreateWindow(L"EDIT", L"6", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER | ES_CENTER, 170, ROFF + 25, 25, 20, dev->ui.container, NULL, hInstance, NULL);
	dev->ui.edit.accel[2] = CreateWindow(L"EDIT", L"6", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER | ES_CENTER, 200, ROFF + 25, 25, 20, dev->ui.container, NULL, hInstance, NULL);
	// buttons
	dev->ui.button.save = CreateWindow(L"BUTTON", L"Save", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_FLAT, 250, ROFF + 25, 40, 20, dev->ui.container, NULL, hInstance, NULL);

	return dev;
#undef VOFF
}

LRESULT CALLBACK uiWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	dprintf(L"wndproc: (%x) %x %x %x\n", (UINT)hWnd, uMsg, wParam, lParam);

	switch (uMsg) {
	case WM_CREATE:
		CHECK_ERROR_EXIT(inRegisterMice(hWnd) == FALSE, -3, L"could not register raw input device");
		break;
	case WM_INPUT:
		inProcessRawInput((HRAWINPUT)lParam);
		break;
	case WM_LBUTTONUP:
		dprintf(L"clicked: (%x) %x %x %x\n", (UINT)hWnd, uMsg, wParam, lParam);
		break;
	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	return 0;
}

/**********
 * GLOBAL *
 **********/

HWND uiInit(HINSTANCE hInstance) {
	WNDCLASS wc = { 0 };

	_num_devices = 0;
	wc.lpfnWndProc = uiWndProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = UI_WINDOW_CLASS_NAME;
	CHECK_ERROR_EXIT(RegisterClass(&wc) == 0, -1, L"could not register class"); // now that the class is registered we create the window
	/*_window = CreateWindow(wc.lpszClassName, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);*/
	_window = CreateWindowEx(
		0,
		wc.lpszClassName,
		UI_WINDOW_NAME,
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		700,
		150,
		NULL,
		NULL,
		hInstance,
		NULL);
	CHECK_ERROR_EXIT(_window == NULL, -2, L"could not create window");

	return _window;
}

BOOL uiSetDevice(HANDLE hDevice) {
	_ui_device_t *dev = _getOrAddDevice(hDevice);
	if (!dev)
		return FALSE;

	return TRUE;
}