//#define _CRT_SECURE_NO_WARNINGS
#define VC_EXTRALEAN

#include <windows.h>
#include <strsafe.h>
#include <tchar.h>

#include "definitions.h"
#include "device.h"
#include "persist.h"
#include "ui.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow) {
	MSG      msg = { 0 };
	HWND     hWnd;
	BOOL     bRet;

#ifdef _DEBUG
	FILE *stream;
	if (AttachConsole(ATTACH_PARENT_PROCESS)) {
		freopen_s(&stream, "CON", "w", stdout);
		devPrintDevices();
	}
#endif

#ifdef _DEBUG
	dprintf(L"loaded %d device settings from file\n", perLoadFile());
#else
	perLoadFile();
#endif
	devInit();
	hWnd = uiInit(hInstance);

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