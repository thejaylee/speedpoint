#pragma once
#include <stdio.h>
#include "definitions.h"

#define PERSIST_FILENAME L"speedpoint.dat"

INT perLoadFile();
INT perSaveFile();
BOOL perGetMouseParams(const TCHAR *devname, UINT *speed, UINT accel[]);
void perSetMouseParams(const TCHAR *devname, UINT speed, UINT accel[]);