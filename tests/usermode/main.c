#include <common/dbglog.h>
#include <shared/config.h>
#include "res/resource.h"
#include <resmon/resmonlib.h>
#include <stdio.h>
#include <tchar.h>

#pragma warning(suppress: 4229)
void (* __stdcall install)(void) = 0;
#pragma warning(suppress: 4229)
void (* __stdcall uninstall)(void) = 0;
#pragma warning(suppress: 4229)
void (* __stdcall logres)(void) = 0;

#ifdef DBG
#define CCNT 0x122345678
#define CCNT2 0x122345678
#else
#define CCNT 0x282345678
#define CCNT2 0x382345678
#endif

static unsigned long __stdcall TestThread1(void * shutDownEvent)
{
	unsigned long long sum = 0;
	unsigned long long index = 0;
	LOG_INFO(" ... start");

	for( index = 0; index < (unsigned long long)CCNT; index++ )
		sum += index;

	LOG_INFO(" ... sum %I64u", sum);
	_tprintf(_T(" ... sum(thread2) %I64u"), sum);

	OsWaitForSingleObject(shutDownEvent, OS_INFINITE);

	LOG_INFO(" ... finish");
	return 0;
}

static unsigned long __stdcall TestThread(void * shutDownEvent)
{
	void * ptr = 0;
	void * ptrEx = 0;
	void * ptrExNuma = 0;
	unsigned long long sum = 0;
	unsigned long long index = 0;
	HANDLE heap = 0, heap1 = 0, heap2 = 0;


	LOG_INFO(" ... start");

	ptr = VirtualAlloc(0, sizeof(MEMORY_BASIC_INFORMATION), MEM_COMMIT, PAGE_READWRITE);
	ptrEx = VirtualAllocEx(GetCurrentProcess(), 0, sizeof(MEMORY_BASIC_INFORMATION), MEM_COMMIT, PAGE_READWRITE);
	ptrExNuma = VirtualAllocExNuma(GetCurrentProcess(), 0, sizeof(MEMORY_BASIC_INFORMATION), MEM_COMMIT, PAGE_READWRITE, 0);


	heap = HeapCreate(0, 0, 0);

	if(heap) {
		unsigned long i = 0;
		for( i = 0; i < 733; i++ )
			HeapAlloc(heap, HEAP_ZERO_MEMORY, 0x33);
	}


	heap1 = HeapCreate(0, 0, 0);

	if(heap1) {
		unsigned long i = 0;
		for( i = 0; i < 327; i++ )
			HeapAlloc(heap1, HEAP_ZERO_MEMORY, 0x33);
	}

	heap2 = HeapCreate(0, 0, 0);

	if(heap2) {
		unsigned long i = 0;
		for( i = 0; i < 17; i++ )
			HeapAlloc(heap2, HEAP_ZERO_MEMORY, 0x33);
	}

	for( index = 0; index < (unsigned long long)CCNT2; index++ )
		sum += index;

	if(heap)
		HeapDestroy(heap);

	if(heap1)
		HeapDestroy(heap1);

	if(heap2)
		HeapDestroy(heap2);

	LOG_INFO(" ... sum %I64u", sum);
	_tprintf(_T(" ... sum(thread1) %I64u"), sum);

	if(logres)
		logres();
	else
		MonLogAllResources();

	OsWaitForSingleObject(shutDownEvent, OS_INFINITE);

	VirtualFree(ptr, 0, MEM_RELEASE);
	VirtualFreeEx(GetCurrentProcess(), ptrEx, 0, MEM_RELEASE);
	VirtualFreeEx(GetCurrentProcess(), ptrExNuma, 0, MEM_RELEASE);

	LOG_INFO(" ... finish");
	return 0;
}

static void test(void)
{
	MEMORY_BASIC_INFORMATION * mbi = 0;
	HANDLE event = 0;
	HANDLE thread = 0, thread1 = 0;
	HANDLE semaphoreA = 0, semaphoreW = 0;
	HANDLE semaphoreOpenA = 0, semaphoreOpenW = 0;
	HANDLE mutexA = 0, mutexW = 0;
	HANDLE mutexOpenA = 0, mutexOpenW = 0;

	semaphoreA = CreateSemaphoreA(NULL, 0, 1, "SemaphoreA");
	semaphoreW = CreateSemaphoreW(NULL, 0, 1, L"SemaphoreW");

	semaphoreOpenA = OpenSemaphoreA(SEMAPHORE_ALL_ACCESS, FALSE, "SemaphoreA");
	semaphoreOpenW = OpenSemaphoreW(SEMAPHORE_ALL_ACCESS, FALSE, L"SemaphoreW");

	mutexA = CreateMutexA(NULL, TRUE, "MutexA");
	mutexW = CreateMutexW(NULL, TRUE, L"MutexW");

	mutexOpenA = OpenMutexA(MUTEX_ALL_ACCESS, FALSE, "MutexA");
	mutexOpenW = OpenMutexW(MUTEX_ALL_ACCESS, FALSE, L"MutexW");

	mbi = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MEMORY_BASIC_INFORMATION));

	mbi = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, mbi, sizeof(MEMORY_BASIC_INFORMATION)*2);

	event = OsCreateEvent(TRUE, FALSE);

	thread = CreateThread(0, 0, TestThread, event, 0, 0);

	thread1 = CreateThread(0, 0, TestThread1, event, 0, 0);

	if( thread ) {
		OsSetEvent(event);
		OsWaitForSingleObject(thread, OS_INFINITE);
		OsCloseThread(thread);
	}

	if( thread1 ) {
		OsSetEvent(event);
		OsWaitForSingleObject(thread1, OS_INFINITE);
		OsCloseThread(thread1);
	}

	CloseHandle(semaphoreA);
	CloseHandle(semaphoreW);
	CloseHandle(semaphoreOpenA);
	CloseHandle(semaphoreOpenW);

	CloseHandle(mutexA);
	CloseHandle(mutexW);
	CloseHandle(mutexOpenA);
	CloseHandle(mutexOpenW);

	OsClose(event);

	HeapFree(GetProcessHeap(), 0, mbi);
}


int __cdecl _tmain(int argc, _TCHAR* argv[])
{
	HMODULE dll = LoadLibrary(_T("resmon.dll"));

	ASSERT( argc );
	_tprintf(_T(": ************************************\n"));
	_tprintf(_T(": resmon usermode test load at 0x%p\n"), GetModuleHandle(0));
	_tprintf(_T(": Built on "__DATE__" at "__TIME__ " \n"));
	_tprintf(_T(": ************************************\n"));

	install = (void (__stdcall *)(void))GetProcAddress(dll, "InstallResMon");
	uninstall = (void (__stdcall *)(void))GetProcAddress(dll, "UninstallResMon");
	logres = (void (__stdcall *)(void))GetProcAddress(dll, "LogAllResources");

	install();
	_tprintf(_T(": resmon.dll ... test begin "));
	test();
	_tprintf(_T("\n"));
	uninstall();

	install();
	_tprintf(_T(": resmon.dll ... second test begin "));
	test();
	_tprintf(_T("\n"));
	uninstall();

	FreeLibrary(dll);

	logres = 0;

	InstallResMonitoringSystem(GetModuleHandle(0), 0);
	_tprintf(_T(": resmon.lib ... test begin "));
	test();
	_tprintf(_T("\n"));
	UninstallResMonitoringSystem();

	InstallResMonitoringSystem(GetModuleHandle(0), 0);
	_tprintf(_T(": resmon.lib ... second test begin "));
	test();
	_tprintf(_T("\n"));
	UninstallResMonitoringSystem();

	return 0;
}
