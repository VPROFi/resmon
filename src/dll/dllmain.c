#define WIN32_LEAN_AND_MEAN 1
#include "Windows.h"
#include <common/dbglog.h>
#include <common/checkptr.h>
#include <resmon/resmon.h>
#include <resmon/resmonlib.h>
#include <modedep/osdep.h>

static HANDLE gMainThread = 0;

extern __declspec(dllexport) void __cdecl InstallResMon(void)
{
	InstallResMonitoringSystem(GetModuleHandle(0), MON_DEFAULT_LIMITS);

	if( gMainThread )
		return;

	// add for monitoring current thread
	gMainThread = OpenThread(THREAD_ALL_ACCESS, FALSE, GetCurrentThreadId());
	if( gMainThread ) {
		void * ctx = MonCreateThreadBegin();
		if( UPTR(ctx) ) {
			void * adr = _ReturnAddress();
			long status = NtQueryInformationThread(
							gMainThread,
							ThreadQuerySetWin32StartAddress,
							&adr, sizeof(void *), 0);

			if( status < 0 ) {
				LOG_ERROR("NtQueryInformationThread() error 0x%08X", status);
			}
			MonCreateThreadEnd(
						ctx,
						gMainThread,
						_ReturnAddress(),
						adr,
						(void *)GetCurrentThreadId());
		} else {
			CloseHandle(gMainThread);
			gMainThread = 0;
		}
	}
}

extern __declspec(dllexport) void __cdecl UninstallResMon(void)
{
	if( gMainThread ) {
		MonClose(gMainThread);
		CloseHandle(gMainThread);
		gMainThread = 0;
	}
	UninstallResMonitoringSystem();
}

extern __declspec(dllexport) void __cdecl LogAllResources(void)
{
	MonLogAllResources();
}


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
	    case DLL_PROCESS_ATTACH:
			LOG_INFO(": ************************************");
			LOG_INFO(": resmon.dll load");
			LOG_INFO(": Built on "__DATE__" at "__TIME__ " ");
			LOG_INFO(": ************************************ ");
			break;
    	case DLL_THREAD_ATTACH:
			LOG_INFO("DLL_THREAD_ATTACH");
			break;
	    case DLL_THREAD_DETACH:
			LOG_INFO("DLL_THREAD_DETACH");
			break;
    	case DLL_PROCESS_DETACH:
			LOG_INFO(": ************************************");
			LOG_INFO(": resmon.dll unload");
			LOG_INFO(": ************************************ ");
        	break;
    }
    return TRUE;
}

