#pragma once

#include <windows.h>
#include "definitions.h"

#define UI_WINDOW_CLASS_NAME L"speedpoint"
#define UI_WINDOW_NAME L"SpeedPoint"

HWND uiInit();

BOOL uiSetDevice(HANDLE hDevice);
//LRESULT CALLBACK uiWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);