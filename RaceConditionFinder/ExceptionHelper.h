#include "stdafx.h"
#include <vector>
#include <psapi.h>

class ExceptionHelper
{
public:
	static void Initialize();
	static void UnInitialize();
	static LONG WINAPI VectoredExceptionHandler(PEXCEPTION_POINTERS pExceptionInfo);
	static LONG WINAPI UnhandledExceptionFilter(PEXCEPTION_POINTERS pExceptionInfo);
protected:
	struct Module
	{
		std::wstring Name;
		MODULEINFO Info;
	};
	static LPTOP_LEVEL_EXCEPTION_FILTER m_lpPreviousExceptionFilter;
protected:
	static void PrintStack(LPVOID lpStack, int nMaximumDepth=20);
	static void PrintRegisters(PEXCEPTION_POINTERS pExceptionInfo);
	static void PrintAllModulesInfo();
	static void PrintBriefExceptionInfo(PEXCEPTION_POINTERS pExceptionInfo);
	static void RawLog(TCHAR * format, ...);
	static std::wstring GetMiniDumpPath();
private:
	static PVOID s_lpHandle;
	static BOOL s_bIsInMyException;
	static std::vector<Module> s_arModules;
};
