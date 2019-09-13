#ifdef _KERNEL_MODE
#error This code only for user mode
#endif
#include <common/dbglog.h>
#include <common/checkptr.h>
#include <modedep/modedep.h>
#include <resmon/resmon.h>
//------------------------------------------------------------------------------
// OsGetSystemTimes
//------------------------------------------------------------------------------
extern signed OsGetSystemTimes(ResSystemTimes * resTimes)
{
	if(!GetSystemTimes(
			(LPFILETIME)&resTimes->idleTime,
			(LPFILETIME)&resTimes->kernelTime,
			(LPFILETIME)&resTimes->userTime)) {
		LOG_ERROR("GetSystemTimes(): error %u", GetLastError());
		return FALSE;
	}
	return TRUE;
}

//------------------------------------------------------------------------------
// OsGetThreadPerfomance
//------------------------------------------------------------------------------
extern void OsGetThreadPerfomance(
				__in void * threadHandle,
				__inout ResObjectCounters * cnt,
				__in const ResSystemTimes * timesInit)
{
	ResSystemTimes resTimes = {0};
	unsigned long long threadKernelTime = 0;
	unsigned long long threadUserTime = 0;

	if(!GetThreadTimes(
				threadHandle,
				(LPFILETIME)&cnt->abst.CreateTime,
				(LPFILETIME)&cnt->abst.ExitTime,
				(LPFILETIME)&threadKernelTime,
				(LPFILETIME)&threadUserTime ) ) {
		LOG_ERROR("GetThreadTimes() error %u", GetLastError());
		return;
	}

	MonUpdateTimes(threadKernelTime, threadUserTime, cnt, timesInit);

	cnt->abst.UserTime = threadUserTime;
	cnt->abst.KernelTime = threadKernelTime;
}

//------------------------------------------------------------------------------
// OsGetProcessPerfomance
//------------------------------------------------------------------------------
extern void OsGetProcessPerfomance(
				__inout ResObjectCounters * cnt,
				__in const ResSystemTimes * timesInit )
{
	ResSystemTimes resTimes = {0};
	unsigned long long totalTime = 0;
	unsigned long long processKrnlTime = 0;
	unsigned long long processUserTime = 0;
	IO_COUNTERS ioCounters = {0};

	if( GetProcessIoCounters( GetCurrentProcess(), &ioCounters ) ) {
		cnt->diskUsed =(unsigned long long)ioCounters.WriteTransferCount;
	} else {
		LOG_ERROR("GetProcessIoCounters() error %u\n", GetLastError());
	}
			
	if(!GetProcessTimes(
				GetCurrentProcess(),
				(LPFILETIME)&cnt->abst.CreateTime,
				(LPFILETIME)&cnt->abst.ExitTime,
				(LPFILETIME)&processKrnlTime,
				(LPFILETIME)&processUserTime ) ) {
		LOG_ERROR("GetProcessTimes() error %u\n", GetLastError());
		return;
	}

	MonUpdateTimes(processKrnlTime, processUserTime, cnt, timesInit);

	cnt->abst.UserTime = processUserTime;
	cnt->abst.KernelTime = processKrnlTime;
}
