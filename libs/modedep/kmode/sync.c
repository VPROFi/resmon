#ifndef _KERNEL_MODE
#error This code only for kenel mode
#endif

#define TAG 'cnyS'

#include <common/dbglog.h>
#include <common/checkptr.h>
//------------------------------------------------------------------------------
// GetSetLastError
//------------------------------------------------------------------------------
static unsigned long GetSetLastError(signed set, signed long status)
{
	static unsigned long lastError = 0;
	if(!set)
		return lastError;
	lastError = (unsigned long)status;
	return lastError;
}
//------------------------------------------------------------------------------
// OsGetLastError
//------------------------------------------------------------------------------
extern unsigned long OsGetLastError(void)
{
	return GetSetLastError(FALSE, 0);
}
//------------------------------------------------------------------------------
// OsWaitForSingleObject
//------------------------------------------------------------------------------
extern unsigned long OsWaitForSingleObject(
									void * object,
									long timeMs)
{
	NTSTATUS status = STATUS_SUCCESS;
	LARGE_INTEGER timeout = {0};

	ASSERT( KPTR(object) );

	timeout.QuadPart = (LONGLONG) - (timeMs * 1000 * 10);

	status = KeWaitForSingleObject(
				object,
				Executive,
				KernelMode,
				FALSE,
				(timeMs == OS_INFINITE) ? NULL:&timeout);

	if(status == STATUS_SUCCESS)
		return OsWaitObject0;
	if(status == STATUS_TIMEOUT)
		return OsWaitTimeout;
	GetSetLastError(TRUE, status);
	return (unsigned long)OsWaitFailed;
}
//------------------------------------------------------------------------------
// OsAcquireResource
//------------------------------------------------------------------------------
extern void OsAcquireResource(void * syncObject)
{
	ASSERT( KPTR(syncObject) );

	ASSERT( ExIsResourceAcquiredExclusiveLite(syncObject) || \
				ExIsResourceAcquiredSharedLite(syncObject) == 0 );

	KeEnterCriticalRegion();
	ExAcquireResourceExclusiveLite(syncObject, TRUE);
	return;
}
//------------------------------------------------------------------------------
// OsReleaseResource
//------------------------------------------------------------------------------
extern void OsReleaseResource(void * syncObject)
{
	ASSERT( KPTR(syncObject) );
	ASSERT( ExIsResourceAcquiredExclusiveLite(syncObject) );

	ExReleaseResourceLite(syncObject);
	KeLeaveCriticalRegion();
	return;
}
//------------------------------------------------------------------------------
// OsCreateResourceObject
//------------------------------------------------------------------------------
extern void * OsCreateResourceObject(void)
{
	NTSTATUS status = STATUS_SUCCESS;
	ERESOURCE * resource = NULL;

	resource = ExAllocatePoolWithTag(NonPagedPool, sizeof(ERESOURCE), TAG);
	if( !resource ) {
		LOG_ERROR("can`t allocate %u bytes", sizeof(ERESOURCE));
		return NULL;
	}
	status = ExInitializeResourceLite(resource);
	if( !NT_SUCCESS(status) ) {
		LOG_ERROR("ExInitializeResourceLite(): error 0x%08X", status);
		ExFreePoolWithTag(resource, TAG);
		return NULL;
	}
	return resource;
}
//------------------------------------------------------------------------------
// OsDeleteResourceObject
//------------------------------------------------------------------------------
extern void OsDeleteResourceObject(void * syncObject)
{
	NTSTATUS status = STATUS_SUCCESS;

	ASSERT( KPTR(syncObject) );
	ASSERT( !ExIsResourceAcquiredExclusiveLite(syncObject) );
	ASSERT( ExIsResourceAcquiredSharedLite(syncObject) == 0 );

	status = ExDeleteResourceLite(syncObject);
	if( !NT_SUCCESS(status) ) {
		LOG_ERROR("ExDeleteResourceLite(): error 0x%08X", status);
		return;
	}
	ExFreePoolWithTag(syncObject, TAG);
	return;
}

//------------------------------------------------------------------------------
// OsCreateEvent
//------------------------------------------------------------------------------
extern void * OsCreateEvent(
			unsigned long manualReset,
			unsigned long initialState)
{
	KEVENT * event = NULL;
	event = ExAllocatePoolWithTag(NonPagedPool, sizeof(KEVENT), TAG);
	if( !event ) {
		LOG_ERROR("can`t allocate %u bytes", sizeof(KEVENT));
		return NULL;
	}

	KeInitializeEvent(
		event,
		manualReset ? NotificationEvent:SynchronizationEvent,
		(BOOLEAN)initialState);

	return event;
}
//------------------------------------------------------------------------------
// OsSetEvent
//------------------------------------------------------------------------------
extern void OsSetEvent(void * event)
{
	ASSERT( KPTR(event) );
	KeSetEvent(event, IO_NO_INCREMENT, FALSE);
	return;
}
//------------------------------------------------------------------------------
// OsResetvent
//------------------------------------------------------------------------------
extern void OsResetEvent(void * event)
{
	ASSERT( KPTR(event) );
	KeResetEvent(event);
	return;
}
//------------------------------------------------------------------------------
// OsCreateMutex
//------------------------------------------------------------------------------
extern void * OsCreateMutex(
			unsigned long initialOwner)
{
	KMUTEX * mutex = NULL;

	mutex = ExAllocatePoolWithTag(NonPagedPool, sizeof(KMUTEX), TAG);
	if( !mutex ) {
		LOG_ERROR("can`t allocate %u bytes", sizeof(KMUTEX));
		return NULL;
	}

	KeInitializeMutex(mutex, 0);

	if(initialOwner)
		OsWaitForSingleObject(mutex, OS_INFINITE);

	return mutex;
}
//------------------------------------------------------------------------------
// OsReleaseMutex
//------------------------------------------------------------------------------
extern void OsReleaseMutex(void * mutex)
{
	ASSERT( KPTR(mutex) );
	KeReleaseMutex(mutex, FALSE);
	return;
}
//------------------------------------------------------------------------------
// OsCreateSemaphore
//------------------------------------------------------------------------------
extern void * OsCreateSemaphore(
			long initialCount,
			long maximumCount)
{
	KSEMAPHORE * semaphore = NULL;
	semaphore = ExAllocatePoolWithTag(NonPagedPool, sizeof(KSEMAPHORE), TAG);
	if( !semaphore ) {
		LOG_ERROR("can`t allocate %u bytes", sizeof(KSEMAPHORE));
		return NULL;
	}

	KeInitializeSemaphore(semaphore, initialCount, maximumCount);
	return semaphore;
}
//------------------------------------------------------------------------------
// OsReleaseSemaphore
//------------------------------------------------------------------------------
extern signed OsReleaseSemaphore(
							void * object,
							long releaseCount,
							long * previousCount)
{
	signed res = TRUE;

	ASSERT( KPTR(object) );
	ASSERT( KPTR(previousCount) || previousCount == NULL );

	__try {

		if( KPTR(previousCount) )
			*previousCount = ((DISPATCHER_HEADER *)object)->SignalState;

		KeReleaseSemaphore(object, IO_NO_INCREMENT, releaseCount, FALSE);

	#pragma warning(suppress: 6320 )
	} __except(EXCEPTION_EXECUTE_HANDLER) {
		// 
		if( GetExceptionCode() == STATUS_SEMAPHORE_LIMIT_EXCEEDED )
			GetSetLastError(TRUE, OS_ERROR_TOO_MANY_POSTS);
		else
			GetSetLastError(TRUE, GetExceptionCode());
		LOG_ERROR("KeReleaseSemaphore() ... exception 0x%08X", GetExceptionCode());
		res = FALSE;
	}
	return res;
}
//------------------------------------------------------------------------------
// OsClose
//------------------------------------------------------------------------------
extern void OsClose(void * object)
{
	ASSERT( KPTR(object) );	
	ExFreePoolWithTag(object, TAG);
	return;
}
