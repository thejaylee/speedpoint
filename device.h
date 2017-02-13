#pragma once
#include <windows.h>
#include "definitions.h"

void devPrintDevices(void);
UINT devGetName(HANDLE hDevice, TCHAR *name, UINT strlen);
UINT devCount(void);
device_info_t *devGetByHandle(HANDLE hDevice);
BOOL devRegisterMice(HWND hWnd);
void devProcessRawInput(HRAWINPUT hRawInput);
BOOL devSetMouseParams(device_info_t *device, UINT speed, UINT accel[]);