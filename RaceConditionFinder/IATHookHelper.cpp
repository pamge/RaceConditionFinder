#include "stdafx.h"
#include "IATHookHelper.h"

BOOL IATHookHelper::HookFunctionByName(const std::string& strFunctionName, LPVOID lpTarget, LPVOID& lpOriginal)
{
	HMODULE hModule = GetModuleHandle(NULL);
	PIMAGE_IMPORT_DESCRIPTOR pImportDescriptor = GetFirstImportDescriptor();
	if (pImportDescriptor == NULL)
		return FALSE;

	PIMAGE_THUNK_DATA originalFirstThunk;
	PIMAGE_THUNK_DATA firstThunk;
	PIMAGE_IMPORT_BY_NAME importByName;

	while (pImportDescriptor->OriginalFirstThunk)
	{
		originalFirstThunk = (PIMAGE_THUNK_DATA)pImportDescriptor->OriginalFirstThunk;
		originalFirstThunk = (PIMAGE_THUNK_DATA)((BYTE*)originalFirstThunk + (UINT_PTR)hModule);
		firstThunk = (PIMAGE_THUNK_DATA)pImportDescriptor->FirstThunk;
		firstThunk = (PIMAGE_THUNK_DATA)((BYTE*)firstThunk + (UINT_PTR)hModule);

		while (originalFirstThunk->u1.Function)
		{
			importByName = (PIMAGE_IMPORT_BY_NAME)originalFirstThunk->u1.AddressOfData;
			importByName = (PIMAGE_IMPORT_BY_NAME)((BYTE*)importByName + ((UINT_PTR)hModule));
			const char* pImportName = (LPCSTR)((BYTE*)importByName + sizeof(WORD));
			if (strcmp(pImportName, strFunctionName.c_str()) == 0) 
			{
				DWORD flOldProtect;
				lpOriginal = (LPVOID)firstThunk->u1.Function;
				if (!VirtualProtect(&firstThunk->u1.Function, sizeof(LPVOID), PAGE_EXECUTE_READWRITE, &flOldProtect))
					return FALSE;
#ifdef _WIN64
				firstThunk->u1.Function = (ULONGLONG)lpTarget;
#else
				firstThunk->u1.Function = (DWORD)lpTarget;
#endif
				return VirtualProtect(&firstThunk->u1.Function, sizeof(LPVOID), flOldProtect, &flOldProtect);
			}
			originalFirstThunk++;
			firstThunk++;
		}
		pImportDescriptor++;
	}

	return FALSE;
}

PIMAGE_IMPORT_DESCRIPTOR WINAPI IATHookHelper::GetFirstImportDescriptor() {

	HMODULE hModule = GetModuleHandle(NULL);
	if (((PIMAGE_DOS_HEADER)hModule)->e_magic != IMAGE_DOS_SIGNATURE)
		return NULL;
	PIMAGE_NT_HEADERS pNTHeader = (PIMAGE_NT_HEADERS)((BYTE*)hModule + ((PIMAGE_DOS_HEADER)hModule)->e_lfanew);
	if (pNTHeader->Signature != IMAGE_NT_SIGNATURE)
		return NULL;
	PIMAGE_IMPORT_DESCRIPTOR pFirstImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
	if (pFirstImportDescriptor == NULL)
		return NULL;
	return (PIMAGE_IMPORT_DESCRIPTOR)((BYTE*)pFirstImportDescriptor + (UINT_PTR)hModule);
}
