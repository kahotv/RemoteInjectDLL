#pragma once
#include <Windows.h>
//ע��DLL������ģ����
HMODULE	RemoteInjectDLL	(DWORD pid, const char* path);
BOOL	RemoteFreeDLL	(DWORD pid, HMODULE hModule);