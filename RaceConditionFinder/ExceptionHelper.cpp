#include "stdafx.h"
#include <ctime>
#include <fstream>
#include <stdarg.h>
#include <DbgHelp.h>
#include <shlwapi.h>
#include "ExceptionHelper.h"
#include "SynchronizationMonitor.h"

PVOID ExceptionHelper::s_lpHandle = NULL;
BOOL ExceptionHelper::s_bIsInMyException = FALSE;
std::vector<ExceptionHelper::Module> ExceptionHelper::s_arModules;
LPTOP_LEVEL_EXCEPTION_FILTER ExceptionHelper::m_lpPreviousExceptionFilter = NULL;

void ExceptionHelper::Initialize()
{
	s_lpHandle = AddVectoredExceptionHandler(0, VectoredExceptionHandler);
	m_lpPreviousExceptionFilter = SetUnhandledExceptionFilter(UnhandledExceptionFilter);
}

void ExceptionHelper::UnInitialize()
{
	RemoveVectoredExceptionHandler(s_lpHandle);
	SetUnhandledExceptionFilter(m_lpPreviousExceptionFilter);
}

LONG WINAPI ExceptionHelper::VectoredExceptionHandler(PEXCEPTION_POINTERS pExceptionInfo)
{
	if (s_bIsInMyException == FALSE && pExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_SINGLE_STEP)
	{
		s_bIsInMyException = TRUE;
		PrintBriefExceptionInfo(pExceptionInfo);
	}
	return EXCEPTION_CONTINUE_SEARCH;
}

LONG WINAPI ExceptionHelper::UnhandledExceptionFilter(PEXCEPTION_POINTERS pExceptionInfo)
{
	if (s_bIsInMyException && pExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_SINGLE_STEP)
	{
		// Fill exception info
		MINIDUMP_EXCEPTION_INFORMATION mei;
		mei.ThreadId = ::GetCurrentThreadId();
		mei.ExceptionPointers = pExceptionInfo;
		mei.ClientPointers = TRUE;
		// Generate mini dump 
		// Note: We need to change exception code, or the callstack will include exception flow when exception code eqauls EXCEPTION_SINGLE_STEP.
		std::wstring strDumpPath = GetMiniDumpPath();
		HANDLE hFile = CreateFile(strDumpPath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		pExceptionInfo->ExceptionRecord->ExceptionCode = EXCEPTION_BREAKPOINT;
		MiniDumpWriteDump(GetCurrentProcess(), ::GetCurrentProcessId(), hFile, MiniDumpNormal, &mei, NULL, NULL);
		pExceptionInfo->ExceptionRecord->ExceptionCode = EXCEPTION_SINGLE_STEP;
		RawLog(L"Dump: %s\n", strDumpPath.c_str());
		s_bIsInMyException = FALSE;
	}
	if (m_lpPreviousExceptionFilter != NULL)
		return m_lpPreviousExceptionFilter(pExceptionInfo);
	return EXCEPTION_CONTINUE_SEARCH;
}

void ExceptionHelper::PrintStack(LPVOID lpStack, int nMaximumDepth)
{
	__try
	{
		RawLog(L"Stack:\n");
		for (int i = 0; i < nMaximumDepth; i++)
		{
			_ULONG* lpAddress = static_cast<_ULONG*>(lpStack) + i;
#ifdef _WIN64
			RawLog(L"\t%016llX(ESP+%03d) 0x%016llX", lpAddress, i * sizeof(_ULONG*), *lpAddress);
#else
			RawLog(L"\t%08X(ESP+%03d) 0x%08X", lpAddress, i * sizeof(_ULONG*), *lpAddress);
#endif
			for (size_t j = 0; j < s_arModules.size(); j++)
			{
				
				_ULONG* lpValue = reinterpret_cast<_ULONG*>(*lpAddress);
				_ULONG* lpStartAddress = static_cast<_ULONG*>(s_arModules[j].Info.lpBaseOfDll);
				_ULONG* lpEndAddress = reinterpret_cast <_ULONG*>(static_cast<char*>(s_arModules[j].Info.lpBaseOfDll) + s_arModules[j].Info.SizeOfImage);
				if (lpValue >= lpStartAddress && lpValue < lpEndAddress)
				{
#ifdef _WIN64
					RawLog(L" (%s + 0x%llX)", s_arModules[j].Name.c_str(), lpValue - lpStartAddress);
#else
					RawLog(L" (%s + 0x%X)", s_arModules[j].Name.c_str(), lpValue - lpStartAddress);
#endif
					break;
				}
			}
			RawLog(L"\n");
			
		}
		RawLog(L"\n");
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return;
	}
}

void ExceptionHelper::PrintRegisters(PEXCEPTION_POINTERS pExceptionInfo)
{
	RawLog(L"Registers:\n");
	PCONTEXT pContext = pExceptionInfo->ContextRecord;
#ifdef _WIN64
	RawLog(L"\tRIP = 0x%016llX RSP = 0x%016llX RBP = 0x%016llX\n", pContext->Rip, pContext->Rsp, pContext->Rbp);
	RawLog(L"\tRAX = 0x%016llX RBX = 0x%016llX RCX = 0x%016llX\n", pContext->Rax, pContext->Rbx, pContext->Rcx);
	RawLog(L"\tRDX = 0x%016llX RDI = 0x%016llX Dr7 = 0x%016llX\n", pContext->Rdx, pContext->Rdi, pContext->Dr7);
#else
	RawLog(L"\tEIP = 0x%08X ESP = 0x%08X EBP = 0x%08X\n", pContext->Eip, pContext->Esp, pContext->Ebp);
	RawLog(L"\tEAX = 0x%08X EBX = 0x%08X ECX = 0x%08X\n", pContext->Eax, pContext->Ebx, pContext->Ecx);
	RawLog(L"\tEDX = 0x%08X EDI = 0x%08X Dr7 = 0x%08X\n", pContext->Edx, pContext->Edi, pContext->Dr7);
#endif
	RawLog(L"\n");
}

void ExceptionHelper::PrintAllModulesInfo()
{
	DWORD cbNeeded;
	HMODULE hMods[1024] = { 0 };
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, GetCurrentProcessId());

	if (hProcess)
	{
		if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded))
		{
			RawLog(L"Modules:\n");
			s_arModules.clear();
			s_arModules.resize(cbNeeded / sizeof(HMODULE));
			for (size_t i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
			{
				TCHAR szModName[MAX_PATH];
				if (GetModuleBaseName(hProcess, hMods[i], szModName, sizeof(szModName) / sizeof(TCHAR))
					&& GetModuleInformation(hProcess, hMods[i], &s_arModules[i].Info, sizeof(MODULEINFO)))
				{
					s_arModules[i].Name = szModName;
#ifdef _WIN64
					RawLog(L"\t(0x%016llX - 0x%016llX) %s\n", s_arModules[i].Info.lpBaseOfDll,
						static_cast<const char*>(s_arModules[i].Info.lpBaseOfDll) + s_arModules[i].Info.SizeOfImage, szModName);
#else
					RawLog(L"\t(0x%08X - 0x%08X) %s\n", s_arModules[i].Info.lpBaseOfDll,
						static_cast<const char*>(s_arModules[i].Info.lpBaseOfDll) + s_arModules[i].Info.SizeOfImage, szModName);
#endif
				}
			}
			RawLog(L"\n");
		}
		CloseHandle(hProcess);
	}
}

void ExceptionHelper::PrintBriefExceptionInfo(PEXCEPTION_POINTERS pExceptionInfo)
{
	struct tm t;
	time_t t_now = time(0);
	localtime_s(&t, &t_now);
	TCHAR szDateTime[64] = { 0 }, message[1024] = { 0 };
	wcsftime(szDateTime, sizeof(szDateTime), L"%Y/%m/%d %H:%M:%S", &t);

	RawLog(L"%s\n", std::string(50, '*').c_str());
	RawLog(L"Exception Time: %s\n\n", szDateTime);
	PrintRegisters(pExceptionInfo);
	PrintAllModulesInfo();
#ifdef _WIN64
	PrintStack(reinterpret_cast<LPVOID>(pExceptionInfo->ContextRecord->Rsp));
#else
	PrintStack(reinterpret_cast<LPVOID>(pExceptionInfo->ContextRecord->Esp));
#endif
}

void ExceptionHelper::RawLog(TCHAR * format, ...)
{
	TCHAR message[2048] = { 0 };
	va_list args;
	va_start(args, format);
	_vsnwprintf_s(message, 2048, format, args);
	va_end(args);
	std::wofstream log_file(std::to_string(GetCurrentProcessId()) + ".log", std::ios_base::out | std::ios_base::app);
	log_file << message;
}

std::wstring ExceptionHelper::GetMiniDumpPath()
{
	struct tm t;
	time_t t_now = time(0);
    localtime_s(&t, &t_now);
	TCHAR szFileName[MAX_PATH] = { 0 }, szOutFilePath[MAX_PATH] = { 0 };
	wcsftime(szFileName, sizeof(szFileName), L"%Y%m%d%H%M%S.dmp", &t);
	swprintf_s(szOutFilePath, MAX_PATH, L"%d_%s", GetCurrentProcessId(), szFileName);
	return szOutFilePath;
}
