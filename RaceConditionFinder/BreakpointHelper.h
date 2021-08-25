#include "stdafx.h"

enum AccessType { EXECUTION  = 0, WRITE = 1, READWRITE = 3 };

class BreakpointHelper
{
	enum
	{
		Dr0 = 0,
		Dr1 = 1,
		Dr2 = 2,
		Dr3 = 3,
	};

public:
	static BOOL RemoveHWDataBPOnThread(HANDLE hThread, LPVOID lpAddress);
	static BOOL SetHWDataBPOnThread(HANDLE hThread, LPVOID lpAddress, AccessType type = AccessType::READWRITE);
	static BOOL RemoveHWDataBPOnAllThread(LPVOID lpAddress);
	static BOOL SetHWDataBPOnAllThread(LPVOID lpAddress, AccessType type = AccessType::READWRITE);	
protected:
	static inline void SetBits(_ULONG& dw, int lowBit, int bits, int newValue);
};