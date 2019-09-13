#ifdef _KERNEL_MODE
#error This code only for user mode
#endif
#include <common/dbglog.h>
#include <common/checkptr.h>
#include <tchar.h>

#define DEFAULT_CRITICAL_SECTION_SPIN_COUNT  4000 // like heap engenering
// CRITICAL_SECTION_SPIN_COUNT

//------------------------------------------------------------------------------
// OsWaitForSingleObject
//------------------------------------------------------------------------------
extern unsigned long OsWaitForSingleObject(
									void * object,
									long timeMs)
{
	ASSERT( object != NULL );
	return WaitForSingleObject(object, (timeMs==OS_INFINITE) ? INFINITE:timeMs);
}
//------------------------------------------------------------------------------
// OsGetLastError
//------------------------------------------------------------------------------
extern unsigned long OsGetLastError( void )
{
	return GetLastError();
}
//------------------------------------------------------------------------------
// OsAcquireResource
//------------------------------------------------------------------------------
extern void OsAcquireResource(void * syncObject)
{
	ASSERT( UPTR(syncObject) );
	EnterCriticalSection((CRITICAL_SECTION *)syncObject);
	return;
}

//------------------------------------------------------------------------------
// OsReleaseResource
//------------------------------------------------------------------------------
extern void OsReleaseResource(void * syncObject)
{
	ASSERT( UPTR(syncObject) );
	LeaveCriticalSection((CRITICAL_SECTION *)syncObject);
	return;
}

//------------------------------------------------------------------------------
// OsCreateResourceObject
//------------------------------------------------------------------------------
extern void * OsCreateResourceObject(void)
{
	CRITICAL_SECTION * resource = NULL;
	resource = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CRITICAL_SECTION));
	if( !resource ) {
		LOG_ERROR("can`t allocate %u bytes", sizeof(CRITICAL_SECTION));
		return NULL;
	}

	if( !InitializeCriticalSectionAndSpinCount(
		(CRITICAL_SECTION *)resource,
		DEFAULT_CRITICAL_SECTION_SPIN_COUNT ) ) {
			LOG_ERROR("InitializeCriticalSectionAndSpinCount(): " \
								"error %u",	GetLastError());
		HeapFree(GetProcessHeap(), 0, resource);
		resource = NULL;
	}

	return resource;
}

//------------------------------------------------------------------------------
// OsDeleteResourceObject
//------------------------------------------------------------------------------
extern void OsDeleteResourceObject(void * syncObject)
{
	ASSERT( UPTR(syncObject) );
	ASSERT( ((PRTL_CRITICAL_SECTION)syncObject)->RecursionCount == 0 );

	DeleteCriticalSection(syncObject);
	HeapFree(GetProcessHeap(), 0, syncObject);
	return;
}

//------------------------------------------------------------------------------
// OsCreateEvent
//------------------------------------------------------------------------------
extern void * OsCreateEvent(
			unsigned long manualReset,
			unsigned long initialState)
{
	HANDLE handle = NULL;

	handle = CreateEventW(NULL, (BOOL)manualReset, (BOOL)initialState, 0);
	if( !handle ) {
		LOG_ERROR("CreateEventW(): error %u\n", GetLastError());
	}

	return handle;
}

//------------------------------------------------------------------------------
// OsSetEvent
//------------------------------------------------------------------------------
extern void OsSetEvent(void * event)
{
	ASSERT( event != NULL );
	SetEvent(event);
	return;
}
//------------------------------------------------------------------------------
// OsResetvent
//------------------------------------------------------------------------------
extern void OsResetEvent(void * event)
{
	ASSERT( event != NULL );
	ResetEvent(event);
	return;
}
//------------------------------------------------------------------------------
// OsCreateMutex
//------------------------------------------------------------------------------
extern void * OsCreateMutex(
			unsigned long initialOwner)
{
	HANDLE mutex = NULL;
	mutex = CreateMutexW(NULL, (BOOL)initialOwner, 0);
	if(!mutex) {
		LOG_ERROR("CreateMutexW(): error %u\n", GetLastError());
	}
	return mutex;
}
//------------------------------------------------------------------------------
// OsReleaseMutex
//------------------------------------------------------------------------------
extern void OsReleaseMutex(void * mutex)
{
	ASSERT( mutex != NULL );
	if(!ReleaseMutex(mutex)) {
		LOG_ERROR("ReleaseMutex(): error %u", GetLastError());
	}
	return;
}
//------------------------------------------------------------------------------
// OsCreateSemaphore
//------------------------------------------------------------------------------
extern void * OsCreateSemaphore(
			long initialCount,
			long maximumCount)
{
	HANDLE semaphore = NULL;
	semaphore = CreateSemaphoreW(NULL, initialCount, maximumCount, 0);
	if(!semaphore) {
		LOG_ERROR("CreateSemaphoreW(): error %u\n", GetLastError());
	}
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
	signed res = FALSE;
	res = ReleaseSemaphore(object, releaseCount, previousCount);
	if(!res) {
		LOG_ERROR("ReleaseSemaphore() error %u", GetLastError());
	}
	return res;
}

//------------------------------------------------------------------------------
// OsClose
//------------------------------------------------------------------------------
extern void OsClose(void * object)
{
	ASSERT( object != NULL );
	CloseHandle(object);
	return;
}
