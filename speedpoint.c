//#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <strsafe.h>
#include <tchar.h>

#define VC_EXTRALEAN

#ifdef _DEBUG
#define dprintf(...) _tprintf(__VA_ARGS__)
#else
#define dprintf(...)
#endif

#define CHECK_ERROR_EXIT(cond, exitcode, errmsg) if ((cond)) {\
	_tprintf(L"ERROR (%d): %s\n", GetLastError(), (errmsg));\
	exit((exitcode));\
}

//#define DEVICE_TYPENAME (const TCHAR*[3]) { L"mouse",L"keyboard",L"hid" }
const TCHAR *dev_typename[] = { L"mouse",L"keyboard",L"hid" };

void PrintDevices(void) {
	UINT num_devices;
	PRAWINPUTDEVICELIST devices;

	GetRawInputDeviceList(NULL, &num_devices, sizeof(RAWINPUTDEVICELIST));
	devices = (PRAWINPUTDEVICELIST) malloc(sizeof(RAWINPUTDEVICELIST) * num_devices);
	GetRawInputDeviceList(devices, &num_devices, sizeof(RAWINPUTDEVICELIST));

	dprintf(L"devices:\n");
	for (UINT c = 0; c < num_devices; c++) {
		UINT            datasz = 256;
		TCHAR           devname[256] = { 0 };
		RID_DEVICE_INFO devinfo;

		dprintf(L"  %-8s %x\n", dev_typename[devices[c].dwType], (unsigned int)devices[c].hDevice);
		datasz = sizeof(devname);
		GetRawInputDeviceInfo(devices[c].hDevice, RIDI_DEVICENAME, devname, &datasz);
		datasz = sizeof(RID_DEVICE_INFO);
		GetRawInputDeviceInfo(devices[c].hDevice, RIDI_DEVICEINFO, &devinfo, &datasz);
		dprintf(L"    name: %s\n", devname);

		/*if (RIM_TYPEMOUSE == devicesp[c].dwType && rid_index < REGISTERED_DEVICE_LIMIT) {
			rid[rid_index].
			rid_index++;
		}*/
	}
}

BOOL RegisterMice(HWND hWnd) {
	RAWINPUTDEVICE rid;
	rid.usUsagePage = 0x01;
	rid.usUsage = 0x02;
	rid.dwFlags = RIDEV_NOLEGACY | RIDEV_INPUTSINK;   // adds HID mouse and also ignores legacy mouse messages
	rid.hwndTarget = hWnd;

	return RegisterRawInputDevices(&rid, 1, sizeof(RAWINPUTDEVICE));
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	//dprintf(L"wndproc: (%x) %x %x %x\n", (UINT)hWnd, uMsg, wParam, lParam);

	switch (uMsg) {
	case WM_CREATE:
		CHECK_ERROR_EXIT(RegisterMice(hWnd) == FALSE, -3, L"could not register raw input device");
		break;
	case WM_INPUT:
		UINT      sz;
		LPBYTE    buf;
		PRAWINPUT raw;

		GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &sz, sizeof(RAWINPUTHEADER));
		buf = malloc(sz);
		if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, buf, &sz, sizeof(RAWINPUTHEADER)) != sz)
		dprintf(L"WARNING: GetRawInputData returned bad size\n");
		raw = (PRAWINPUT)buf;

		if (raw->header.dwType == RIM_TYPEMOUSE) {
			//dprintf(L"device: %x\n", raw->header.hDevice);
			UINT            datasz = 256;
			TCHAR           devname[256] = { 0 };
			RID_DEVICE_INFO devinfo;

			datasz = sizeof(devname);
			GetRawInputDeviceInfo(raw->header.hDevice, RIDI_DEVICENAME, devname, &datasz);
			datasz = sizeof(RID_DEVICE_INFO);
			GetRawInputDeviceInfo(raw->header.hDevice, RIDI_DEVICEINFO, &devinfo, &datasz);
			dprintf(L"device: %d %s\n", devinfo.mouse.dwId, devname);
		}

		free(buf);
		break;
	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow) {
	WNDCLASS wc  = { 0 };
	MSG      msg = { 0 };
	HWND     hWnd;
	BOOL     bRet;

#ifdef _DEBUG
	FILE *stream;
	if (AttachConsole(ATTACH_PARENT_PROCESS)) {
		freopen_s(&stream, "CON", "w", stdout);
		PrintDevices();
	}
#endif

	wc.lpfnWndProc   = WndProc;
	wc.hInstance     = hInstance;
	wc.lpszClassName = L"SpeedPoint";
	CHECK_ERROR_EXIT(RegisterClass(&wc) == 0, -1, L"could not register class"); // now that the class is registered we create the window
	hWnd = CreateWindow(wc.lpszClassName, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);
	CHECK_ERROR_EXIT(hWnd == NULL, -2, L"could not create window");

	dprintf(L"entering message loop\n");
	while ((bRet = GetMessage(&msg, hWnd, 0, 0)) != 0) {
		if (bRet == -1) {
			return bRet;
		} else {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	FreeConsole();
	return msg.wParam; // Return the exit code to the system. 
}