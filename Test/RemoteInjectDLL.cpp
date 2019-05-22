#include "RemoteInjectDLL.h"
#include <stdio.h>
#include <filesystem>
#pragma pack(push,1)
#ifdef _WIN64
#define InjectDLLShllCode InjectDLLShllCode64
struct InjectDLLShllCode64
{
	BYTE		_1[2];
	SIZE_T		addr_dll_name;
	BYTE		_2[3];
	SIZE_T		addr_loadlibrarya;
	BYTE		_3[4];
	SIZE_T		addr_ret;
	BYTE		_4[6];
	SIZE_T		addr_exitthread;
	BYTE		_5[3];
	InjectDLLShllCode64() :
		_1{ 0x48,0xB9 },
		_2{ 0x51,0x48,0xB8 },
		_3{ 0xFF,0xD0,0x48,0XA3 },
		_4{ 0x48,0x31,0xC9,0x51,0x48,0xB8 },
		_5{ 0xFF,0xD0,0xCC },
		addr_dll_name(0),
		addr_loadlibrarya(0),
		addr_ret(0),
		addr_exitthread(0)
	{
		/*
		mov		rcx,addr_dll_name
		push	rcx
		mov		rax,addr_loadlibrarya
		call	rax
		mov		qword ptr [addr_ret],rax
		xor		rcx,rcx
		push	rcx
		mov		rax,addr_exitthread
		call	rax
		int3
		*/
	}
};
#else
#define InjectDLLShllCode InjectDLLShllCode32
struct InjectDLLShllCode32
{
	BYTE		_1[1];
	SIZE_T		addr_dll_name;
	BYTE		_2[1];
	SIZE_T		addr_loadlibrarya;
	BYTE		_3[3];
	SIZE_T		addr_ret;
	BYTE		_4[3];
	SIZE_T		addr_exitthread;
	BYTE		_5[3];
	InjectDLLShllCode32() :
		_1{ 0x68 },
		_2{ 0xB8 },
		_3{ 0xFF,0xD0,0xA3 },
		_4{ 0x6A,0x00,0xB8 },
		_5{ 0xFF,0xD0,0xCC },
		addr_dll_name(0),
		addr_loadlibrarya(0),
		addr_ret(0),
		addr_exitthread(0)
	{
		/*
		push	addr_dll_name
		mov		eax,addr_loadlibrarya
		call	eax
		mov		dword ptr[addr_ret],eax
		push	0
		mov		eax,addr_exitthread
		call	eax
		int3
		*/
	}
};
#endif
#pragma pack(pop)

//��ȡDLL����
SIZE_T	GetDllFunc(const char* dllName, const char* funcName)
{
	HMODULE h = LoadLibraryA(dllName);
	if (h > 0)
	{
		return (SIZE_T)GetProcAddress(h, funcName);
	}
	return 0;
}

//ע��DLL������ģ����
HMODULE RemoteInjectDLL(DWORD pid, const char* path)
{

	HMODULE hModule = 0;
	HANDLE hProcess = 0;
	HANDLE hThread = 0;
	SIZE_T pRemoteBuf = 0;
	SIZE_T pathLen = 0;
	do
	{
		if (path == NULL)
		{
			printf("DLL·��Ϊ��\n");
			break;
		}
		pathLen = strlen(path);

		if (pathLen == 0)
		{
			printf("DLL·��Ϊ��\n");
			break;
		}

		if (!std::experimental::filesystem::is_regular_file(path))
		{
			printf("DLL�ļ�������\n");
			break;
		}

		hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
		if (hProcess <= 0)
		{
			printf("�򿪽���[%d]ʧ�ܣ��������%d\n", pid, GetLastError());
			break;
		}

		pRemoteBuf = (SIZE_T)VirtualAllocEx(hProcess, NULL, 0x1000, MEM_COMMIT, PAGE_EXECUTE_READWRITE);  // ��Ŀ����̿ռ��������ڴ�

		//pRemoteBuf:
		//+0=����ֵ;+0x10=����;+0x100=shellcode

		if (pRemoteBuf <= 0)
		{
			printf("�����ڴ�ʧ�ܣ��������%d\n",GetLastError());
			break;
		}

		//׼������غ����Ͳ���
		SIZE_T pLoadLibraryA = GetDllFunc("kernel32.dll", "LoadLibraryA");
		SIZE_T pExitThread = GetDllFunc("kernel32.dll", "ExitThread");


		//����shellcode
		InjectDLLShllCode sc;

		sc.addr_dll_name = pRemoteBuf + 0x10;
		sc.addr_exitthread = pExitThread;
		sc.addr_loadlibrarya = pLoadLibraryA;
		sc.addr_ret = pRemoteBuf;

		//д���ڴ�	����ֵ	��0
		WriteProcessMemory(hProcess, (void*)(pRemoteBuf + 0x0), &hModule, sizeof(hModule), NULL);
		//д���ڴ�	����	DLL·��
		WriteProcessMemory(hProcess, (void*)(pRemoteBuf + 0x10), path, pathLen, NULL);
		//д���ڴ�	shellcode 
		WriteProcessMemory(hProcess, (void*)(pRemoteBuf + 0x100), &sc, sizeof(sc), NULL);

		//�����߳�
		hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)(pRemoteBuf + 0x100), NULL, 0, NULL);

		if (hThread <= 0)
		{
			printf("����Զ���߳�ʧ��,�������%d\n", GetLastError());
			break;
		}

		//�ȴ��߳̽���
		WaitForSingleObject(hThread, INFINITE);

		//��ȡ����ֵ
		if (0 == ReadProcessMemory(hProcess, (void*)pRemoteBuf, &hModule, sizeof(hModule), NULL))
		{
			hModule = 0;
		}

	} while (false);

	if (hThread > 0)
		CloseHandle(hThread);

	if (pRemoteBuf > 0)
		VirtualFreeEx(hProcess, (void*)pRemoteBuf, 0, MEM_FREE);

	if (hProcess > 0)
		CloseHandle(hProcess);
	return hModule;
}

//ж��ָ�����̵�ָ��ģ��
BOOL RemoteFreeDLL(DWORD pid,HMODULE hModule)
{
	HANDLE hProcess = 0;
	HANDLE hThread = 0;
	do
	{
		if (hModule <= 0)
		{
			printf("ģ����Ϊ��\n");
			break;
		}

		hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
		if (INVALID_HANDLE_VALUE == hProcess)
		{
			printf("�򿪽���ʧ�ܣ��������%d\n",GetLastError());
			break;
		}
		//׼������غ����Ͳ���
		SIZE_T pFreeLibrary = GetDllFunc("kernel32.dll", "FreeLibrary");

		//�����̣߳���������ģ����
		hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)(pFreeLibrary), hModule, 0, NULL);

		if (hThread > 0)
		{
			//�ȴ��߳̽���
			WaitForSingleObject(hThread, INFINITE);
		}

	} while (false);

	if (hThread > 0)
		CloseHandle(hThread);

	if (hProcess > 0)
		CloseHandle(hProcess);
	return TRUE;
}

BOOL	AdjustToken()
{
	HANDLE hToken;
	TOKEN_PRIVILEGES tp;
	LUID luid;

	if (FALSE == OpenProcessToken(
		GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
	{
		return FALSE;
	}

	if (!LookupPrivilegeValueA(NULL, "SeDebugPrivilege", &luid))
	{
		return FALSE;
	}

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL))
	{
		return FALSE;
	}

	return TRUE;
}