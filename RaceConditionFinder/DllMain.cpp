#include "stdafx.h"
#include "RaceConditionFinder.h"

BOOL SetTarget(SRWLOCK* pSRWLock, LPVOID lpDataAddress)
{
	return SynchronizationMonitor::SetTarget(pSRWLock, lpDataAddress);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		if (!SynchronizationMonitor::Initialize())
			return FALSE;
		break;
	case DLL_PROCESS_DETACH:
		if (!SynchronizationMonitor::UnInitialize())
			return FALSE;
		break;
	}
	return TRUE;
}