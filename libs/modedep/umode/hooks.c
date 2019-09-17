#ifdef _KERNEL_MODE
#error This code only for user mode
#endif
#include <common/dbglog.h>
#include <common/checkptr.h>
#include <modedep/osdep.h>
#include <resmon/resmonlib.h>
#include <tchar.h>
//------------------------------------------------------------------------------
// HookApiIndex enum
//------------------------------------------------------------------------------
typedef enum {
	iHeapAlloc,
	iHeapFree,
	iHeapDestroy,
	iCreateThread,
	iOpenThread,
	iCloseHandle,
	iCreateEventW,
	iCreateEventA,
	iOpenEventW,
	iOpenEventA,
	iHeapReAlloc,
	iVirtualAlloc,
	iVirtualAllocEx,
	iVirtualAllocExNuma,
	iVirtualFree,
	iVirtualFreeEx,
	iCreateSemaphoreA,
	iCreateSemaphoreW,
	iOpenSemaphoreA,
	iOpenSemaphoreW,
	iCreateMutexA,
	iCreateMutexW,
	iOpenMutexA,
	iOpenMutexW,
	iExitProcess,
	iTerminateProcess,
	iMaxKernel32,
	iexit = iMaxKernel32,
	i_exit,
	i_amsg_exit,
	i_c_exit,
	i_cexit,
	imalloc,
	ifree,
	irealloc,
	icalloc,
	iMax
} HookApiIndex;

// memory types
typedef enum {
	HeapMemType = 0x1000,
	VirtualMemType,
	CallocMemType,
	MallocMemType,
	LastMemType
} MemoryType;

C_ASSERT( HeapMemType >= 0x1000 );
//------------------------------------------------------------------------------
// GetMemoryTypeName
//------------------------------------------------------------------------------
extern const char * GetMemoryTypeName(unsigned long mtype)
{
	switch(mtype) {
		case HeapMemType:
			return (const char *)"Heap";
		case VirtualMemType:
			return (const char *)"Virtual";
		case CallocMemType:
			return (const char *)"Calloc";
		case MallocMemType:
			return (const char *)"Malloc";
	}
	return (const char *)"Unknown";
}

//------------------------------------------------------------------------------
// OsAllocObject
//------------------------------------------------------------------------------
extern void * OsAllocObject(size_tr size)
{
	Method * m = GetMonitorMethods(0);
	void * obj = ((void * (WINAPI *)(HANDLE, DWORD, SIZE_T)) \
		m[iHeapAlloc].osapi)(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
	return obj;
}

//------------------------------------------------------------------------------
// OsFreeObject
//------------------------------------------------------------------------------
extern void OsFreeObject(void * obj)
{
	Method * m = GetMonitorMethods(0);
	((BOOL (WINAPI *)(HANDLE, DWORD, LPVOID)) \
			m[iHeapFree].osapi)(GetProcessHeap(), 0, obj);
	return;
}

//------------------------------------------------------------------------------
// HeapAllocHook
//------------------------------------------------------------------------------
static __bcount(dwBytes) LPVOID	WINAPI HeapAllocHook(
		__in HANDLE hHeap,
		__in DWORD dwFlags,
		__in SIZE_T dwBytes)
{
	Method * m = GetMonitorMethods(0);
	void * ptr = 0;

	ASSERT( UPTR(m[iHeapAlloc].osapi) );
	ptr = ((void * (WINAPI *)(HANDLE, DWORD, SIZE_T)) \
		m[iHeapAlloc].osapi)(hHeap, dwFlags, dwBytes);

	if( ptr )
		MonAlloc(ptr, _ReturnAddress(), dwBytes, (ptr_t)hHeap, HeapMemType);
	return ptr;
}
//------------------------------------------------------------------------------
// HeapFreeHook
//------------------------------------------------------------------------------
static BOOL WINAPI HeapFreeHook(
		__inout HANDLE hHeap,
		__in    DWORD dwFlags,
		__drv_freesMem(Mem) __post __notvalid __deref LPVOID lpMem)
{
	Method * m = GetMonitorMethods(0);

	MonFree(lpMem);

	ASSERT( UPTR(m[iHeapFree].osapi) );
	return ((BOOL (WINAPI *)(HANDLE, DWORD, LPVOID)) \
			m[iHeapFree].osapi)(hHeap, dwFlags, lpMem);
}
//------------------------------------------------------------------------------
// CreateThreadHook
//------------------------------------------------------------------------------
static HANDLE WINAPI CreateThreadHook(
		__in_opt  LPSECURITY_ATTRIBUTES lpThreadAttributes,
		__in      SIZE_T dwStackSize,
		__in      LPTHREAD_START_ROUTINE lpStartAddress,
		__in_opt __deref __drv_aliasesMem LPVOID lpParameter,
		__in      DWORD dwCreationFlags,
		__out_opt LPDWORD lpThreadId)
{
	Method * m = GetMonitorMethods(0);
	void * ctx = MonCreateThreadBegin();
	DWORD threadId = 0;
	HANDLE threadHandle = ((HANDLE (WINAPI *)(LPSECURITY_ATTRIBUTES,SIZE_T,\
							LPTHREAD_START_ROUTINE, LPVOID, DWORD, LPDWORD)) \
			m[iCreateThread].osapi)(lpThreadAttributes,
									dwStackSize,
									lpStartAddress,
									lpParameter,
									dwCreationFlags,
									&threadId);

	if( threadHandle && UPTR(lpThreadId) ) {
		__try {
			*lpThreadId = threadId;
		} __except(EXCEPTION_EXECUTE_HANDLER) {
			LOG_ERROR("lpThreadId parametr error");
		}
	}

	if( UPTR(ctx) )
		MonCreateThreadEnd(	ctx,
						threadHandle,
						_ReturnAddress(),
						lpStartAddress,
						(void *)threadId);
	return threadHandle;
}

//------------------------------------------------------------------------------
// CloseHandleHook
//------------------------------------------------------------------------------
static BOOL WINAPI CloseHandleHook(__in HANDLE hObject)
{
	Method * m = GetMonitorMethods(0);
	BOOL res = ((BOOL (WINAPI *)(HANDLE))m[iCloseHandle].osapi)(hObject);
	if( res )
		MonClose(hObject);
	return res;
}

//------------------------------------------------------------------------------
// CreateOpenSyncCommon
//------------------------------------------------------------------------------
static HANDLE WINAPI CreateOpenSyncCommon(
	__in HookApiIndex index,
	__in void * returnAddress,
    __in_opt LPSECURITY_ATTRIBUTES lpAttributes,
    __in LONG parametr1,
    __in LONG parametr2,
    __in DWORD dwDesiredAccess,
    __in BOOL bInheritHandle,
    __in_opt void * lpName)
{
	Method * m = GetMonitorMethods(0);
	void * ctx = MonCreateSyncBegin();
	HANDLE handle = 0;
	SyncType syncType = EventSync;

	if( index == iCreateSemaphoreA || index == iCreateSemaphoreW ||
		 index == iOpenSemaphoreA || index == iOpenSemaphoreW )
		syncType = SemaphoreSync;

	if( index == iOpenMutexA || index == iOpenMutexW )
		syncType = MutexSync;

	if( index == iCreateEventW || index == iCreateEventA || \
		index == iCreateSemaphoreA || index == iCreateSemaphoreW )
		handle = ((HANDLE (WINAPI *)(LPSECURITY_ATTRIBUTES,LONG,\
							LONG, void *))m[index].osapi)(lpAttributes,
									parametr1,
									parametr2,
									lpName);
	else if ( index == iOpenEventW || index == iOpenEventA || \
			index == iOpenSemaphoreA || index == iOpenSemaphoreW || \
			index == iOpenMutexA || index == iOpenMutexW)
		handle = ((HANDLE (WINAPI *)(DWORD,BOOL,void *)) \
						m[index].osapi)(dwDesiredAccess,
									bInheritHandle,
									lpName);
	if( UPTR(ctx) )
		MonCreateSyncEnd(
					ctx,
					handle,
					syncType,
					returnAddress,
					parametr1,
					parametr2);
	return handle;
}
//------------------------------------------------------------------------------
// CreateEventAHook
//------------------------------------------------------------------------------
static HANDLE WINAPI CreateEventAHook(
    __in_opt LPSECURITY_ATTRIBUTES lpEventAttributes,
    __in     BOOL bManualReset,
    __in     BOOL bInitialState,
    __in_opt LPCSTR lpName)
{
	return CreateOpenSyncCommon(
				iCreateEventA,
				_ReturnAddress(),
				lpEventAttributes,
				bManualReset,
				bInitialState,
				0, 0,
				(void *)lpName);
}

//------------------------------------------------------------------------------
// CreateEventWHook
//------------------------------------------------------------------------------
static HANDLE WINAPI CreateEventWHook(
    __in_opt LPSECURITY_ATTRIBUTES lpEventAttributes,
    __in     BOOL bManualReset,
    __in     BOOL bInitialState,
    __in_opt LPCWSTR lpName)
{
	return CreateOpenSyncCommon(
				iCreateEventW,
				_ReturnAddress(),
				lpEventAttributes,
				bManualReset,
				bInitialState,
				0, 0,
				(void *)lpName);
}

//------------------------------------------------------------------------------
// OpenEventAHook
//------------------------------------------------------------------------------
static HANDLE WINAPI OpenEventAHook(
    _In_ DWORD dwDesiredAccess,
    _In_ BOOL bInheritHandle,
    _In_ LPCSTR lpName)
{
	return CreateOpenSyncCommon(
				iOpenEventA,
				_ReturnAddress(),
				0,	0,	0,
				dwDesiredAccess, bInheritHandle,
				(void *)lpName);
}

//------------------------------------------------------------------------------
// OpenEventWHook
//------------------------------------------------------------------------------
static HANDLE WINAPI OpenEventWHook(
    _In_ DWORD dwDesiredAccess,
    _In_ BOOL bInheritHandle,
    _In_ LPCWSTR lpName)
{
	return CreateOpenSyncCommon(
				iOpenEventW,
				_ReturnAddress(),
				0,	0,	0,
				dwDesiredAccess, bInheritHandle,
				(void *)lpName);
}

//------------------------------------------------------------------------------
// HeapReAllocHook
//------------------------------------------------------------------------------
static LPVOID WINAPI HeapReAllocHook(
    __inout HANDLE hHeap,
    __in    DWORD dwFlags,
    __deref LPVOID lpMem,
    __in    SIZE_T dwBytes)
{
	Method * m = GetMonitorMethods(0);
	void * ptr = 0;
	MonFree(lpMem);
	ASSERT( UPTR(m[iHeapReAlloc].osapi) );
	ptr = ((void * (WINAPI *)(HANDLE, DWORD, LPVOID, SIZE_T)) \
		m[iHeapReAlloc].osapi)(hHeap, dwFlags, lpMem, dwBytes);

	if( ptr )
		MonAlloc(ptr, _ReturnAddress(), dwBytes, (ptr_t)hHeap, HeapMemType);
	return ptr;
}

//------------------------------------------------------------------------------
// VirtualAllocCommon
//------------------------------------------------------------------------------
static void * VirtualAllocCommon(
	__in HookApiIndex index,
	__in void * returnAddress,
    __in     HANDLE hProcess,
    __in_opt LPVOID lpAddress,
    __in     SIZE_T dwSize,
    __in     DWORD  flAllocationType,
    __in     DWORD  flProtect,
    __in     DWORD  nndPreferred)
{
	Method * m = GetMonitorMethods(0);
	void * ptr = 0;

	ASSERT( index == iVirtualAllocExNuma || \
			index == iVirtualAllocEx || \
			index == iVirtualAlloc );

	switch( index ) {
		case iVirtualAllocExNuma:
			ptr = ((void * (WINAPI *)(HANDLE, LPVOID, SIZE_T, DWORD, DWORD, DWORD)) \
				m[iVirtualAllocExNuma].osapi)(
					hProcess,
					lpAddress,
					dwSize,
					flAllocationType,
					flProtect,
					nndPreferred);
			break;
		case iVirtualAllocEx:
			ptr = ((void * (WINAPI *)(HANDLE, LPVOID, SIZE_T, DWORD, DWORD)) \
				m[iVirtualAllocEx].osapi)(
					hProcess,
					lpAddress,
					dwSize,
					flAllocationType,
					flProtect);
			break;
		case iVirtualAlloc:
			ptr = ((void * (WINAPI *)(LPVOID, SIZE_T, DWORD, DWORD)) \
				m[iVirtualAlloc].osapi)(
					lpAddress,
					dwSize,
					flAllocationType,
					flProtect);
			break;
		default:
			LOG_ERROR("index error");
			break;
	}

	if( ptr )
		MonAlloc(ptr,
			returnAddress,
			dwSize,
			hProcess ? (ptr_t)GetProcessId(hProcess):(ptr_t)GetCurrentProcessId(),
			VirtualMemType);
	return ptr;
}
//------------------------------------------------------------------------------
// VirtualAllocHook
//------------------------------------------------------------------------------
static LPVOID WINAPI VirtualAllocHook(
    __in_opt LPVOID lpAddress,
    __in     SIZE_T dwSize,
    __in     DWORD flAllocationType,
    __in     DWORD flProtect)
{
	return VirtualAllocCommon(
		iVirtualAlloc,
		_ReturnAddress(),
		0,
		lpAddress,
		dwSize,
		flAllocationType,
		flProtect,
		0);
}

//------------------------------------------------------------------------------
// VirtualAllocExHook
//------------------------------------------------------------------------------
static LPVOID WINAPI VirtualAllocExHook(
    __in     HANDLE hProcess,
    __in_opt LPVOID lpAddress,
    __in     SIZE_T dwSize,
    __in     DWORD flAllocationType,
    __in     DWORD flProtect)
{
	return VirtualAllocCommon(
		iVirtualAllocEx,
		_ReturnAddress(),
		hProcess,
		lpAddress,
		dwSize,
		flAllocationType,
		flProtect,
		0);
}

//------------------------------------------------------------------------------
// VirtualAllocExNumaHook
//------------------------------------------------------------------------------
static LPVOID WINAPI VirtualAllocExNumaHook(
    __in     HANDLE hProcess,
    __in_opt LPVOID lpAddress,
    __in     SIZE_T dwSize,
    __in     DWORD flAllocationType,
    __in     DWORD flProtect,
    __in     DWORD nndPreferred)
{
	return VirtualAllocCommon(
		iVirtualAllocExNuma,
		_ReturnAddress(),
		hProcess,
		lpAddress,
		dwSize,
		flAllocationType,
		flProtect,
		nndPreferred);
}

//------------------------------------------------------------------------------
// VirtualFreeCommon
//------------------------------------------------------------------------------
static BOOL WINAPI VirtualFreeCommon(
	__in HookApiIndex index,
    __in HANDLE hProcess,
    __in LPVOID lpAddress,
    __in SIZE_T dwSize,
    __in DWORD dwFreeType)
{
	Method * m = GetMonitorMethods(0);
	BOOL res = 0;

	ASSERT( index == iVirtualFreeEx || \
			index == iVirtualFree );

	if( UPTR(lpAddress) && dwFreeType & MEM_RELEASE )
		MonFree(lpAddress);

	switch( index ) {
		case iVirtualFreeEx:
			res = ((BOOL (WINAPI *)(HANDLE, LPVOID, SIZE_T, DWORD)) \
				m[iVirtualFreeEx].osapi)(
					hProcess,
					lpAddress,
					dwSize,
					dwFreeType);
			break;
		case iVirtualFree:
			res = ((BOOL (WINAPI *)(LPVOID, SIZE_T, DWORD)) \
				m[iVirtualFree].osapi)(
					lpAddress,
					dwSize,
					dwFreeType);
			break;
		default:
			LOG_ERROR("index error");
			break;
	}

	return res;
}

//------------------------------------------------------------------------------
// VirtualFreeExHook
//------------------------------------------------------------------------------
static BOOL WINAPI VirtualFreeExHook(
    __in HANDLE hProcess,
    __in LPVOID lpAddress,
    __in SIZE_T dwSize,
    __in DWORD  dwFreeType)
{
	return VirtualFreeCommon(
		iVirtualFreeEx,
		hProcess,
		lpAddress,
		dwSize,
		dwFreeType);
}

//------------------------------------------------------------------------------
// VirtualFreeExHook
//------------------------------------------------------------------------------
static BOOL WINAPI VirtualFreeHook(
    __in LPVOID lpAddress,
    __in SIZE_T dwSize,
    __in DWORD  dwFreeType)
{
	return VirtualFreeCommon(
		iVirtualFree,
		0,
		lpAddress,
		dwSize,
		dwFreeType);
}

//------------------------------------------------------------------------------
// OpenThreadHook
//------------------------------------------------------------------------------
static HANDLE WINAPI OpenThreadHook(
    __in DWORD dwDesiredAccess,
    __in BOOL bInheritHandle,
    __in DWORD dwThreadId)
{
	Method * m = GetMonitorMethods(0);
	void * ctx = MonCreateThreadBegin();
	void * start = 0;
	HANDLE threadHandle = ((HANDLE (WINAPI *)(DWORD,BOOL,DWORD)) \
			m[iOpenThread].osapi)(dwDesiredAccess,
									bInheritHandle,
									dwThreadId);
	if( threadHandle && UPTR(ctx) )
		start = OsGetThreadStartAddress(threadHandle);

	if( UPTR(ctx) )
		MonCreateThreadEnd(
					ctx,
					threadHandle,
					_ReturnAddress(),
					UPTR(start) ? start:_ReturnAddress(),
					(void *)dwThreadId);

	return threadHandle;
}

//------------------------------------------------------------------------------
// HeapDestroyHook
//------------------------------------------------------------------------------
static BOOL WINAPI HeapDestroyHook(__in HANDLE hHeap)
{
	Method * m = GetMonitorMethods(0);
	BOOL res = ((BOOL (WINAPI *)(HANDLE)) \
			m[iHeapDestroy].osapi)(hHeap);
	if(res)
		MonFreeAllByTag((ptr_t)hHeap);

	return res;
}

//------------------------------------------------------------------------------
// CreateSemaphoreAHook
//------------------------------------------------------------------------------
static HANDLE WINAPI CreateSemaphoreAHook(
    __in_opt LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
    __in     LONG lInitialCount,
    __in     LONG lMaximumCount,
    __in_opt LPCSTR lpName)
{
	return CreateOpenSyncCommon(
				iCreateSemaphoreA,
				_ReturnAddress(),
				lpSemaphoreAttributes,
				lInitialCount,
				lMaximumCount,
				0, 0,
				(void *)lpName);
}
//------------------------------------------------------------------------------
// CreateSemaphoreWHook
//------------------------------------------------------------------------------
static HANDLE WINAPI CreateSemaphoreWHook(
    __in_opt LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
    __in     LONG lInitialCount,
    __in     LONG lMaximumCount,
    __in_opt LPCWSTR lpName)
{
	return CreateOpenSyncCommon(
				iCreateSemaphoreW,
				_ReturnAddress(),
				lpSemaphoreAttributes,
				lInitialCount,
				lMaximumCount,
				0, 0,
				(void *)lpName);
}

//------------------------------------------------------------------------------
// OpenSemaphoreAHook
//------------------------------------------------------------------------------
static HANDLE WINAPI OpenSemaphoreAHook(
    __in DWORD dwDesiredAccess,
    __in BOOL bInheritHandle,
    __in LPCSTR lpName)
{
	return CreateOpenSyncCommon(
				iOpenSemaphoreA,
				_ReturnAddress(),
				0,	0,	0,
				dwDesiredAccess, bInheritHandle,
				(void *)lpName);
}
//------------------------------------------------------------------------------
// OpenSemaphoreWHook
//------------------------------------------------------------------------------
static HANDLE WINAPI OpenSemaphoreWHook(
    __in DWORD dwDesiredAccess,
    __in BOOL bInheritHandle,
    __in LPCWSTR lpName)
{
	return CreateOpenSyncCommon(
				iOpenSemaphoreW,
				_ReturnAddress(),
				0,	0,	0,
				dwDesiredAccess, bInheritHandle,
				(void *)lpName);
}

//------------------------------------------------------------------------------
// CreateMutexCommon
//------------------------------------------------------------------------------
static HANDLE WINAPI CreateMutexCommon(
	__in HookApiIndex index,
	__in void * returnAddress,
    __in_opt LPSECURITY_ATTRIBUTES lpMutexAttributes,
    __in     BOOL bInitialOwner,
    __in_opt void * lpName)
{
	Method * m = GetMonitorMethods(0);
	void * ctx = MonCreateSyncBegin();
	HANDLE handle = 0;

	if( index == iCreateMutexW || index == iCreateMutexA )
		handle = ((HANDLE (WINAPI *)(LPSECURITY_ATTRIBUTES,BOOL,void *))\
					m[index].osapi)(lpMutexAttributes,
									bInitialOwner,
										lpName);
	if( UPTR(ctx) )
		MonCreateSyncEnd(
					ctx,
					handle,
					MutexSync,
					returnAddress,
					bInitialOwner,
					0);
	return handle;
}
//------------------------------------------------------------------------------
// CreateMutexAHook
//------------------------------------------------------------------------------
static HANDLE WINAPI CreateMutexAHook(
    __in_opt LPSECURITY_ATTRIBUTES lpMutexAttributes,
    __in     BOOL bInitialOwner,
    __in_opt LPCSTR lpName)
{
	return CreateMutexCommon(
			iCreateMutexA,
			_ReturnAddress(),
			lpMutexAttributes,
			bInitialOwner,
			(void *)lpName);
}
//------------------------------------------------------------------------------
// CreateMutexWHook
//------------------------------------------------------------------------------
static HANDLE WINAPI CreateMutexWHook(
    __in_opt LPSECURITY_ATTRIBUTES lpMutexAttributes,
    __in     BOOL bInitialOwner,
    __in_opt LPCWSTR lpName)
{
	return CreateMutexCommon(
			iCreateMutexW,
			_ReturnAddress(),
			lpMutexAttributes,
			bInitialOwner,
			(void *)lpName);
}
//------------------------------------------------------------------------------
// OpenMutexAHook
//------------------------------------------------------------------------------
static HANDLE WINAPI OpenMutexAHook(
    __in DWORD dwDesiredAccess,
    __in BOOL bInheritHandle,
    __in LPCSTR lpName)
{
	return CreateOpenSyncCommon(
				iOpenMutexA,
				_ReturnAddress(),
				0,	0,	0,
				dwDesiredAccess, bInheritHandle,
				(void *)lpName);

}
//------------------------------------------------------------------------------
// OpenMutexWHook
//------------------------------------------------------------------------------
static HANDLE WINAPI OpenMutexWHook(
    __in DWORD dwDesiredAccess,
    __in BOOL bInheritHandle,
    __in LPCSTR lpName)
{
	return CreateOpenSyncCommon(
				iOpenMutexW,
				_ReturnAddress(),
				0,	0,	0,
				dwDesiredAccess, bInheritHandle,
				(void *)lpName);

}

//------------------------------------------------------------------------------
// ExitProcessHook
//------------------------------------------------------------------------------
static VOID WINAPI ExitProcessHook(__in UINT uExitCode)
{
	Method * m = GetMonitorMethods(0);
	LOG_INFO("exit process [%u] %u", GetCurrentProcessId(), uExitCode);
	UninstallResMonitoringSystem();
	((VOID (WINAPI *)(UINT))\
		m[iExitProcess].osapi)(uExitCode);
}

//------------------------------------------------------------------------------
// TerminateProcess
//------------------------------------------------------------------------------
static BOOL WINAPI TerminateProcessHook(
    __in HANDLE hProcess,
    __in UINT uExitCode)
{
	Method * m = GetMonitorMethods(0);
	LOG_INFO("terminame process %p [%u] current [%u] %u", \
		hProcess, GetProcessId(hProcess), GetCurrentProcessId(), uExitCode);
	if( GetProcessId(hProcess) == GetCurrentProcessId() )
		UninstallResMonitoringSystem();
	return ((BOOL (WINAPI *)(HANDLE, UINT))\
		m[iTerminateProcess].osapi)(hProcess, uExitCode);
}

//------------------------------------------------------------------------------
// exitHook
//------------------------------------------------------------------------------
static void __cdecl exitHook(_In_ int _Code)
{
	Method * m = GetMonitorMethods(0);
	LOG_INFO("exit [%u] %i", GetCurrentProcessId(), _Code);
	UninstallResMonitoringSystem();
	((void (__cdecl *)(int))\
		m[iexit].osapi)(_Code);
}

//------------------------------------------------------------------------------
// _exitHook
//------------------------------------------------------------------------------
static void __cdecl _exitHook(_In_ int _Code)
{
	Method * m = GetMonitorMethods(0);
	LOG_INFO("_exit [%u] %i", GetCurrentProcessId(), _Code);
	UninstallResMonitoringSystem();
	((void (__cdecl *)(int))\
		m[i_exit].osapi)(_Code);
}

//------------------------------------------------------------------------------
// _amsg_exitHook
//------------------------------------------------------------------------------
static void __cdecl _amsg_exitHook(int rterrnum)
{
	Method * m = GetMonitorMethods(0);
	LOG_INFO("_amsg_exit [%u] %i", GetCurrentProcessId(), rterrnum);
	UninstallResMonitoringSystem();
	((void (__cdecl *)(int))\
		m[i_amsg_exit].osapi)(rterrnum);
}

//------------------------------------------------------------------------------
// _cexitHook
//------------------------------------------------------------------------------
static void __cdecl _cexitHook(void)
{
	Method * m = GetMonitorMethods(0);
	LOG_INFO("_cexit [%u]", GetCurrentProcessId());
	UninstallResMonitoringSystem();
	((void (__cdecl *)(void))\
		m[i_cexit].osapi)();
}

//------------------------------------------------------------------------------
// _c_exitHook
//------------------------------------------------------------------------------
static void __cdecl _c_exitHook(void)
{
	Method * m = GetMonitorMethods(0);
	LOG_INFO("_c_exit [%u]", GetCurrentProcessId());
	UninstallResMonitoringSystem();
	((void (__cdecl *)(void))\
		m[i_c_exit].osapi)();
}

//------------------------------------------------------------------------------
// callocHook
//------------------------------------------------------------------------------
static void * __cdecl callocHook(_In_ size_t _Count, _In_ size_t _Size)
{
	Method * m = GetMonitorMethods(0);
	void * ptr = 0;

	ASSERT( UPTR(m[icalloc].osapi) );
	ptr = ((void * (__cdecl *)(size_t, size_t)) \
		m[icalloc].osapi)(_Count, _Size);

	if( ptr )
		MonAlloc(ptr, _ReturnAddress(), _Count*_Size, (ptr_t)0, CallocMemType);
	return ptr;

}

//------------------------------------------------------------------------------
// mallocHook
//------------------------------------------------------------------------------
static void * __cdecl mallocHook(_In_ size_t _Size)
{
	Method * m = GetMonitorMethods(0);
	void * ptr = 0;

	ASSERT( UPTR(m[icalloc].osapi) );
	ptr = ((void * (__cdecl *)(size_t)) \
		m[imalloc].osapi)(_Size);

	if( ptr )
		MonAlloc(ptr, _ReturnAddress(), _Size, (ptr_t)0, MallocMemType);
	return ptr;
}

//------------------------------------------------------------------------------
// reallocHook
//------------------------------------------------------------------------------
static void * __cdecl reallocHook(_In_opt_ void * _Memory, _In_ size_t _NewSize)
{
	Method * m = GetMonitorMethods(0);
	void * ptr = 0;
	MonFree(_Memory);
	ASSERT( UPTR(m[irealloc].osapi) );
	ptr = ((void * (__cdecl *)(void *,size_t)) \
		m[irealloc].osapi)(_Memory, _NewSize);

	if( ptr )
		MonAlloc(ptr, _ReturnAddress(), _NewSize, (ptr_t)0, MallocMemType);
	return ptr;
}

//------------------------------------------------------------------------------
// freeHook
//------------------------------------------------------------------------------
static void __cdecl freeHook(_Inout_opt_ void * _Memory)
{
	Method * m = GetMonitorMethods(0);
	MonFree(_Memory);
	ASSERT( UPTR(m[ifree].osapi) );
	((void (__cdecl *)(void*))m[ifree].osapi)(_Memory);
}

//------------------------------------------------------------------------------
// GetMonitorMethods
//------------------------------------------------------------------------------
extern Method * GetMonitorMethods(__out_opt unsigned long * totalMethods)
{
	static signed init = FALSE;
	static Method m[] = {\
		{0, 0, "HeapAlloc",         HeapAllocHook},       
		{0, 0, "HeapFree",          HeapFreeHook},           
		{0, 0, "HeapDestroy",       HeapDestroyHook},           
		{0, 0, "CreateThread",      CreateThreadHook},
		{0, 0, "OpenThread",        OpenThreadHook},
		{0, 0, "CloseHandle",       CloseHandleHook},     
		{0, 0, "CreateEventW",      CreateEventWHook},        
		{0, 0, "CreateEventA",      CreateEventAHook},
		{0, 0, "OpenEventW",        OpenEventWHook},        
		{0, 0, "OpenEventA",        OpenEventAHook},
		{0, 0, "HeapReAlloc",       HeapReAllocHook},
		{0, 0, "VirtualAlloc",      VirtualAllocHook},
		{0, 0, "VirtualAllocEx",    VirtualAllocExHook},
		{0, 0, "VirtualAllocExNuma",VirtualAllocExNumaHook},
		{0, 0, "VirtualFree",       VirtualFreeHook},
		{0, 0, "VirtualFreeEx",     VirtualFreeExHook},
		{0, 0, "CreateSemaphoreA",  CreateSemaphoreAHook},
		{0, 0, "CreateSemaphoreW",  CreateSemaphoreWHook},
		{0, 0, "OpenSemaphoreA",    OpenSemaphoreAHook},
		{0, 0, "OpenSemaphoreW",    OpenSemaphoreWHook},
		{0, 0, "CreateMutexA",      CreateMutexAHook},
		{0, 0, "CreateMutexW",      CreateMutexWHook},
		{0, 0, "OpenMutexA",        OpenMutexAHook},
		{0, 0, "OpenMutexW",        OpenMutexWHook},
		{0, 0, "ExitProcess",       ExitProcessHook},
		{0, 0, "TerminateProcess",  TerminateProcessHook},
		{0, 0, "exit",		    exitHook},
		{0, 0, "_exit",		    _exitHook},
		{0, 0, "_amsg_exit",	    _amsg_exitHook},
		{0, 0, "_c_exit",	    _c_exitHook},
		{0, 0, "_cexit",	    _cexitHook},
		{0, 0, "malloc",	    mallocHook},
		{0, 0, "free",		    freeHook},
		{0, 0, "realloc",	    reallocHook},
		{0, 0, "calloc",	    callocHook}};

	if( totalMethods ) {
		if( !init ) {
			HookApiIndex index = 0;
			HMODULE kernel32 = GetModuleHandle(_T("kernel32.dll"));
			HMODULE msvcrt = GetModuleHandle(_T("msvcrt.dll"));

			C_ASSERT( sizeof(m)/sizeof(m[0]) == iMax );

			ASSERT( UPTR(kernel32) );
			if( !UPTR(kernel32) ) {
				LOG_ERROR("GetModuleHandle(\"kernel32.dll\") error %u", \
					GetLastError());
				return (Method *)0;
			}

			C_ASSERT( sizeof(m)/sizeof(m[0]) == iMax );
			while( index < iMax ) {
				if( index < iMaxKernel32 )
					m[index].osapi = GetProcAddress(kernel32, m[index].apiname);
				else if( UPTR(msvcrt) )
					m[index].osapi = GetProcAddress(msvcrt, m[index].apiname);
				index++;
			}
			init = TRUE;
		}
		*totalMethods = iMax;
	}
	return (Method *)&m;
}
