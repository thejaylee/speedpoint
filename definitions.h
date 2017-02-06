#pragma once

#include <tchar.h>
#include <Windows.h>

#ifdef _DEBUG
#define dprintf(...) _tprintf(__VA_ARGS__)
#else
#define dprintf(...)
#endif

#define CHECK_ERROR_EXIT(cond, exitcode, errmsg) if ((cond)) {\
	_tprintf(L"ERROR (%d): %s\n", GetLastError(), (errmsg));\
	exit((exitcode));\
}

#define MAX_DEVICES 8

#define _tsizeof(x) (sizeof((x))/sizeof(TCHAR))

typedef struct {
	HANDLE hDevice;
	TCHAR  name[256];
} device_t;