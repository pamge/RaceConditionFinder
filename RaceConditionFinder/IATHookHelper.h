#include "stdafx.h"

class IATHookHelper
{
public:
	static BOOL HookFunctionByName(const std::string& strFunctionName, LPVOID lpTarget, LPVOID& lpOriginal);
protected:
	static PIMAGE_IMPORT_DESCRIPTOR WINAPI GetFirstImportDescriptor();
};