#include "stdafx.h"
#include <tlhelp32.h>
#include "BreakpointHelper.h"

inline void BreakpointHelper::SetBits(_ULONG& dw, int lowBit, int bits, int newValue)
{
	int mask = (1 << bits) - 1;
	dw = (dw & ~(mask << lowBit)) | (newValue << lowBit);
}

BOOL BreakpointHelper::RemoveHWDataBPOnThread(HANDLE hThread, LPVOID lpAddress)
{
	CONTEXT context;
	context.ContextFlags = CONTEXT_DEBUG_REGISTERS;
	if (!GetThreadContext(hThread, &context))
		return FALSE;

	BOOL flags[Dr3 + 1] = {0};
	if (context.Dr0 == reinterpret_cast<_ULONG>(lpAddress))
		context.Dr0 = NULL, flags[Dr0] = TRUE;
	if (context.Dr1 == reinterpret_cast<_ULONG>(lpAddress))
		context.Dr1 = NULL, flags[Dr1] = TRUE;
	if (context.Dr2 == reinterpret_cast<_ULONG>(lpAddress))
		context.Dr2 = NULL, flags[Dr2] = TRUE;
	if (context.Dr3 == reinterpret_cast<_ULONG>(lpAddress))
		context.Dr3 = NULL, flags[Dr3] = TRUE;

	for (int index = Dr0; index <= Dr3; index++)
	{
		if (flags[index] == FALSE) continue;
		// Set local enable to false
		SetBits(reinterpret_cast<_ULONG&>(context.Dr7), index * 2, 1, 0); 
		// Set breakpoint type
		SetBits(reinterpret_cast<_ULONG&>(context.Dr7), 16 + (index * 4), 2, AccessType::EXECUTION);
		// Set breakpoint size
		SetBits(reinterpret_cast<_ULONG&>(context.Dr7), 18 + (index * 4), 2, 0);
	}

	return SetThreadContext(hThread, &context) ? TRUE : FALSE;
}

BOOL BreakpointHelper::SetHWDataBPOnThread(HANDLE hThread, LPVOID lpAddress, AccessType type)
{
	int index = 0;
	CONTEXT context;
	context.ContextFlags = CONTEXT_DEBUG_REGISTERS;
	if (!GetThreadContext(hThread, &context))
		return FALSE;
	
	// Has already been set
	if (context.Dr0 == reinterpret_cast<_ULONG>(lpAddress) || context.Dr1 == reinterpret_cast<_ULONG>(lpAddress) || 
		context.Dr2 == reinterpret_cast<_ULONG>(lpAddress) || context.Dr3 == reinterpret_cast<_ULONG>(lpAddress))
		return TRUE;

	// Find availabe dr register
	for (index = Dr0; index <= Dr3; index++)
	{
#ifdef _WIN64
		if ((context.Dr7 & (1ULL << (index * 2))) == 0)
			break;
#else
		if ((context.Dr7 & (1 << (index * 2))) == 0)
			break;
#endif
	}

	// Set Dr{index} register to data address
	switch (index)
	{
		case 0:
			context.Dr0 = reinterpret_cast<_ULONG>(lpAddress);
			break;
		case 1:
			context.Dr1 = reinterpret_cast<_ULONG>(lpAddress);
			break;
		case 2:
			context.Dr2 = reinterpret_cast<_ULONG>(lpAddress);
			break;
		case 3:
			context.Dr3 = reinterpret_cast<_ULONG>(lpAddress);
			break;
		default:
			return FALSE;
	}

	// Set local enable to true
	SetBits(reinterpret_cast<_ULONG&>(context.Dr7), index * 2, 1, 1);
	// Set breakpoint type
	SetBits(reinterpret_cast<_ULONG&>(context.Dr7), 16 + (index * 4), 2, type);
	// Set breakpoint size
	SetBits(reinterpret_cast<_ULONG&>(context.Dr7), 18 + (index * 4), 2, 3);
	
	return SetThreadContext(hThread, &context) ? TRUE : FALSE;
}

BOOL BreakpointHelper::SetHWDataBPOnAllThread(LPVOID lpAddress, AccessType type)
{
	BOOL bRet = TRUE;
	THREADENTRY32 te32;
	HANDLE handle = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (handle != INVALID_HANDLE_VALUE)
	{
		te32.dwSize = sizeof(THREADENTRY32);
		if (Thread32First(handle, &te32)) {
			do {
				if (te32.th32OwnerProcessID == GetCurrentProcessId()) {
					HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, te32.th32ThreadID);
					if (te32.th32ThreadID != GetCurrentThreadId()) SuspendThread(hThread);
					bRet = bRet & SetHWDataBPOnThread(hThread, lpAddress, type);
					if (te32.th32ThreadID != GetCurrentThreadId()) ResumeThread(hThread);
					CloseHandle(hThread);
				}
		   } while (Thread32Next(handle, &te32));
		}
		CloseHandle(handle);
	}
	return bRet;
}

BOOL BreakpointHelper::RemoveHWDataBPOnAllThread(LPVOID lpAddress)
{
	BOOL bRet = TRUE;
	THREADENTRY32 te32;
	HANDLE handle = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (handle != INVALID_HANDLE_VALUE)
	{
		te32.dwSize = sizeof(THREADENTRY32);
		if (Thread32First(handle, &te32)) {
			do {
				if (te32.th32OwnerProcessID == GetCurrentProcessId()) {
					HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, te32.th32ThreadID);
					if (te32.th32ThreadID != GetCurrentThreadId()) SuspendThread(hThread);
					bRet = bRet & RemoveHWDataBPOnThread(hThread, lpAddress);
					if (te32.th32ThreadID != GetCurrentThreadId()) ResumeThread(hThread);
					CloseHandle(hThread);
				}
		   } while (Thread32Next(handle, &te32));
		}
		CloseHandle(handle);
	}
	return bRet;
}