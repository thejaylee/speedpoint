#include "device.h"
#include "ui.h"

#define DEFAULT_SPEED 8
#define DEFAULT_ACCEL_LOW 6
#define DEFAULT_ACCEL_HIGH 10
#define DEFAULT_ACCEL_STATE 0

//#define DEVICE_TYPENAME (const TCHAR*[3]) { L"mouse",L"keyboard",L"hid" }
static const TCHAR *dev_typename[] = { L"mouse",L"keyboard",L"hid" };

static UINT _num_devices = 0;
static device_info_t _devices[MAX_DEVICES];

/**********
* STATIC *
**********/

static _updateSystemParams(device_info_t *device) {
	return SystemParametersInfo(SPI_SETMOUSESPEED, 0, (PVOID)device->speed, SPIF_SENDCHANGE) & SystemParametersInfo(SPI_SETMOUSE, 0, device->accel, SPIF_SENDCHANGE);
}

device_info_t *devGetByHandle(HANDLE hDevice) {
	for (UINT c = 0; c < _num_devices; c++) {
		if (_devices[c].hDevice == hDevice)
			return &_devices[c];
	}

	return NULL;
}

/**********
* GLOBALS *
**********/

device_info_t *devAdd(HANDLE hDevice) {
	device_info_t *devinfo;
	UINT bufsz = _tsizeof(devinfo->name);
	devinfo = &_devices[_num_devices++];
	devinfo->hDevice = hDevice;
	devinfo->speed = DEFAULT_SPEED;
	devinfo->accel[0] = DEFAULT_ACCEL_LOW;
	devinfo->accel[1] = DEFAULT_ACCEL_HIGH;
	devinfo->accel[2] = DEFAULT_ACCEL_STATE;
	GetRawInputDeviceInfo(hDevice, RIDI_DEVICENAME, devinfo->name, &bufsz);
	return devinfo;
}

UINT devCount(void) {
	return _num_devices;
}

void devPrintDevices(void) {
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

//void inGetDevices(DWORD dwType, HANDLE *deviceList, UINT count) {}

BOOL devRegisterMice(HWND hWnd) {
	RAWINPUTDEVICE rid;
	rid.usUsagePage = 0x01;
	rid.usUsage = 0x02;
	rid.dwFlags = RIDEV_INPUTSINK; // inputsink captures while window not in foreground
	rid.hwndTarget = hWnd;

	return RegisterRawInputDevices(&rid, 1, sizeof(RAWINPUTDEVICE));
}

void devProcessRawInput(HRAWINPUT hRawInput) {
	static device_info_t *lastDev;
	device_info_t *devinfo = NULL;
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

		if (!lastDev || raw.header.hDevice != lastDev->hDevice) {
			if ((devinfo = devGetByHandle(raw.header.hDevice)) == NULL) {
				dprintf(L"new device:\n");
				devinfo = devAdd(raw.header.hDevice);
			}
			uiSetActive(devinfo); // ui maintains it's own device cache, can call without checking ours
			_updateSystemParams(devinfo);
		}
		lastDev = devinfo;
	}

	//DefRawInputProc(&raw, 1, sizeof(RAWINPUTHEADER));
}

BOOL devSetMouseParams(device_info_t *devinfo, UINT speed, UINT accel1, UINT accel2, UINT accel3) {
	if (!devinfo)
		return FALSE;

	devinfo->speed = speed;
	devinfo->accel[0] = accel1;
	devinfo->accel[1] = accel2;
	devinfo->accel[2] = accel3;

	dprintf(L"mouseparams: speed: %u accel: %u %u %u\n", speed, accel1, accel2, accel3);
	return _updateSystemParams(devinfo);
}