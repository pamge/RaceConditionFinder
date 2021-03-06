// Example.cpp : 此檔案包含 'main' 函式。程式會於該處開始執行及結束執行。
//

#include "pch.h"
#include <iostream>
#include <Windows.h>

volatile int g_nTestCounter = 0;

__declspec(noinline) void Operation()
{
	g_nTestCounter++;
	g_nTestCounter--;
}

DWORD WINAPI TestThreadProc1(LPVOID lpParameter)
{
	int i = 0, j = 0;
	SRWLOCK* pSRWLock = reinterpret_cast<SRWLOCK*>(lpParameter);
	while (i++ < 3000)
	{
		AcquireSRWLockExclusive(pSRWLock);
		for (j = 0; j < 50000; j++)
		{
			Operation();
		}
		ReleaseSRWLockExclusive(pSRWLock);
	}
	return 0;
}

DWORD WINAPI TestThreadProc2(LPVOID lpParameter)
{
	int i = 0, j = 0;
	SRWLOCK* pSRWLock = reinterpret_cast<SRWLOCK*>(lpParameter);
	while (i++ < 3000)
	{
		AcquireSRWLockExclusive(pSRWLock);
		for (j = 0; j < 50000; j++)
		{
			Operation();
		}
		ReleaseSRWLockExclusive(pSRWLock);
	}
	return 0;
}

DWORD WINAPI TestThreadProc3(LPVOID lpParameter)
{
	int i = 0, j = 0, count = 0;
	SRWLOCK* pSRWLock = reinterpret_cast<SRWLOCK*>(lpParameter);
	while (i++ < 3000)
	{
		AcquireSRWLockShared(pSRWLock);
		for (j = 0; j < 50000; j++)
		{
			Operation();
		}
		ReleaseSRWLockShared(pSRWLock);
	}
	return 0;
}
int main()
{
	SRWLOCK srw;
	InitializeSRWLock(&srw);

	typedef BOOL(*SetTarget)(SRWLOCK*, LPVOID);
	HMODULE hModule = LoadLibrary(L"RaceConditionFinder.dll");
	SetTarget pSetTarget = (SetTarget)GetProcAddress(hModule, "SetTarget");
	pSetTarget(&srw, (LPVOID)&g_nTestCounter);

	HANDLE hThread1 = CreateThread(0, 0, TestThreadProc1, &srw, 0, NULL);
	HANDLE hThread2 = CreateThread(0, 0, TestThreadProc2, &srw, 0, NULL);
	HANDLE hThread3 = CreateThread(0, 0, TestThreadProc3, &srw, 0, NULL);

	WaitForSingleObject(hThread1, INFINITE);
	WaitForSingleObject(hThread2, INFINITE);
	WaitForSingleObject(hThread3, INFINITE);
	return 0;
}
