#ifndef __MODEDEP_H__
#define __MODEDEP_H__

//------------------------------------------------------------------------------
// resmon types
//------------------------------------------------------------------------------
#include <common/types.h>
#include <resmon/resmon.h>

//------------------------------------------------------------------------------
// base api
//------------------------------------------------------------------------------
#ifdef _KERNEL_MODE

#include <ntifs.h>
#pragma comment(lib, "ntoskrnl.lib")

#else

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#include <windows.h>

#pragma warning(push)
#pragma warning(disable: 4005)
#include <ntstatus.h>
#pragma warning(pop)

#define NTSTATUS LONG
#define NT_SUCCESS(x) ((x) >= 0)

#pragma comment(lib, "ntdll.lib")
EXTERN_C void WINAPI RtlAssert(
    __in PVOID FailedAssertion,
    __in PVOID FileName,
    __in ULONG LineNumber,
    __in_opt PSTR Message);

EXTERN_C signed __cdecl DbgPrint(const char * msg, ...);

#ifdef NDEBUG

	#define ASSERT( exp ) ((void)0)

#else

	#define ASSERT( exp ) \
    	((!(exp)) ? \
        	(RtlAssert(#exp, __FILE__, __LINE__, NULL ),FALSE) : \
	        TRUE)
#endif // NDEBUG

#endif // _KERNEL_MODE

#ifdef __cplusplus
extern "C"
{
#endif

//------------------------------------------------------------------------------
// hooks
//------------------------------------------------------------------------------
typedef struct {
	void * osapi;
	void * osapiptr;
	const char * apiname;
	void * apihook;
} Method;
Method * GetMonitorMethods(unsigned long * totalMethods);
typedef struct {
	void * base;
	unsigned long imageSize;
	UniStr basename;
} OsModuleData;
signed OsBaseAndDataFromPointer(void *ptr, OsModuleData * mod);
void OsFreeModuleData(OsModuleData * mod);
//------------------------------------------------------------------------------
// perfomance api
//------------------------------------------------------------------------------
signed OsGetSystemTimes(ResSystemTimes * resTimes);
void OsGetThreadPerfomance(
		void * threadHandle,
		ResObjectCounters * cnt,
		const ResSystemTimes * resTimesInit);
void OsGetProcessPerfomance(
				ResObjectCounters * cnt,
				const ResSystemTimes * timesInit);
//------------------------------------------------------------------------------
// thread process api
//------------------------------------------------------------------------------
void * OsGetCurrentThreadId(void);
void * OsGetCurrentProcessId(void);
void * OsCreateThread(
							void * startAddress,
							void * parameter,
							unsigned long creationFlags,
							void ** threadId);
void OsCloseThread(void * object);
long OsExitThread(long status);

unsigned long OsGetLastError( void );
//------------------------------------------------------------------------------
// memory api
//------------------------------------------------------------------------------
signed GetMemoryAccess(void * ptr, void ** rwptr, void ** ctx);
signed RestoreMemoryAccess(void * ctx);
void * OsAllocObject(size_tr size);
void OsFreeObject(void * obj);
const char * GetMemoryTypeName(unsigned long mtype);
void OsMoveMemory(void * dst, void *src, size_tr size);
//------------------------------------------------------------------------------
// sync api
//------------------------------------------------------------------------------
#define OS_INFINITE 0xFFFFFFFF
#define OS_MAXIMUM_LIMIT_VALUE	2147483647L
#define OS_ERROR_TOO_MANY_POSTS             298L
#define OS_ERROR_NOT_READY                  21L
typedef enum {
	OsWaitObject0,
	OsWaitObject1,
//.............
	OsWaitTimeout = 0x00000102L,
	OsWaitFailed = 0xFFFFFFFFL
} OsWaitStatus;
unsigned long OsWaitForSingleObject(
									void * object,
									long timeMs);
void OsAcquireResource(void * syncObject);
void OsReleaseResource(void * syncObject);
void * OsCreateResourceObject(void);
void OsDeleteResourceObject(void * syncObject);
void * OsCreateEvent(
			unsigned long manualReset,
			unsigned long initialState);
void OsSetEvent(void * event);
void OsResetEvent(void * event);
void * OsCreateMutex(
			unsigned long initialOwner);
void OsReleaseMutex(void * mutex);
void * OsCreateSemaphore(
			long initialCount,
			long maximumCount);
signed OsReleaseSemaphore(
							void * object,
							long releaseCount,
							long * previousCount);
void OsClose(void * object);

#ifdef __cplusplus
}
#endif

#endif // __MODEDEP_H__
