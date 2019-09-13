#include <common/dbglog.h>
#include <shared/config.h>
#include "res/resource.h"
#include <resmon/resmonlib.h>
#include <stdio.h>
#include <tchar.h>

int __cdecl _tmain(int argc, _TCHAR* argv[])
{
	ASSERT( argc );
	LOG_INFO(": ************************************");
	LOG_INFO(": resmon client at 0x%p", GetModuleHandle(0));
	LOG_INFO(": Built on "__DATE__" at "__TIME__ " ");
	LOG_INFO(": ************************************ ");
	// TODO: not implemented yet
	//InstallResMonitoringSystem(GetModuleHandle(0), 0);
	//MonLogAllResources();
	//UninstallResMonitoringSystem();
	return 0;
}
