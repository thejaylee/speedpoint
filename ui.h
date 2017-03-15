#pragma once

#include <windows.h>
#include "definitions.h"

#define UI_WINDOW_CLASS_NAME L"speedpoint"
#define UI_WINDOW_NAME L"SpeedPoint"

HWND uiInit(HINSTANCE hInstance);
BOOL uiSetActive(device_info_t *dev);
void uiTerminate();