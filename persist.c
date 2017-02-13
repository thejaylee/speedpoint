#include "persist.h"

typedef struct _persist_device {
	TCHAR name[NAME_MAX_LENGTH];
	UINT speed;
	UINT accel[3];
	struct _persist_device *next;
} persist_device_t ;

static persist_device_t *firstdev = NULL;

void _freeList() {
	persist_device_t *next;
	while (firstdev) {
		next = firstdev->next;
		free(firstdev);
		firstdev = next;
	}
}

INT perLoadFile() {
	FILE *fp;
	TCHAR buf[4096];
	UINT count = 0;
	persist_device_t *dev = firstdev;

	ERROR_RETURN(_tfopen_s(&fp, PERSIST_FILENAME, L"r"), -1, L"ERROR: could not open settings file\n");

	_freeList();
	while (_fgetts(buf, _countof(buf), fp)) {
		if (!firstdev) {
			firstdev = malloc(sizeof(persist_device_t));
			memset(firstdev, 0, sizeof(persist_device_t));
			dev = firstdev;
		} else {
			dev->next = malloc(sizeof(persist_device_t));
			memset(dev->next, 0, sizeof(persist_device_t));
			dev = dev->next;
		}

		_stscanf_s(buf, L"%s %u %u %u %u", dev->name, _countof(dev->name), &dev->speed, &dev->accel[0], &dev->accel[1], &dev->accel[2]);
		dprintf(L"loaded: %s %u %u %u %u\n", dev->name, dev->speed, dev->accel[0], dev->accel[1], dev->accel[2]);
		count++;
	}

	fclose(fp);
	return count;
}

INT perSaveFile() {
	FILE *fp;
	UINT count = 0;

	_tfopen_s(&fp, PERSIST_FILENAME, L"w");
	for (persist_device_t *pdev = firstdev; pdev; pdev = pdev->next) {
		_ftprintf_s(fp, L"%s %u %u %u %u\n", pdev->name, pdev->speed, pdev->accel[0], pdev->accel[1], pdev->accel[2]);
	}
	fclose(fp);
	return count;
}

BOOL perGetMouseParams(const TCHAR *devname, UINT *speed, UINT accel[]) {
	for (persist_device_t *pdev = firstdev; pdev; pdev = pdev->next) {
		if (_tcscmp(devname, pdev->name) == 0) {
			*speed = pdev->speed;
			accel[0] = pdev->accel[0];
			accel[1] = pdev->accel[1];
			accel[2] = pdev->accel[2];
			return TRUE;
		}
	}
	return FALSE;
}

void perSetMouseParams(const TCHAR *devname, UINT speed, UINT accel[]) {
	persist_device_t *pdev = firstdev;
	while (pdev) {
		if (_tcscmp(devname, pdev->name) == 0) {
			dprintf(L"persist update: %s %u %u %u %u\n", devname, speed, accel[0], accel[1], accel[2]);
			goto copyvals;
		}

		if (!pdev->next) {
			dprintf(L"persist new: %s %u %u %u %u\n", devname, speed, accel[0], accel[1], accel[2]);
			pdev->next = malloc(sizeof(persist_device_t));
			pdev = pdev->next;
			memset(pdev, 0, sizeof(persist_device_t));
			goto copyname;
		}

		pdev = pdev->next;
	}

	pdev = malloc(sizeof(persist_device_t));
	memset(pdev, 0, sizeof(persist_device_t));
copyname:
	_tcscpy_s(pdev->name, _countof(pdev->name), devname);
copyvals:
	pdev->speed = speed;
	pdev->accel[0] = accel[0];
	pdev->accel[1] = accel[1];
	pdev->accel[2] = accel[2];

	perSaveFile();
}