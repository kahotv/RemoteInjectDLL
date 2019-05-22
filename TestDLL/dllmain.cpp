#include <Windows.h>
#include <filesystem>

std::string GetExeName()
{
	char tmp[MAX_PATH];
	GetModuleFileNameA(NULL, tmp, sizeof(tmp));
	std::experimental::filesystem::path path = tmp;

	if (path.has_filename())
		return path.filename().string();
	return "";
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
	OutputDebugStringA("DllMain");

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
	{
		char txt[0x100];

		std::string filename = GetExeName();
	
		wsprintfA(txt,
			"事件:\t\tDLL_PROCESS_ATTACH\r\n"
			"当前进程名:\t%s\n"
			"当前进程ID:\t%d\r\n"
			"模块句柄:\t\t0x%p\r\n",
			filename.c_str(),
			GetCurrentProcessId(),
			hModule);

		//auto h = GetActiveWindow();
		//MessageBoxA(GetActiveWindow(), txt, "TestDLL", MB_OK);
		OutputDebugStringA(txt);
		break;
	}
    case DLL_THREAD_ATTACH:
		break;
    case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
	{
		char txt[0x100];

		std::string filename = GetExeName();

		wsprintfA(txt,
			"事件:\t\tDLL_PROCESS_DETACH\r\n"
			"当前进程名:\t%s\n"
			"当前进程ID:\t%d\r\n"
			"模块句柄:\t\t0x%p\r\n",
			filename.c_str(),
			GetCurrentProcessId(),
			hModule);

		OutputDebugStringA(txt);
		//MessageBoxA(GetActiveWindow(), txt, "TestDLL", MB_OK);
		break;
	}
    }
    return TRUE;
}

