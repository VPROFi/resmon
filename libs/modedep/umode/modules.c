#ifdef _KERNEL_MODE
#error This code only for user mode
#endif
#include <common/dbglog.h>
#include <common/checkptr.h>
#include <modedep/osdep.h>

typedef struct _PEB_LDR_DATA {
	unsigned long Length;
	unsigned char Initialized;
	void* SsHandle;
	LIST_ENTRY InLoadOrderModuleList;
	LIST_ENTRY InMemoryOrderModuleList;
	LIST_ENTRY InInitializationOrderModuleList;
	void* EntryInProgress;
	unsigned char ShutdownInProgress;
	void* ShutdownThreadId;
} PEB_LDR_DATA;

#pragma warning(push)
#pragma warning(disable:4214)
typedef struct _RTL_BALANCED_NODE {
    union {
        struct _RTL_BALANCED_NODE *Children[2];
        struct {
            struct _RTL_BALANCED_NODE *Left;
            struct _RTL_BALANCED_NODE *Right;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
#define RTL_BALANCED_NODE_RESERVED_PARENT_MASK 3
    union {
        UCHAR Red : 1;
        UCHAR Balance : 2;
        ULONG_PTR ParentValue;
    } DUMMYUNIONNAME2;
} RTL_BALANCED_NODE, *PRTL_BALANCED_NODE;
#pragma warning(pop)

typedef enum _LDR_DLL_LOAD_REASON {
  LoadReasonStaticDependency = 0x0,
  LoadReasonStaticForwarderDependency = 0x1,
  LoadReasonDynamicForwarderDependency = 0x2,
  LoadReasonDelayloadDependency = 0x3,
  LoadReasonDynamicLoad = 0x4,
  LoadReasonAsImageLoad = 0x5,
  LoadReasonAsDataLoad = 0x6,
  LoadReasonEnclavePrimary = 0x7,
  LoadReasonEnclaveDependency = 0x8,
  LoadReasonUnknown = 0xffffffff,
} LDR_DLL_LOAD_REASON;

typedef struct _LDR_DATA_TABLE_ENTRY {
	LIST_ENTRY InLoadOrderLinks;
	LIST_ENTRY InMemoryOrderLinks;
	LIST_ENTRY InInitializationOrderLinks;
	void* DllBase;
	void* EntryPoint;
	unsigned long SizeOfImage;
	struct _UNICODE_STRING FullDllName;
	struct _UNICODE_STRING BaseDllName;
	union {
		unsigned char FlagGroup[4];
		unsigned long Flags;
		struct {
			unsigned long PackagedBinary:1;
			unsigned long MarkedForRemoval:1;
			unsigned long ImageDll:1;
			unsigned long LoadNotificationsSent:1;
			unsigned long TelemetryEntryProcessed:1;
			unsigned long ProcessStaticImport:1;
			unsigned long InLegacyLists:1;
			unsigned long InIndexes:1;
			unsigned long ShimDll:1;
			unsigned long InExceptionTable:1;
			unsigned long ReservedFlags1:2;
			unsigned long LoadInProgress:1;
			unsigned long LoadConfigProcessed:1;
			unsigned long EntryProcessed:1;
			unsigned long ProtectDelayLoad:1;
			unsigned long ReservedFlags3:2;
			unsigned long DontCallForThreads:1;
			unsigned long ProcessAttachCalled:1;
			unsigned long ProcessAttachFailed:1;
			unsigned long CorDeferredValidate:1;
			unsigned long CorImage:1;
			unsigned long DontRelocate:1;
			unsigned long CorILOnly:1;
			unsigned long ChpeImage:1;
			unsigned long ReservedFlags5:2;
			unsigned long Redirected:1;
			unsigned long ReservedFlags6:2;
			unsigned long CompatDatabaseProcessed:1;
		};
	};
	unsigned short ObsoleteLoadCount;
	unsigned short TlsIndex;
	LIST_ENTRY HashLinks;
	unsigned long TimeDateStamp;
	struct _ACTIVATION_CONTEXT* EntryPointActivationContext;
	void* Lock;
	struct _LDR_DDAG_NODE* DdagNode;
	LIST_ENTRY NodeModuleLink;
	struct _LDRP_LOAD_CONTEXT* LoadContext;
	void* ParentDllBase;
	void* SwitchBackContext;
	RTL_BALANCED_NODE BaseAddressIndexNode;
	RTL_BALANCED_NODE MappingInfoIndexNode;
	ULONG_PTR OriginalBase;
	LARGE_INTEGER LoadTime;
	unsigned long BaseNameHashValue;
	LDR_DLL_LOAD_REASON LoadReason;
	unsigned long ImplicitPathOptions;
	unsigned long ReferenceCount;
	unsigned long DependentLoadFlags;
	unsigned char SigningLevel;
} LDR_DATA_TABLE_ENTRY;

typedef struct _PEB {
	unsigned char InheritedAddressSpace;
	unsigned char ReadImageFileExecOptions;
	unsigned char BeingDebugged;
	union {
		unsigned char BitField;
		struct {
			unsigned char ImageUsesLargePages:1;
			unsigned char IsProtectedProcess:1;
			unsigned char IsImageDynamicallyRelocated:1;
			unsigned char SkipPatchingUser32Forwarders:1;
			unsigned char IsPackagedProcess:1;
			unsigned char IsAppContainer:1;
			unsigned char IsProtectedProcessLight:1;
			unsigned char IsLongPathAwareProcess:1;
		};
	};
	void* Mutant;
	void* ImageBaseAddress;
	struct _PEB_LDR_DATA* Ldr;
	struct _RTL_USER_PROCESS_PARAMETERS* ProcessParameters;
	void* SubSystemData;
	void* ProcessHeap;
	struct _RTL_CRITICAL_SECTION* FastPebLock;
	union _SLIST_HEADER* AtlThunkSListPtr;
	void* IFEOKey;
	union {
		unsigned long CrossProcessFlags;
		struct {
			unsigned long ProcessInJob:1;
			unsigned long ProcessInitializing:1;
			unsigned long ProcessUsingVEH:1;
			unsigned long ProcessUsingVCH:1;
			unsigned long ProcessUsingFTH:1;
			unsigned long ProcessPreviouslyThrottled:1;
			unsigned long ProcessCurrentlyThrottled:1;
			unsigned long ProcessImagesHotPatched:1;
			unsigned long ReservedBits0:18;
		};
	};
	union {
		void* KernelCallbackTable;
		void* UserSharedInfoPtr;
	};
	unsigned long SystemReserved;
	unsigned long AtlThunkSListPtr32;
	void* ApiSetMap;
	unsigned long TlsExpansionCounter;
	void* TlsBitmap;
	unsigned long TlsBitmapBits[2];
	void* ReadOnlySharedMemoryBase;
	void* SharedData;
	void** ReadOnlyStaticServerData;
	void* AnsiCodePageData;
	void* OemCodePageData;
	void* UnicodeCaseTableData;
	unsigned long NumberOfProcessors;
	unsigned long NtGlobalFlag;
	LARGE_INTEGER CriticalSectionTimeout;
	SIZE_T HeapSegmentReserve;
	SIZE_T HeapSegmentCommit;
	SIZE_T HeapDeCommitTotalFreeThreshold;
	SIZE_T HeapDeCommitFreeBlockThreshold;
	unsigned long NumberOfHeaps;
	unsigned long MaximumNumberOfHeaps;
	void** ProcessHeaps;
	void* GdiSharedHandleTable;
	void* ProcessStarterHelper;
	unsigned long GdiDCAttributeList;
	struct _RTL_CRITICAL_SECTION* LoaderLock;
	unsigned long OSMajorVersion;
	unsigned long OSMinorVersion;
	unsigned short OSBuildNumber;
	unsigned short OSCSDVersion;
	unsigned long OSPlatformId;
	unsigned long ImageSubsystem;
	unsigned long ImageSubsystemMajorVersion;
	unsigned long ImageSubsystemMinorVersion;
	ULONG_PTR ActiveProcessAffinityMask;
#if defined _M_IX86
	unsigned long GdiHandleBuffer[34];
#elif defined _M_X64
	unsigned long GdiHandleBuffer[60];
#endif
	void (* PostProcessInitRoutine)(void);
	void* TlsExpansionBitmap;
	unsigned long TlsExpansionBitmapBits[32];
	unsigned long SessionId;
	ULARGE_INTEGER AppCompatFlags;
	ULARGE_INTEGER AppCompatFlagsUser;
	void* pShimData;
	void* AppCompatInfo;
	UNICODE_STRING CSDVersion;
	struct _ACTIVATION_CONTEXT_DATA* ActivationContextData;
	struct _ASSEMBLY_STORAGE_MAP* ProcessAssemblyStorageMap;
	struct _ACTIVATION_CONTEXT_DATA* SystemDefaultActivationContextData;
	struct _ASSEMBLY_STORAGE_MAP* SystemAssemblyStorageMap;
	SIZE_T MinimumStackCommit;
	void* SparePointers[4];
	unsigned long SpareUlongs[5];
	void* WerRegistrationData;
	void* WerShipAssertPtr;
	void* pUnused;
	void* pImageHeaderHash;
	union {
		unsigned long TracingFlags;
		struct {
			unsigned long HeapTracingEnabled:1;
			unsigned long CritSecTracingEnabled:1;
			unsigned long LibLoaderTracingEnabled:1;
			unsigned long SpareTracingBits:0x1d;
		};
	};
    ULONGLONG CsrServerReadOnlySharedMemoryBase;
	ULONG_PTR TppWorkerpListLock;
	LIST_ENTRY TppWorkerpList;
	void* WaitOnAddressHashTable[128];
	void* TelemetryCoverageHeader;
	unsigned long CloudFileFlags;
	unsigned long CloudFileDiagFlags;
	char PlaceholderCompatibilityMode;
	char PlaceholderCompatibilityModeReserved[7];
	struct _LEAP_SECOND_DATA* LeapSecondData;
	union {
		unsigned long LeapSecondFlags;
		struct {
			unsigned long SixtySecondEnabled:1;
			unsigned long Reserved:0x1f;
		};
	};
	unsigned long NtGlobalFlag2;
} PEB;

#if defined _M_IX86
	C_ASSERT(sizeof(PEB) == 0x480);
	C_ASSERT(sizeof(PEB_LDR_DATA) == 0x30);
	C_ASSERT(sizeof(LDR_DATA_TABLE_ENTRY) == 0xA8);
#elif defined _M_X64
	C_ASSERT(sizeof(PEB) == 0x7C8);
	C_ASSERT(sizeof(PEB_LDR_DATA) == 0x58);
	C_ASSERT(sizeof(LDR_DATA_TABLE_ENTRY) == 0x120);
#endif

extern NTSTATUS WINAPI RtlGetVersion(__out PRTL_OSVERSIONINFOW version);
//------------------------------------------------------------------------------
// OsFreeModuleData
//------------------------------------------------------------------------------
extern void OsFreeModuleData(OsModuleData * mod)
{
	if( mod->basename.str ) {
		HeapFree(GetProcessHeap(), 0, mod->basename.str);
		mod->basename.str = 0;
	}
}
//------------------------------------------------------------------------------
// OsModuleInfo
//------------------------------------------------------------------------------
static signed OsModuleInfo(void *ptr, UNICODE_STRING * us, OsModuleData * mod)
{

	#if defined _M_IX86
	PEB * peb =(PEB *)__readfsdword(0x30);
	#elif defined _M_X64
	PEB * peb =(PEB *)__readgsqword(0x60);
	#endif

	PEB_LDR_DATA * loaderData = 0;
	LIST_ENTRY * listHead = 0, * listEntry = 0;

	#if DBG
	RTL_OSVERSIONINFOW vi = {sizeof(RTL_OSVERSIONINFOW), 0};
	ASSERT( NT_SUCCESS(RtlGetVersion(&vi) ) );

	ASSERT( PTR_USER(peb) );
	ASSERT( peb->OSMajorVersion == vi.dwMajorVersion );
	ASSERT( peb->OSMinorVersion == vi.dwMinorVersion );
	ASSERT( peb->OSBuildNumber == vi.dwBuildNumber );
	ASSERT( PTR_USER(peb->LoaderLock) );
	#endif

	ASSERT( UPTR(mod) );
	mod->base = 0;
	mod->imageSize = 0;

	if( !UPTR(ptr) && !UPTR(us) )
		ptr = _ReturnAddress();

	ASSERT( (UPTR(ptr) && !UPTR(us)) || (!UPTR(ptr) && UPTR(us)) );

	EnterCriticalSection(peb->LoaderLock);

	loaderData = peb->Ldr;
	ASSERT( PTR_USER(loaderData) );
	listHead = &loaderData->InLoadOrderModuleList;
	listEntry = listHead->Flink;

	while( listEntry != listHead ) {
		LDR_DATA_TABLE_ENTRY * ldte = CONTAINING_RECORD(listEntry, \
				LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

		ASSERT( PTR_USER(listEntry) );
		ASSERT( PTR_USER(ldte->DllBase) );
		ASSERT( PTR_USER(ldte->FullDllName.Buffer) );
		ASSERT( PTR_USER(ldte->BaseDllName.Buffer) );
		ASSERT( ldte->SizeOfImage != 0 );

		if( (us && RtlEqualUnicodeString(us, &ldte->BaseDllName, TRUE)) || \
			(ptr && (ptr >= ldte->DllBase && \
			ptr < (void *)((char *)ldte->DllBase + ldte->SizeOfImage))) ) {

			mod->basename.str = HeapAlloc(GetProcessHeap(),
						HEAP_ZERO_MEMORY,
						ldte->BaseDllName.Length+sizeof(uni_char));

			if( !mod->basename.str )
				break;

			RtlMoveMemory(mod->basename.str,
				ldte->BaseDllName.Buffer, ldte->BaseDllName.Length);

			mod->basename.size = ldte->BaseDllName.Length;
			mod->base = ldte->DllBase;
			mod->imageSize = ldte->SizeOfImage;
			break;
		}
		listEntry = listEntry->Flink;
	}
	LeaveCriticalSection(peb->LoaderLock);
	return UPTR(mod->base) ? TRUE:FALSE;
}

//------------------------------------------------------------------------------
// OsModuleInfoFromPointer
//------------------------------------------------------------------------------
extern signed OsModuleInfoFromPointer(void *ptr, OsModuleData * mod)
{
	return OsModuleInfo(ptr, (UNICODE_STRING *)0, mod);
}

//------------------------------------------------------------------------------
// OsModuleInfoFromPointer
//------------------------------------------------------------------------------
extern signed OsModuleInfoFromName(UniStrConst *name, OsModuleData * mod)
{
	UNICODE_STRING us = {(USHORT)name->size, \
						(USHORT)name->size+sizeof(WCHAR), \
						(PWCH)name->str};
	ASSERT( UPTR(name) );
	ASSERT( UPTR(name->str) );
	ASSERT( name->size >= sizeof(uni_char) );
	return OsModuleInfo((void *)0, &us, mod);
}
