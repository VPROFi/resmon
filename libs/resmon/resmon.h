#ifndef __RESMON_H__
#define __RESMON_H__

#include <resmon/resmonlib.h>

#ifdef __cplusplus
extern "C"
{
#endif

void MonAlloc(void * mem, void * returnAddress, size_tr size, ptr_t tag, unsigned long mtype);
void MonFree(void * ptr);
void MonFreeAllByTag(ptr_t tag);

// times
typedef struct {
	// total time used detail
	unsigned long long idleTime;
	unsigned long long kernelTime;
	unsigned long long userTime;
} ResSystemTimes;

typedef struct {
	unsigned char cpuUserUsed;
	unsigned char cpuKrnlUsed;
	unsigned char cpuTotalUsed;
} ResPercentTimes;

typedef struct {
	unsigned long long CreateTime;
	unsigned long long ExitTime;
	unsigned long long KernelTime;
	unsigned long long UserTime;
} ResAbsoluteTimes;

typedef struct {
	ResPercentTimes pcnt;
	ResAbsoluteTimes abst;
	unsigned long long diskUsed;
} ResObjectCounters;

void MonUpdateTimes(
				unsigned long long krnlTime,
				unsigned long long userTime,
				ResObjectCounters * cnt,
				const ResSystemTimes * timesInit);

void * MonCreateThreadBegin(void);
void MonCreateThreadEnd(void * beginCtx,
						void * thread,
						void * returnAddress,
						void * startAddress,
						void * threadId);

// synchronization types
typedef enum {
	EventSync,
	MutexSync,
	SemaphoreSync,
	ResourceSync,
	LastSync
} SyncType;

void * MonCreateSyncBegin(void);
extern void MonCreateSyncEnd(
						void * beginCtx,
						void * sync,
						SyncType syncType,
						void * returnAddress,
						unsigned long parametr1,
						unsigned long parametr2);

void MonClose(void * obj);

#define LOG_PREFIX "|resmon| "

#ifdef __cplusplus
}
#endif

#endif // __RESMON_H__
