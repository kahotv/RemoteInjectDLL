// Test.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <string>
#include <filesystem>
#include <Windows.h>
#include "RemoteInjectDLL.h"

#ifdef _WIN64
#define DLL_NAME		"TestDLL64.dll"
#else
#define DLL_NAME		"TestDLL.dll"
#endif

std::string	GetModuleFilePath(const char* name)
{
	char tmppath[MAX_PATH];
	HMODULE h = GetModuleHandleA(name);

	if (0 != GetModuleFileNameA(h, tmppath, sizeof(tmppath)))
	{
		std::experimental::filesystem::path path = tmppath;

		if (std::experimental::filesystem::is_regular_file(path))
		{
			return path.parent_path().generic_string();
		}
	}
	return "";
}

int main()
{
	DWORD pid;
	std::string dllName = DLL_NAME;

	//提权
	std::cout << "提权:" << (AdjustSeltTokenPrivileges() ? "成功" : "失败") << std::endl;

	std::cout << "输入进程ID:";
	std::cin >> pid;
	
	//取EXE目录
	std::string path = GetModuleFilePath(NULL) + "/" + dllName;
	std::cout << "注入 " << path << " 到进程 " << pid << std::endl;
	HMODULE h = RemoteInjectDLL(pid, path.c_str());
	std::cout << "模块句柄:" << h << std::endl;

	std::cout << "任意键卸载DLL" << std::endl;
	system("pause");

	RemoteFreeDLL(pid, h);

	std::cout << "卸载DLL完毕" << std::endl;

	system("pause");
}
