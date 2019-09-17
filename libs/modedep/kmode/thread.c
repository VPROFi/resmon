#ifndef _KERNEL_MODE
#error This code only for kenel mode
#endif

#include <common/dbglog.h>
#include <common/checkptr.h>
#include <modedep/osdep.h>
//------------------------------------------------------------------------------
// OsGetCurrentThreadId
//------------------------------------------------------------------------------
extern void * OsGetCurrentThreadId(void)
{
	return (void *)PsGetCurrentThreadId();
}
//------------------------------------------------------------------------------
// OsCreateThread
//------------------------------------------------------------------------------
extern void * OsCreateThread(
							void * startAddress,
							void * parameter,
							unsigned long creationFlags,
							void ** threadId)
{

	NTSTATUS status = STATUS_SUCCESS;
	OBJECT_ATTRIBUTES oa = {0};
	HANDLE threadHandle = NULL;
	CLIENT_ID clientId = {0};
	PKTHREAD thread = NULL;

	ASSERT( KPTR(startAddress) );

	UNREFERENCED_PARAMETER(creationFlags);

	InitializeObjectAttributes(&oa, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);

	status = PsCreateSystemThread(
				&threadHandle,
				THREAD_ALL_ACCESS,
				&oa,
				NULL,
				&clientId,
				#pragma warning( suppress:4055 )
				(PKSTART_ROUTINE)startAddress,
				parameter);
	if( !NT_SUCCESS(status) ) {
		LOG_ERROR("PsCreateSystemThread(): error 0x%08X", status);
		return NULL;
	}

	status = ObReferenceObjectByHandle(
					threadHandle,
					THREAD_ALL_ACCESS,
					*PsThreadType,
					KernelMode,
					&thread,
					NULL);
	ZwClose(threadHandle);
	if( !NT_SUCCESS(status) ) {
		LOG_ERROR("ObReferenceObjectByHandle(): error 0x%08X", status);
		return NULL;
	}

	if( KPTR(threadId) )
		*threadId = clientId.UniqueThread;

	return (void *)thread;
}

//------------------------------------------------------------------------------
// OsCloseThread
//------------------------------------------------------------------------------
extern void OsCloseThread(void * object)
{
	ASSERT( KPTR(object) );	
	ObDereferenceObject(object);
	return;
}

//------------------------------------------------------------------------------
// OsExitThread
//------------------------------------------------------------------------------
extern long OsExitThread(long status)
{
	return PsTerminateSystemThread(status);
}

//------------------------------------------------------------------------------
// OsOpenCurrentThread
//------------------------------------------------------------------------------
extern void * OsOpenCurrentThread(void)
{
	return (void *)0;
}

#define ThreadSystemThreadInformation 0x28
//------------------------------------------------------------------------------
// OsGetThreadStartAddress
//------------------------------------------------------------------------------
extern void * OsGetThreadStartAddress(void * thread)
{
	SYSTEM_THREAD_INFORMATION sti = {0};
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
		return _ReturnAddress();
	}
	status = ZwQueryInformationThread(
				threadHandle,
				ThreadSystemThreadInformation,
				&sti, sizeof(sti), 0);
	ZwClose(threadHandle);
	if( NT_SUCCESS(status) ) {
		return sti.StartAddress;
	} else {
		LOG_ERROR("get ThreadSystemThreadInformation for %p error 0x%08X", \
			thread, status);
	}
	return _ReturnAddress();
}

//------------------------------------------------------------------------------
// OsGetThreadId
//------------------------------------------------------------------------------
extern void * OsGetThreadId(void * thread)
{
	return (void *)PsGetThreadId(thread);
}
