#pragma once
#include <stdio.h>
#include "definitions.h"

void storLoadSettings(const char *filename, device_info_t *devices[], UINT sz);
void storSaveSettings(const char *filename, device_info_t *devices[], UINT num_devices);