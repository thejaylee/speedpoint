#pragma once
#include <windows.h>

void inPrintDevices();
BOOL inRegisterMice(HWND hWnd);
UINT inGetDeviceName(HANDLE hDevice, TCHAR *name, UINT strlen);
void inProcessRawInput(HRAWINPUT hRawInput);