#ifdef _KERNEL_MODE
#error This code only for user mode
#endif
#include <common/dbglog.h>
#include <common/checkptr.h>
#include <modedep/modedep.h>
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
