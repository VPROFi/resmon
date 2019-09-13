#ifndef __RESMONLIB_H__
#define __RESMONLIB_H__

#include <common/types.h>

#ifdef __cplusplus
extern "C"
{
#endif

//------------------------------------------------------------------------------
// defaults resmonitoring values
//------------------------------------------------------------------------------
#define MON_MAXIMUM_DISK_USAGE (1024*1024*128)            // 128 mb
#define MON_MAXIMUM_CPU_USAGE 30                          // 30%
#define MON_MAXIMUM_MEMORY_USAGE (1024*1024*128)          // 128 mb
#define MON_MAXIMUM_THREAD_USAGE (100-1)                  // 100
#define MON_MAXIMUM_SYNCHRONIZATION_OBJECT_USAGE (180-1)  // 180
#define MON_DEFAULT_RES_MONITORING_INTERVAL 5000          // 5 sec

typedef struct {
	ssize_tr            maxMemoryUsedLimit;
	unsigned long long  maxDiskUsedLimit;
	signed long         maxThreadUsedLimit;
	signed long         maxSyncObjectUsedLimit;
	unsigned char       maxCpuUsedLimit; // in percent
	unsigned long       resMonInterval; // im ms
} MonLimits;

#define MON_DEFAULT_LIMITS ((MonLimits *)0)

signed __stdcall MonLogAllResources(void);
signed __stdcall InstallResMonitoringSystem(void * base, MonLimits * limits);
signed __stdcall UninstallResMonitoringSystem(void);

#ifdef __cplusplus
}
#endif

#endif // __RESMONLIB_H__
