#pragma once
#include "SynchronizationMonitor.h"

extern "C" {
	__declspec(dllexport) BOOL SetTarget(SRWLOCK* pSRWLock, LPVOID lpDataAddress);
}