#ifdef _KERNEL_MODE
#error This code only for user mode
#endif
#include <common/dbglog.h>
#include <common/checkptr.h>
#include <modedep/osdep.h>
//------------------------------------------------------------------------------
// OsGetCurrentThreadId
//------------------------------------------------------------------------------
extern void * OsGetCurrentThreadId(void)
{
	return (void *)GetCurrentThreadId();
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
	void * threadHandle = NULL;
	DWORD newThreadId = 0;

	ASSERT( UPTR(startAddress) );

	threadHandle = CreateThread(
						NULL,
						0,
						#pragma warning( suppress:4055 )
						(LPTHREAD_START_ROUTINE)startAddress,
						parameter,
						creationFlags,
						&newThreadId);
	if( !threadHandle ) {
		LOG_ERROR("CreateThread(): error %u", GetLastError());
		return NULL;
	}
	if( UPTR(threadId) )
		*threadId = (void *)(ULONG_PTR)newThreadId;
	return threadHandle;
}

//------------------------------------------------------------------------------
// OsCloseThread
//------------------------------------------------------------------------------
extern void OsCloseThread(void * object)
{
	ASSERT( object != NULL );
	CloseHandle(object);
	return;
}
//------------------------------------------------------------------------------
// OsExitThread
//------------------------------------------------------------------------------
extern long OsExitThread(long status)
{
	return status;
}

//------------------------------------------------------------------------------
// OsOpenCurrentThread
//------------------------------------------------------------------------------
extern void * OsOpenCurrentThread(void)
{
	return OpenThread(THREAD_ALL_ACCESS, FALSE, GetCurrentThreadId());
}

//------------------------------------------------------------------------------
// OsGetThreadStartAddress
//------------------------------------------------------------------------------
extern void * OsGetThreadStartAddress(void * thread)
{
	void * adr = _ReturnAddress();
	NTSTATUS status = NtQueryInformationThread(
							thread,
							ThreadQuerySetWin32StartAddress,
							&adr, sizeof(void *), 0);
	
	if( !NT_SUCCESS(status) ) {
		LOG_ERROR("NtQueryInformationThread(%p) error 0x%08X", thread, status);
	}
	return adr;
}

//------------------------------------------------------------------------------
// OsGetThreadId
//------------------------------------------------------------------------------
extern void * OsGetThreadId(void * thread)
{
	return (void *)GetThreadId(thread);
}
