#include "stdafx.h"
#include "BreakpointHelper.h"

typedef VOID(NTAPI *ProcRtlLock)(PRTL_SRWLOCK);
typedef HANDLE(NTAPI *ProcCreateThread)(LPSECURITY_ATTRIBUTES, SIZE_T, LPTHREAD_START_ROUTINE, LPVOID, DWORD, LPDWORD);

class SynchronizationMonitor
{
public:
	static BOOL Initialize();
	static BOOL UnInitialize();
	static BOOL SetTarget(SRWLOCK* pSRWLock, LPVOID lpDataAddress);
	
private:
	static ProcCreateThread s_lpCreateThread;
	static ProcRtlLock s_lpRtlAcquireSRWLockExclusive;
	static ProcRtlLock s_lpRtlAcquireSRWLockShared;
	static ProcRtlLock s_lpRtlReleaseSRWLockExclusive;
	static ProcRtlLock s_lpRtlReleaseSRWLockShared;
	static SRWLOCK* s_pSRWLock;
	static LPVOID s_lpDataAddress;

private:
	static BOOL EnableDataBP(BOOL bAllThread = FALSE, AccessType accessType = AccessType::READWRITE);
	static BOOL DisableDataBP(BOOL bAllThread = FALSE);
	static VOID __stdcall FakeRtlAcquireSRWLockShared(_Inout_ PRTL_SRWLOCK SRWLock);
	static VOID __stdcall FakeRtlReleaseSRWLockShared(_Inout_ PRTL_SRWLOCK SRWLock);
	static VOID __stdcall FakeRtlAcquireSRWLockExclusive(_Inout_ PRTL_SRWLOCK SRWLock);
	static VOID __stdcall FakeRtlReleaseSRWLockExclusive(_Inout_ PRTL_SRWLOCK SRWLock);
	static HANDLE __stdcall FakeCreateThread(
		_In_opt_  LPSECURITY_ATTRIBUTES  lpThreadAttributes,
		_In_      SIZE_T                 dwStackSize,
		_In_      LPTHREAD_START_ROUTINE lpStartAddress,
		_In_opt_  LPVOID                 lpParameter,
		_In_      DWORD                  dwCreationFlags,
		_Out_opt_ LPDWORD                lpThreadId
	);
};