#ifndef _KERNEL_MODE
#error This code only for kenel mode
#endif
#include <common/dbglog.h>
#include <common/checkptr.h>
#include <modedep/osdep.h>
#include <resmon/resmon.h>

#define TAG 'emiT'
//------------------------------------------------------------------------------
// OsGetSystemTimes
//------------------------------------------------------------------------------
extern signed OsGetSystemTimes(ResSystemTimes * resTimes)
{
	SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION * sppi = 0;
	ULONG returnLength = 0, size = sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * KeNumberProcessors;
	NTSTATUS status = STATUS_UNSUCCESSFUL;

	sppi = ExAllocatePoolWithTag(NonPagedPool, size, TAG);
	if( !sppi )
		return FALSE;
	status = ZwQuerySystemInformation(
			SystemProcessorPerformanceInformation,
			sppi,
			size,
			&returnLength);
	if( NT_SUCCESS(status) ) {
		ULONG i = 0;

		ASSERT( KPTR(resTimes) );

		RtlZeroMemory(resTimes, sizeof(ResSystemTimes));
		// can`t use KeNumberProcessors - this is dynamic variable
		for( i = 0; i < returnLength/sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION); i++) {
			resTimes->idleTime += sppi[i].IdleTime.QuadPart;
			resTimes->kernelTime += sppi[i].KernelTime.QuadPart;
			resTimes->userTime += sppi[i].UserTime.QuadPart;
		}
	} else {
		LOG_ERROR("get SystemProcessorPerformanceInformation error 0x%08X", status);
	}
	ExFreePoolWithTag(sppi, TAG);
	return NT_SUCCESS(status) ? TRUE:FALSE;
}

//------------------------------------------------------------------------------
// OsGetThreadPerfomance
//------------------------------------------------------------------------------
extern void OsGetThreadPerfomance(
				__in void * thread,
				__inout ResObjectCounters * cnt,
				__in const ResSystemTimes * timesInit)
{
	KERNEL_USER_TIMES kutimes = {0};
	HANDLE threadHandle = 0;
	// TODO: call without hook
	NTSTATUS status = ObOpenObjectByPointer(
					thread,
					OBJ_KERNEL_HANDLE,
					NULL,
					THREAD_ALL_ACCESS,
					*PsThreadType,
					KernelMode,
					&threadHandle);
	if( !NT_SUCCESS(status) ) {
		LOG_ERROR("ObOpenObjectByPointer(%p) error 0x%08X", thread, status);
		return;
	}

	ASSERT( threadHandle != 0 );

	status = ZwQueryInformationThread(
				threadHandle,
				ThreadTimes,
				&kutimes,
				sizeof(kutimes), 0);
	ZwClose(threadHandle);
	if( NT_SUCCESS(status) ) {
		ASSERT( KPTR(cnt) );
		cnt->abst.CreateTime = kutimes.CreateTime.QuadPart;
		cnt->abst.ExitTime = kutimes.ExitTime.QuadPart;
		MonUpdateTimes(kutimes.KernelTime.QuadPart, kutimes.UserTime.QuadPart, cnt, timesInit);
		cnt->abst.UserTime = kutimes.UserTime.QuadPart;
		cnt->abst.KernelTime = kutimes.KernelTime.QuadPart;
	}
}

//------------------------------------------------------------------------------
// OsGetProcessPerfomance
//------------------------------------------------------------------------------
extern void OsGetProcessPerfomance(
				__inout ResObjectCounters * cnt,
				__in const ResSystemTimes * timesInit )
{
	KERNEL_USER_TIMES kutimes = {0};
	HANDLE processHandle = 0;
	IO_COUNTERS ioCounters = {0};
	// TODO: call without hook
	NTSTATUS status = ObOpenObjectByPointer(
					PsGetCurrentProcess(),
					OBJ_KERNEL_HANDLE,
					NULL,
					PROCESS_ALL_ACCESS,
					*PsProcessType,
					KernelMode,
					&processHandle);
	if( !NT_SUCCESS(status) ) {
		LOG_ERROR("ObOpenObjectByPointer(%p) error 0x%08X", \
			PsGetCurrentProcess(), status);
		return;
	}

	ASSERT( processHandle != 0 );

	status = ZwQueryInformationProcess(
				processHandle,
				ProcessIoCounters,
				&ioCounters,
				sizeof(ioCounters), 0);
	if( NT_SUCCESS(status) ) {
		cnt->diskUsed =(unsigned long long)ioCounters.WriteTransferCount;
	} else {
		LOG_ERROR("get ProcessIoCounters for %p error 0x%08X", \
			PsGetCurrentProcess(), status);
	}

	status = ZwQueryInformationProcess(
				processHandle,
				ProcessTimes,
				&kutimes,
				sizeof(kutimes), 0);
	ZwClose(processHandle);
	if( NT_SUCCESS(status) ) {
		ASSERT( KPTR(cnt) );
		cnt->abst.CreateTime = kutimes.CreateTime.QuadPart;
		cnt->abst.ExitTime = kutimes.ExitTime.QuadPart;
		MonUpdateTimes(kutimes.KernelTime.QuadPart, kutimes.UserTime.QuadPart, cnt, timesInit);
		cnt->abst.UserTime = kutimes.UserTime.QuadPart;
		cnt->abst.KernelTime = kutimes.KernelTime.QuadPart;
	} else {
		LOG_ERROR("get ProcessTimes for %p error 0x%08X", \
			PsGetCurrentProcess(), status);
	}
}
