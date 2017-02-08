#include "input.h"
#include "definitions.h"
#include "ui.h"

//#define DEVICE_TYPENAME (const TCHAR*[3]) { L"mouse",L"keyboard",L"hid" }
const TCHAR *dev_typename[] = { L"mouse",L"keyboard",L"hid" };

typedef struct {
	HANDLE hDevice;
	UINT speed;
	UINT accel[3];
} _devinfo_t;

_devinfo_t _devices[MAX_DEVICES];

void inPrintDevices(void) {
	UINT num_devices;
	PRAWINPUTDEVICELIST devices;

	GetRawInputDeviceList(NULL, &num_devices, sizeof(RAWINPUTDEVICELIST));
	devices = (PRAWINPUTDEVICELIST)malloc(sizeof(RAWINPUTDEVICELIST) * num_devices);
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

/**
* @param[in]  hDevice   a HANDLE to the device
* @param[out] name      pointer to a character buffer to hold device name
* @param[in]  strlen	length (in characters) of the buffer
*/
UINT inGetDeviceName(HANDLE hDevice, TCHAR *name, UINT strlen) {
	return GetRawInputDeviceInfo(hDevice, RIDI_DEVICENAME, name, &strlen);
}

//void inGetDevices(DWORD dwType, HANDLE *deviceList, UINT count) {}

BOOL inRegisterMice(HWND hWnd) {
	RAWINPUTDEVICE rid;
	rid.usUsagePage = 0x01;
	rid.usUsage = 0x02;
	rid.dwFlags = RIDEV_INPUTSINK; // inputsink captures while window not in foreground
	rid.hwndTarget = hWnd;

	return RegisterRawInputDevices(&rid, 1, sizeof(RAWINPUTDEVICE));
}

void inProcessRawInput(HRAWINPUT hRawInput) {
	UINT     sz;
	RAWINPUT raw;

	GetRawInputData(hRawInput, RID_INPUT, NULL, &sz, sizeof(RAWINPUTHEADER));
	if (sz > sizeof(raw)) {
		dprintf(L"raw input buf is too small\n");
		return;
	}
	if (GetRawInputData(hRawInput, RID_INPUT, &raw, &sz, sizeof(RAWINPUTHEADER)) != sz) {
		dprintf(L"WARNING: GetRawInputData returned bad size\n");
		return;
	}

	if (raw.header.dwType == RIM_TYPEMOUSE) {
		//dprintf(L"device: %x\n", raw->header.hDevice);
		/*UINT            datasz = 256;
		TCHAR           devname[256] = { 0 };
		RID_DEVICE_INFO devinfo;

		datasz = sizeof(devname);
		GetRawInputDeviceInfo(raw->header.hDevice, RIDI_DEVICENAME, devname, &datasz);
		datasz = sizeof(RID_DEVICE_INFO);
		GetRawInputDeviceInfo(raw->header.hDevice, RIDI_DEVICEINFO, &devinfo, &datasz);
		dprintf(L"device: %d %s\n", devinfo.mouse.dwId, devname);*/


		uiSetDevice(raw.header.hDevice);
	}

	//DefRawInputProc(&raw, 1, sizeof(RAWINPUTHEADER));
}

BOOL inSetDeviceSpeed(UINT speed, UINT accel1, UINT accel2, UINT accel3) {
	UINT accel[3];
	accel[0] = accel1;
	accel[1] = accel2;
	accel[2] = accel3;

	return TRUE;
}