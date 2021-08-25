#include "stdafx.h"
#include "IATHookHelper.h"
#include "ExceptionHelper.h"
#include "SynchronizationMonitor.h"

SRWLOCK* SynchronizationMonitor::s_pSRWLock = NULL;
LPVOID SynchronizationMonitor::s_lpDataAddress = NULL;
ProcCreateThread SynchronizationMonitor::s_lpCreateThread = NULL;
ProcRtlLock SynchronizationMonitor::s_lpRtlAcquireSRWLockExclusive = NULL;
ProcRtlLock SynchronizationMonitor::s_lpRtlAcquireSRWLockShared = NULL;
ProcRtlLock SynchronizationMonitor::s_lpRtlReleaseSRWLockExclusive = NULL;
ProcRtlLock SynchronizationMonitor::s_lpRtlReleaseSRWLockShared = NULL;

BOOL SynchronizationMonitor::EnableDataBP(BOOL bAllThread, AccessType accessType)
{
	if (bAllThread)
		return BreakpointHelper::SetHWDataBPOnAllThread(s_lpDataAddress, accessType);
	return BreakpointHelper::SetHWDataBPOnThread(GetCurrentThread(), s_lpDataAddress, accessType);
}

BOOL SynchronizationMonitor::DisableDataBP(BOOL bAllThread)
{
	if (bAllThread)
		return BreakpointHelper::RemoveHWDataBPOnAllThread(s_lpDataAddress);
	return BreakpointHelper::RemoveHWDataBPOnThread(GetCurrentThread(), s_lpDataAddress);
}

VOID __stdcall SynchronizationMonitor::FakeRtlAcquireSRWLockShared(_Inout_ PRTL_SRWLOCK SRWLock)
{
	(*s_lpRtlAcquireSRWLockShared)(SRWLock);

	if (SRWLock == s_pSRWLock && s_pSRWLock != NULL)
	{
		DisableDataBP();
		EnableDataBP(FALSE, AccessType::WRITE);
	}
}

VOID __stdcall SynchronizationMonitor::FakeRtlReleaseSRWLockShared(_Inout_ PRTL_SRWLOCK SRWLock)
{
	(*s_lpRtlReleaseSRWLockShared)(SRWLock);

	if (SRWLock == s_pSRWLock && s_pSRWLock != NULL)
		EnableDataBP();
}

VOID WINAPI SynchronizationMonitor::FakeRtlAcquireSRWLockExclusive(PRTL_SRWLOCK SRWLock)
{
	(*s_lpRtlAcquireSRWLockExclusive)(SRWLock);

	if (SRWLock == s_pSRWLock && s_pSRWLock != NULL)
		DisableDataBP();
}

VOID __stdcall SynchronizationMonitor::FakeRtlReleaseSRWLockExclusive(_Inout_ PRTL_SRWLOCK SRWLock)
{
	(*s_lpRtlReleaseSRWLockExclusive)(SRWLock);
	
	if (SRWLock == s_pSRWLock && s_pSRWLock != NULL)
		EnableDataBP();
}

HANDLE __stdcall SynchronizationMonitor::FakeCreateThread(
	_In_opt_  LPSECURITY_ATTRIBUTES  lpThreadAttributes,
	_In_      SIZE_T                 dwStackSize,
	_In_      LPTHREAD_START_ROUTINE lpStartAddress,
	_In_opt_  LPVOID                 lpParameter,
	_In_      DWORD                  dwCreationFlags,
	_Out_opt_ LPDWORD                lpThreadId
)
{
	HANDLE hThread = (*s_lpCreateThread)(lpThreadAttributes, dwStackSize, lpStartAddress, lpParameter, dwCreationFlags | CREATE_SUSPENDED, lpThreadId);
	
	if (s_pSRWLock)
		BreakpointHelper::SetHWDataBPOnThread(hThread, s_lpDataAddress, AccessType::READWRITE);
	
	if ((dwCreationFlags & CREATE_SUSPENDED) == 0)
		ResumeThread(hThread);

	return hThread;
}

BOOL SynchronizationMonitor::Initialize()
{
	ExceptionHelper::Initialize();

	if (!IATHookHelper::HookFunctionByName("CreateThread", FakeCreateThread, reinterpret_cast<LPVOID&>(s_lpCreateThread)))
		return FALSE;
	if (!IATHookHelper::HookFunctionByName("AcquireSRWLockShared", FakeRtlAcquireSRWLockShared, reinterpret_cast<LPVOID&>(s_lpRtlAcquireSRWLockShared)))
		return FALSE;
	if (!IATHookHelper::HookFunctionByName("ReleaseSRWLockShared", FakeRtlReleaseSRWLockShared, reinterpret_cast<LPVOID&>(s_lpRtlReleaseSRWLockShared)))
		return FALSE;
	if (!IATHookHelper::HookFunctionByName("AcquireSRWLockExclusive", FakeRtlAcquireSRWLockExclusive, reinterpret_cast<LPVOID&>(s_lpRtlAcquireSRWLockExclusive)))
		return FALSE;
	if (!IATHookHelper::HookFunctionByName("ReleaseSRWLockExclusive", FakeRtlReleaseSRWLockExclusive, reinterpret_cast<LPVOID&>(s_lpRtlReleaseSRWLockExclusive)))
		return FALSE;

	return TRUE;
}

BOOL SynchronizationMonitor::UnInitialize()
{
	ExceptionHelper::UnInitialize();

	LPVOID lpTemp;
	if (!IATHookHelper::HookFunctionByName("CreateThread", s_lpCreateThread, lpTemp))
		return FALSE;
	if (!IATHookHelper::HookFunctionByName("AcquireSRWLockShared", s_lpRtlAcquireSRWLockShared, lpTemp))
		return FALSE;
	if (!IATHookHelper::HookFunctionByName("ReleaseSRWLockShared", s_lpRtlReleaseSRWLockShared, lpTemp))
		return FALSE;
	if (!IATHookHelper::HookFunctionByName("AcquireSRWLockExclusive", s_lpRtlAcquireSRWLockExclusive, lpTemp))
		return FALSE;
	if (!IATHookHelper::HookFunctionByName("ReleaseSRWLockExclusive", s_lpRtlReleaseSRWLockExclusive, lpTemp))
		return FALSE;

	return TRUE;
}

BOOL SynchronizationMonitor::SetTarget(SRWLOCK* pSRWLock, LPVOID lpDataAddress)
{
	if (lpDataAddress == NULL || pSRWLock == NULL)
		return FALSE;
	
	s_pSRWLock = NULL;
	if (!DisableDataBP(TRUE))
	{
		return FALSE;
	}

	s_pSRWLock = pSRWLock, s_lpDataAddress = lpDataAddress;
	
	return EnableDataBP(TRUE);
}