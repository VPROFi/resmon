#include <common/dbglog.h>
#include <shared/config.h>
#include "res/resource.h"
#include <stdio.h>
#include <tchar.h>
#include <TlHelp32.h>

//------------------------------------------------------------------------------
// sructures
//------------------------------------------------------------------------------
typedef struct {
	void * realcall;
	void * loadLibraryW;
	void * getProcAddress;
	void * freeLibrary;
	WCHAR dll[sizeof(L"resmon.dll")];
	char install[sizeof("InstallResMon")];
	char uninstall[sizeof("UninstallResMon")];
} Loader;

//------------------------------------------------------------------------------
// defaults
//------------------------------------------------------------------------------
#define DLL_NAME "resmon.dll"
#define _WSTR(...) L##__VA_ARGS__
#define DLL_NAMEW _WSTR(DLL_NAME)
#define DLL_SUFFIX "\\"DLL_NAME
#define DLL_SUFFIXW _WSTR(DLL_SUFFIX)

//------------------------------------------------------------------------------
// SkipArg
//------------------------------------------------------------------------------
static _TCHAR * SkipArg(_TCHAR * cmdl)
{
	while( *cmdl && *cmdl == ' ' )
		cmdl++;
	if( *cmdl == '"' ) {
		cmdl++;
		while( *cmdl && *cmdl != '"' )
			cmdl++;
		if( *cmdl == '"' )
			cmdl++;
	} else
		while( *cmdl && *cmdl != ' ' )
			cmdl++;

	while( *cmdl && *cmdl == ' ' )
		cmdl++;
	return cmdl;
}

//------------------------------------------------------------------------------
// FillImports
//------------------------------------------------------------------------------
static void FillImports(
				void * base,
				ULONG_PTR delta,
				const char ** functions,
				void ** imports)
{
	while( *functions ) {
		*imports = (char *)GetProcAddress(base, *functions) + delta;
		functions++;
		imports++;
	}
}

//------------------------------------------------------------------------------
// GetImports
//------------------------------------------------------------------------------
static signed GetImports(DWORD pid, _TCHAR * mname, const char ** functions, void ** imports)
{
	MODULEENTRY32 me = {sizeof(MODULEENTRY32), 0};
	HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
	BOOL res = 0;
	if( snap == INVALID_HANDLE_VALUE ) {
		while( GetLastError() == ERROR_BAD_LENGTH ) {
			snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
			if (snap != INVALID_HANDLE_VALUE)
				break;
		}
	}

	if( snap == INVALID_HANDLE_VALUE ) {
		_tprintf(_T("CreateToolhelp32Snapshot(%u) error %u\n"), pid, GetLastError());
		return FALSE;
	}
		
	res = Module32First(snap, &me);
	while( res ) {
		if( !_tcsicmp(me.szModule, mname) ) {
			if( GetModuleHandle(mname) )
				FillImports(
					GetModuleHandle(mname),
					(ULONG_PTR)GetModuleHandle(mname) - (ULONG_PTR)me.hModule,
					functions,
					imports);
			else {
				HMODULE mod = LoadLibrary(mname);
				if( mod ) {
					FillImports(
						mod,
						(ULONG_PTR)mod - (ULONG_PTR)me.hModule,
						functions,
						imports);
					FreeLibrary(mod);
				}
			}
			break;
		}
		res = Module32Next(snap, &me);
	}
	CloseHandle(snap);
	return TRUE;
}

//------------------------------------------------------------------------------
// asm block
//------------------------------------------------------------------------------
extern char run;
extern char run_end;
extern char loader_start;

//------------------------------------------------------------------------------
// _tmain
//------------------------------------------------------------------------------
int __cdecl _tmain(int argc, _TCHAR* argv[])
{
	_TCHAR * cmdl = GetCommandLine();
	STARTUPINFO si = {0};
	PROCESS_INFORMATION pi = {0};
	_tprintf(_T("************************************\n"));
	_tprintf(_T(" resmon client\n"));
	_tprintf(_T(" Built on "__DATE__" at "__TIME__ "\n"));
	_tprintf(_T(" ************************************\n"));

	cmdl = SkipArg(cmdl);

	if( argc < 2 ) {
		_tprintf(_T("Use %s <anyfile.exe> [arguments]\n"), argv[0]);
		return -1;
	}

	_tprintf(_T("start = %s\n"), cmdl);

	si.cb = sizeof(STARTUPINFO);

	if( !CreateProcess(
		0, cmdl, 0, 0, FALSE,
		#ifdef _UNICODE
		CREATE_SUSPENDED|DETACHED_PROCESS|CREATE_UNICODE_ENVIRONMENT,
		#else
		CREATE_SUSPENDED|DETACHED_PROCESS,
		#endif
		GetEnvironmentStrings(), 0,	&si, &pi)) {
		_tprintf(_T("CreateProcess(%s) error %u\n"), cmdl, GetLastError());
		return -1;
	} else {
		Loader * loader = (Loader *)(&run + (&loader_start - &run));
		PVOID codeBuf = 0;
		ULONG loaderSize = (ULONG)(ULONG_PTR)(&run_end - &run), pathSize = 0;
		CONTEXT ctx = {0};

		ctx.ContextFlags = CONTEXT_ALL;
		GetThreadContext(pi.hThread, &ctx);

		#if defined(_WIN64)
		loader->realcall = (void *)ctx.Rcx;
		#else
		loader->realcall = (void *)ctx.Eax;
		#endif

		pathSize = GetCurrentDirectoryW(MAX_PATH-sizeof(DLL_SUFFIX), (LPWSTR)loader->dll);
		if(pathSize)
			RtlMoveMemory(&loader->dll[pathSize], DLL_SUFFIXW, sizeof(DLL_SUFFIXW));
		else
			RtlMoveMemory(&loader->dll[pathSize], DLL_NAMEW, sizeof(DLL_NAMEW));

		codeBuf = VirtualAllocEx(pi.hProcess, NULL, loaderSize, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
		if( codeBuf && !WriteProcessMemory(pi.hProcess, codeBuf, &run, loaderSize, NULL) ) {
			_tprintf(_T("WriteProcessMemory(%p) error %u\n"), codeBuf, GetLastError());
		} else if(codeBuf) {
			static const char * functions[] = { \
				"LoadLibraryW",
				"GetProcAddress",
				"FreeLibrary", 0};

			#if defined(_WIN64)
			ctx.Rcx = (DWORD64)codeBuf;
			#else
			ctx.Eax = (DWORD)codeBuf;
			#endif

			SetThreadContext(pi.hThread, &ctx);
			ResumeThread(pi.hThread);
			do {
				Sleep(30);
				GetThreadContext(pi.hThread, &ctx);
			}
			#if defined(_WIN64)
			while( ctx.Rip < (DWORD64)codeBuf || ctx.Rip > ((DWORD64)codeBuf+2) );
			#else
			while( ctx.Eip < (DWORD64)codeBuf || ctx.Eip > ((DWORD64)codeBuf+2) );
			#endif
			SuspendThread(pi.hThread);

			if( GetImports(pi.dwProcessId, _T("kernel32.dll"), (const char **)functions, &loader->loadLibraryW) &&
				WriteProcessMemory(pi.hProcess, codeBuf, &run, loaderSize, NULL) ) {
				#if defined(_WIN64)
				ctx.Rip = (DWORD64)((char*)codeBuf+2);
				#else
				ctx.Eip = (DWORD)((char*)codeBuf+2);
				#endif
			} else {
				#if defined(_WIN64)
				ctx.Rip = (DWORD64)loader->realcall;
				#else
				ctx.Eip = (DWORD)loader->realcall;
				#endif
			}
			SetThreadContext(pi.hThread, &ctx);
		}
	}
	ResumeThread(pi.hThread);
	return 0;
}
