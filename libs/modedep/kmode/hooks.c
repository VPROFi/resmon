#ifndef _KERNEL_MODE
#error This code only for kenel mode
#endif
#include <common/dbglog.h>
#include <common/checkptr.h>
#include <modedep/modedep.h>
#define TAG 'KHMK'
#define IS_KERNEL_HANDLE(x) ((ULONG_PTR)(x) & 0x80000000)
//------------------------------------------------------------------------------
// HookApiIndex enum
//------------------------------------------------------------------------------
typedef enum {
	iExAllocatePoolWithTag,
	iExFreePoolWithTag,
	iExAllocatePool,
	iExAllocatePoolWithQuota,
	iExAllocatePoolWithTagPriority,
	iExAllocatePoolWithQuotaTag,
	iExFreePool,
	iMmAllocateContiguousMemory,
	iMmAllocateContiguousMemorySpecifyCache,
	iMmAllocateContiguousMemorySpecifyCacheNode,
	iMmFreeContiguousMemory,
	iMmFreeContiguousMemorySpecifyCache,
	iMmMapIoSpace,
	iMmUnmapIoSpace,
	iPsCreateSystemThread,
	iPsTerminateSystemThread,
	iIoCreateSystemThread,
	iPsCreateSystemThreadEx,
	iZwClose,
	iNtClose,
	iObCloseHandle,
	iIoCreateNotificationEvent,
	iIoCreateSynchronizationEvent,
	iNtCreateEvent,
	iZwCreateEvent,
	iNtOpenEvent,
	iZwOpenEvent,

	// side alloc api
	iSideAllocApiBegin,
	iIoGetDeviceInterfaces = iSideAllocApiBegin,
	iSeQueryInformationToken,
	iSideAllocApiEnd,

	iMax = iSideAllocApiEnd
} HookApiIndex;

// memory types
typedef enum {
	NonPagedPoolMemType = 0x0,
	NonPagedPoolExecuteMemType = 0x0,
	PagedPoolMemType = 0x1,
	NonPagedPoolMustSucceedMemType = 0x2,
	DontUseThisTypeMemType = 0x3,
	NonPagedPoolCacheAlignedMemType = 0x4,
	PagedPoolCacheAlignedMemType = 0x5,
	NonPagedPoolCacheAlignedMustSMemType = 0x6,
	MaxPoolTypeMemType = 0x7,
	NonPagedPoolBaseMemType = 0x0,
	NonPagedPoolBaseMustSucceedMemType = 0x2,
	NonPagedPoolBaseCacheAlignedMemType = 0x4,
	NonPagedPoolBaseCacheAlignedMustSMemType = 0x6,
	NonPagedPoolSessionMemType = 0x20,
	PagedPoolSessionMemType = 0x21,
	NonPagedPoolMustSucceedSessionMemType = 0x22,
	DontUseThisTypeSessionMemType = 0x23,
	NonPagedPoolCacheAlignedSessionMemType = 0x24,
	PagedPoolCacheAlignedSessionMemType = 0x25,
	NonPagedPoolCacheAlignedMustSSessionMemType = 0x26,
	NonPagedPoolNxMemType = 0x200,
	NonPagedPoolNxCacheAlignedMemType = 0x204,
	NonPagedPoolSessionNxMemType = 0x220,
	HeapMemType = 0x1000,
	VirtualMemType,
	IoMapMemType,
	MmContiguousMemoryMemType,

	// side alloc mem type
	SideAllocApiBeginMemType,
	IoGetDeviceInterfacesMemType = SideAllocApiBeginMemType,
	SeQueryInformationTokenMemType,
	SideAllocApiEndMemType,

	LastMemType = SideAllocApiEndMemType
} MemoryType;
//------------------------------------------------------------------------------
// GetMemoryTypeName
//------------------------------------------------------------------------------
extern const char * GetMemoryTypeName(unsigned long mtype)
{
	Method * m = GetMonitorMethods(0);
	switch(mtype) {
		case NonPagedPoolMemType:
			return (const char *)"NonPagedPool";
		case PagedPoolMemType:
			return (const char *)"PagedPool";
		case NonPagedPoolMustSucceedMemType:
			return (const char *)"NonPagedPoolMustSucceed";
		case DontUseThisTypeMemType:
			return (const char *)"DontUseThisType";
		case NonPagedPoolCacheAlignedMemType:
			return (const char *)"NonPagedPoolCacheAligned";
		case PagedPoolCacheAlignedMemType:
			return (const char *)"PagedPoolCacheAligned";
		case NonPagedPoolCacheAlignedMustSMemType:
			return (const char *)"NonPagedPoolCacheAlignedMustS";
		case MaxPoolTypeMemType:
			return (const char *)"MaxPoolType";
		case NonPagedPoolSessionMemType:
			return (const char *)"NonPagedPoolSession";
		case PagedPoolSessionMemType:
			return (const char *)"PagedPoolSession";
		case NonPagedPoolMustSucceedSessionMemType:
			return (const char *)"NonPagedPoolMustSucceedSession";
		case DontUseThisTypeSessionMemType:
			return (const char *)"DontUseThisTypeSession";
		case NonPagedPoolCacheAlignedSessionMemType:
			return (const char *)"NonPagedPoolCacheAlignedSession";
		case PagedPoolCacheAlignedSessionMemType:
			return (const char *)"PagedPoolCacheAlignedSession";
		case NonPagedPoolCacheAlignedMustSSessionMemType:
			return (const char *)"NonPagedPoolCacheAlignedMustSSession";
		case NonPagedPoolNxMemType:
			return (const char *)"NonPagedPoolNx";
		case NonPagedPoolNxCacheAlignedMemType:
			return (const char *)"NonPagedPoolNxCacheAligned";
		case NonPagedPoolSessionNx:
			return (const char *)"NonPagedPoolSessionNx";
		case HeapMemType:
			return (const char *)"Heap";
		case VirtualMemType:
			return (const char *)"Virtual";
		case IoMapMemType:
			return (const char *)"IoMap";
		case MmContiguousMemoryMemType:
			return (const char *)"MmContiguousMemory";
	}

	if( mtype >= SideAllocApiBeginMemType &&
		mtype < SideAllocApiEndMemType ) {
		ASSERT( (mtype+iSideAllocApiBegin) < iMax );
		return m[mtype+iSideAllocApiBegin].apiname;
	}

	return (const char *)"Unknown";
}

//------------------------------------------------------------------------------
// OsAllocObject
//------------------------------------------------------------------------------
extern void * OsAllocObject(size_tr size)
{
	Method * m = GetMonitorMethods(0);
	void * obj = ((void * (NTAPI *)(POOL_TYPE, SIZE_T, ULONG)) \
			m[iExAllocatePoolWithTag].osapi)(NonPagedPool, \
				size, TAG);
	if( obj )
		RtlZeroMemory(obj, size);
	return obj;
}

//------------------------------------------------------------------------------
// OsFreeObject
//------------------------------------------------------------------------------
extern void OsFreeObject(void * obj)
{
	Method * m = GetMonitorMethods(0);
	((void (NTAPI *)(PVOID, ULONG)) \
			m[iExFreePoolWithTag].osapi)(obj, TAG);
	return;
}

//------------------------------------------------------------------------------
// ExAllocatePoolCommon
//------------------------------------------------------------------------------
static PVOID NTAPI ExAllocatePoolCommon(
	__in HookApiIndex index,
	__in void * returnAddress,
	__in __drv_strictTypeMatch(__drv_typeExpr) POOL_TYPE PoolType,
	__in SIZE_T NumberOfBytes,
	__in ULONG Tag,
    __in __drv_strictTypeMatch(__drv_typeExpr) EX_POOL_PRIORITY Priority)
{
	Method * m = GetMonitorMethods(0);
	void * ptr = 0;

	ASSERT( KPTR(m[index].osapi) );

	switch(index) {
		case iExAllocatePoolWithTag:
		case iExAllocatePoolWithQuotaTag:
			ptr = ((void * (NTAPI *)(POOL_TYPE, SIZE_T, ULONG)) \
				m[index].osapi)(PoolType, NumberOfBytes, Tag);
			break;
		case iExAllocatePool:
		case iExAllocatePoolWithQuota:
			ptr = ((void * (NTAPI *)(POOL_TYPE, SIZE_T)) \
				m[index].osapi)(PoolType, NumberOfBytes);
			break;
		case iExAllocatePoolWithTagPriority:
			ptr = ((void * (NTAPI *)(POOL_TYPE, SIZE_T, ULONG, \
				EX_POOL_PRIORITY))m[index].osapi)(PoolType, \
					NumberOfBytes, Tag, Priority);
			break;
		default:
			ASSERT( index == iExAllocatePoolWithTag || \
					index == iExAllocatePoolWithQuotaTag || \
					index == iExAllocatePool ||
					index == iExAllocatePoolWithQuota ||
					index == iExAllocatePoolWithTagPriority);

			break;
	}

	if( ptr )
		MonAlloc(ptr, returnAddress, NumberOfBytes, (ptr_t)Tag,
			(MemoryType)PoolType);
	return ptr;
}

//------------------------------------------------------------------------------
// ExAllocatePoolHook
//------------------------------------------------------------------------------
static PVOID NTAPI ExAllocatePoolHook(
    __drv_strictTypeMatch(__drv_typeExpr) __in POOL_TYPE PoolType,
    __in SIZE_T NumberOfBytes)
{
	return ExAllocatePoolCommon(
		iExAllocatePool,
		_ReturnAddress(),
		PoolType,
		NumberOfBytes,
		(ULONG)'enoN', 0);
}

//------------------------------------------------------------------------------
// ExAllocatePoolWithQuotaHook
//------------------------------------------------------------------------------
static PVOID NTAPI ExAllocatePoolWithQuotaHook(
    __drv_strictTypeMatch(__drv_typeExpr) __in POOL_TYPE PoolType,
    __in SIZE_T NumberOfBytes)
{
	return ExAllocatePoolCommon(
		iExAllocatePoolWithQuota,
		_ReturnAddress(),
		PoolType,
		NumberOfBytes,
		(ULONG)'enoN', 0);
}

//------------------------------------------------------------------------------
// ExAllocatePoolWithTagPriorityHook
//------------------------------------------------------------------------------
static PVOID NTAPI ExAllocatePoolWithTagPriorityHook(
    __in __drv_strictTypeMatch(__drv_typeCond) POOL_TYPE PoolType,
    __in SIZE_T NumberOfBytes,
    __in ULONG Tag,
    __in __drv_strictTypeMatch(__drv_typeExpr) EX_POOL_PRIORITY Priority)
{
	return ExAllocatePoolCommon(
		iExAllocatePoolWithTagPriority,
		_ReturnAddress(),
		PoolType,
		NumberOfBytes,
		Tag, Priority);
}

//------------------------------------------------------------------------------
// ExAllocatePoolWithQuotaTagHook
//------------------------------------------------------------------------------
static PVOID NTAPI ExAllocatePoolWithQuotaTagHook(
    __in __drv_strictTypeMatch(__drv_typeExpr) POOL_TYPE PoolType,
    __in SIZE_T NumberOfBytes,
    __in ULONG Tag)
{
	return ExAllocatePoolCommon(
		iExAllocatePoolWithQuotaTag,
		_ReturnAddress(),
		PoolType,
		NumberOfBytes,
		Tag, 0);
}

//------------------------------------------------------------------------------
// ExAllocatePoolWithTagHook
//------------------------------------------------------------------------------
static PVOID NTAPI ExAllocatePoolWithTagHook(
	__in __drv_strictTypeMatch(__drv_typeExpr) POOL_TYPE PoolType,
	__in SIZE_T NumberOfBytes,
	__in ULONG Tag)
{
	return ExAllocatePoolCommon(
		iExAllocatePoolWithTag,
		_ReturnAddress(),
		PoolType,
		NumberOfBytes,
		Tag, 0);
}
//------------------------------------------------------------------------------
// ExFreePoolWithTagHook
//------------------------------------------------------------------------------
static VOID NTAPI ExFreePoolWithTagHook(
	__in __drv_freesMem(Mem) PVOID P,
	__in ULONG Tag)
{
	Method * m = GetMonitorMethods(0);
	MonFree(P);
	ASSERT( KPTR(m[iExFreePoolWithTag].osapi) );
	((void (NTAPI *)(PVOID, ULONG))m[iExFreePoolWithTag].osapi)(P, Tag);
}

//------------------------------------------------------------------------------
// ExFreePoolHook
//------------------------------------------------------------------------------
static VOID NTAPI ExFreePoolHook(__in __drv_freesMem(Mem) PVOID P)
{
	Method * m = GetMonitorMethods(0);
	MonFree(P);
	ASSERT( KPTR(m[iExFreePool].osapi) );
	((void (NTAPI *)(PVOID))m[iExFreePool].osapi)(P);
}

//------------------------------------------------------------------------------
// CreateSystemThreadCommon
//------------------------------------------------------------------------------
static NTSTATUS NTAPI CreateSystemThreadCommon(
	__in HookApiIndex index,
	__in void * returnAddress,
	__in PVOID IoObject,
	__out PHANDLE ThreadHandle,
	__in ULONG DesiredAccess,
	__in_opt POBJECT_ATTRIBUTES ObjectAttributes,
	__in_opt  HANDLE ProcessHandle,
	__out_opt PCLIENT_ID ClientId,
	__in PKSTART_ROUTINE StartRoutine,
	__in_opt __drv_when(return==0, __drv_aliasesMem) PVOID StartContext)
{
	Method * m = GetMonitorMethods(0);
	void * ctx = MonCreateThreadBegin();
	CLIENT_ID clientId = {0};
	void * thread = 0;
	NTSTATUS status = 0;

	if( index == iPsCreateSystemThread )
		status = ((NTSTATUS (NTAPI *)(PHANDLE,ULONG,POBJECT_ATTRIBUTES,\
				HANDLE,PCLIENT_ID,PKSTART_ROUTINE,PVOID)) \
				m[iPsCreateSystemThread].osapi)(ThreadHandle,
									DesiredAccess,
									ObjectAttributes,
									ProcessHandle,
									&clientId,
									StartRoutine,
									StartContext);
	else if( index == iIoCreateSystemThread )
		status = ((NTSTATUS (NTAPI *)(PVOID, PHANDLE,ULONG,POBJECT_ATTRIBUTES,\
				HANDLE,PCLIENT_ID,PKSTART_ROUTINE,PVOID)) \
				m[iIoCreateSystemThread].osapi)(
									IoObject,
									ThreadHandle,
									DesiredAccess,
									ObjectAttributes,
									ProcessHandle,
									&clientId,
									StartRoutine,
									StartContext);
	else
		status = STATUS_INVALID_PARAMETER;

	if( NT_SUCCESS(status) ) {
		if( KPTR(ClientId) ) {
			__try {
				RtlMoveMemory(ClientId, &clientId, sizeof(CLIENT_ID));
			} __except(EXCEPTION_EXECUTE_HANDLER) {
				LOG_ERROR("ClientId parametr error");
			}
		}
		if( KPTR(ctx) && NT_SUCCESS(ObReferenceObjectByHandle(
											*ThreadHandle,
											THREAD_ALL_ACCESS,
											*PsThreadType,
											KernelMode,
											&thread,
											NULL)) ) {
			// use pointer (object pointer still valid - handle not closed) 
			ObDereferenceObject(thread);
		}
	}

	MonCreateThreadEnd(	ctx,
						thread,
						returnAddress,
						StartRoutine,
						clientId.UniqueThread);
	return status;
}

//------------------------------------------------------------------------------
// PsCreateSystemThreadHook
//------------------------------------------------------------------------------
static NTSTATUS NTAPI PsCreateSystemThreadHook(
	__out PHANDLE ThreadHandle,
	__in ULONG DesiredAccess,
	__in_opt POBJECT_ATTRIBUTES ObjectAttributes,
	__in_opt  HANDLE ProcessHandle,
	__out_opt PCLIENT_ID ClientId,
	__in PKSTART_ROUTINE StartRoutine,
	__in_opt __drv_when(return==0, __drv_aliasesMem) PVOID StartContext)
{
	return CreateSystemThreadCommon(
				iPsCreateSystemThread,
				_ReturnAddress(),
				0,
				ThreadHandle,
				DesiredAccess,
				ObjectAttributes,
				ProcessHandle,
				ClientId,
				StartRoutine,
				StartContext);
}

//------------------------------------------------------------------------------
// IoCreateSystemThreadHook
//------------------------------------------------------------------------------
static NTSTATUS NTAPI IoCreateSystemThreadHook(
    __inout PVOID IoObject,
	__out PHANDLE ThreadHandle,
	__in ULONG DesiredAccess,
	__in_opt POBJECT_ATTRIBUTES ObjectAttributes,
	__in_opt  HANDLE ProcessHandle,
	__out_opt PCLIENT_ID ClientId,
	__in PKSTART_ROUTINE StartRoutine,
	__in_opt __drv_when(return==0, __drv_aliasesMem) PVOID StartContext)
{
	return CreateSystemThreadCommon(
				iIoCreateSystemThread,
				_ReturnAddress(),
				IoObject,
				ThreadHandle,
				DesiredAccess,
				ObjectAttributes,
				ProcessHandle,
				ClientId,
				StartRoutine,
				StartContext);
}

//------------------------------------------------------------------------------
// PsCreateSystemThreadExHook
//------------------------------------------------------------------------------
static NTSTATUS NTAPI PsCreateSystemThreadExHook(
	__out PHANDLE ThreadHandle,
	__in ULONG ThreadExtraSize,
	__in ULONG KernelStackSize,
	__in ULONG TlsDataSize,
	__out_opt PULONG ThreadId,
	__in_opt __drv_when(return==0, __drv_aliasesMem) PVOID StartContext1,
	__in_opt __drv_when(return==0, __drv_aliasesMem) PVOID StartContext2,
	__in BOOLEAN CreateSuspended,
	__in BOOLEAN DebugStack,
	__in PKSTART_ROUTINE StartRoutine)
{
	Method * m = GetMonitorMethods(0);
	void * ctx = MonCreateThreadBegin();
	ULONG threadId = 0;
	void * thread = 0;
	NTSTATUS status = ((NTSTATUS (NTAPI *)(PHANDLE,ULONG,ULONG,\
				ULONG,PULONG,PVOID,PVOID,BOOLEAN,BOOLEAN,PKSTART_ROUTINE)) \
				m[iPsCreateSystemThreadEx].osapi)(
									ThreadHandle,
									ThreadExtraSize,
									KernelStackSize,
									TlsDataSize,
									&threadId,
									StartContext1,
									StartContext2,
									CreateSuspended,
									DebugStack,
									StartRoutine);
	if( NT_SUCCESS(status) ) {
		if( KPTR(ThreadId) ) {
			__try {
				*ThreadId = threadId;
			} __except(EXCEPTION_EXECUTE_HANDLER) {
				LOG_ERROR("ThreadId parametr error");
			}
		}

		if( KPTR(ctx) && NT_SUCCESS(ObReferenceObjectByHandle(*ThreadHandle,
												THREAD_ALL_ACCESS,
												*PsThreadType,
												KernelMode,
												&thread,
												NULL)) ) {
			// use pointer (object pointer still valid - handle not closed) 
			ObDereferenceObject(thread);
		}
	}

	MonCreateThreadEnd(	ctx,
						thread,
						_ReturnAddress(),
						StartRoutine,
						(void *)threadId);
	return status;
}

//------------------------------------------------------------------------------
// PsTerminateSystemThreadHook
//------------------------------------------------------------------------------
static NTSTATUS NTAPI PsTerminateSystemThreadHook(__in NTSTATUS ExitStatus)
{
	Method * m = GetMonitorMethods(0);
	MonClose(PsGetCurrentThread());
	return ((NTSTATUS (NTAPI *)(NTSTATUS)) \
		m[iPsTerminateSystemThread].osapi)(ExitStatus);
}
//------------------------------------------------------------------------------
// ZwCloseHooks
//------------------------------------------------------------------------------
static NTSTATUS NTAPI ZwCloseHook(__in HANDLE Handle)
{
	Method * m = GetMonitorMethods(0);
	NTSTATUS status = ((NTSTATUS (NTAPI *)(HANDLE))m[iZwClose].osapi)(Handle);
	if( NT_SUCCESS(status) && IS_KERNEL_HANDLE(Handle) )
		MonClose(Handle);
	return status;
}

//------------------------------------------------------------------------------
// NtCloseHooks
//------------------------------------------------------------------------------
static NTSTATUS NTAPI NtCloseHook(__in HANDLE Handle)
{
	Method * m = GetMonitorMethods(0);
	NTSTATUS status = ((NTSTATUS (NTAPI *)(HANDLE))m[iNtClose].osapi)(Handle);
	if( NT_SUCCESS(status) && IS_KERNEL_HANDLE(Handle) )
		MonClose(Handle);
	return status;
}

//------------------------------------------------------------------------------
// ObCloseHandleHook
//------------------------------------------------------------------------------
static NTSTATUS NTAPI ObCloseHandleHook(
	__in HANDLE Handle,
	__in KPROCESSOR_MODE PreviousMode)
{
	Method * m = GetMonitorMethods(0);
	NTSTATUS status = ((NTSTATUS (NTAPI *)(HANDLE,KPROCESSOR_MODE))\
			m[iObCloseHandle].osapi)(Handle, PreviousMode);
	if( NT_SUCCESS(status) && IS_KERNEL_HANDLE(Handle) )
		MonClose(Handle);
	return status;
}
//------------------------------------------------------------------------------
// CreateEventHookCommon
//------------------------------------------------------------------------------
static PKEVENT NTAPI CreateEventHookCommon(
	__in HookApiIndex index,
    __in void * returnAddress,
    __in  PUNICODE_STRING EventName,
    __out PHANDLE EventHandle)
{
	Method * m = GetMonitorMethods(0);
	void * ctx = MonCreateSyncBegin();
	PKEVENT event = ((PKEVENT (NTAPI *)(PUNICODE_STRING,PHANDLE)) \
						m[index].osapi)(EventName,
										EventHandle);
	MonCreateSyncEnd(ctx,
					(KPTR(event) && \
					KPTR(EventHandle) && \
					IS_KERNEL_HANDLE(*EventHandle)) ? \
						*EventHandle:0,
					EventSync,
					returnAddress,
					index == iIoCreateSynchronizationEvent ? TRUE:FALSE,
					TRUE);
	return event;
}

//------------------------------------------------------------------------------
// IoCreateNotificationEventHook
//------------------------------------------------------------------------------
static PKEVENT NTAPI IoCreateNotificationEventHook(
    __in  PUNICODE_STRING EventName,
    __out PHANDLE EventHandle)
{
	return CreateEventHookCommon(
			iIoCreateNotificationEvent,
			_ReturnAddress(),
			EventName,
			EventHandle);
}

//------------------------------------------------------------------------------
// IoCreateSynchronizationEventHook
//------------------------------------------------------------------------------
static PKEVENT NTAPI IoCreateSynchronizationEventHook(
	__in  PUNICODE_STRING EventName,
	__out PHANDLE EventHandle)
{
	return CreateEventHookCommon(
			iIoCreateSynchronizationEvent,
			_ReturnAddress(),
			EventName,
			EventHandle);
}

//------------------------------------------------------------------------------
// CreateNtZwEventCommon
//------------------------------------------------------------------------------
static NTSTATUS NTAPI CreateOpenNtZwEventCommon(
	__in HookApiIndex index,
    __in void * returnAddress,
    __out PHANDLE EventHandle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes,
    __in EVENT_TYPE EventType,
    __in BOOLEAN InitialState)
{
	Method * m = GetMonitorMethods(0);
	void * ctx = MonCreateSyncBegin();
	NTSTATUS status = 0;
	if( index == iNtCreateEvent || index == iZwCreateEvent )
		status = ((NTSTATUS (NTAPI *)(PHANDLE,ACCESS_MASK,\
				POBJECT_ATTRIBUTES,EVENT_TYPE,BOOLEAN))m[index].osapi)(
					EventHandle, DesiredAccess, 
					ObjectAttributes, EventType, InitialState);
	else {
		ASSERT( index == iNtOpenEvent || index == iZwOpenEvent );
		status = ((NTSTATUS (NTAPI *)(PHANDLE,ACCESS_MASK,\
				POBJECT_ATTRIBUTES))m[index].osapi)(
					EventHandle, DesiredAccess, 
					ObjectAttributes);
	}

	MonCreateSyncEnd(
					ctx,
					(NT_SUCCESS(status) && \
					KPTR(EventHandle) && \
					IS_KERNEL_HANDLE(*EventHandle)) ? \
						*EventHandle:0,
					EventSync,
					returnAddress,
					EventType == SynchronizationEvent ? TRUE:FALSE,
					InitialState);
	return status;
}

//------------------------------------------------------------------------------
// ZwCreateEventHook
//------------------------------------------------------------------------------
static NTSTATUS NTAPI ZwCreateEventHook(
    __out PHANDLE EventHandle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes,
    __in EVENT_TYPE EventType,
    __in BOOLEAN InitialState)
{
	return CreateOpenNtZwEventCommon(
				iZwCreateEvent,
				_ReturnAddress(),
				EventHandle,
				DesiredAccess,
				ObjectAttributes,
				EventType,
				InitialState);
}
//------------------------------------------------------------------------------
// NtCreateEventHook
//------------------------------------------------------------------------------
static NTSTATUS NTAPI NtCreateEventHook(
    __out PHANDLE EventHandle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes,
    __in EVENT_TYPE EventType,
    __in BOOLEAN InitialState)
{
	return CreateOpenNtZwEventCommon(
				iNtCreateEvent,
				_ReturnAddress(),
				EventHandle,
				DesiredAccess,
				ObjectAttributes,
				EventType,
				InitialState);
}
//------------------------------------------------------------------------------
// NtOpenEventHook
//------------------------------------------------------------------------------
static NTSTATUS NTAPI NtOpenEventHook(
	__out PHANDLE EventHandle,
	__in ACCESS_MASK DesiredAccess,
	__in POBJECT_ATTRIBUTES ObjectAttributes)
{
	return CreateOpenNtZwEventCommon(
				iNtOpenEvent,
				_ReturnAddress(),
				EventHandle,
				DesiredAccess,
				ObjectAttributes, 0, 0);
}

//------------------------------------------------------------------------------
// ZwOpenEventHook
//------------------------------------------------------------------------------
static NTSTATUS NTAPI ZwOpenEventHook(
	__out PHANDLE EventHandle,
	__in ACCESS_MASK DesiredAccess,
	__in POBJECT_ATTRIBUTES ObjectAttributes)
{
	return CreateOpenNtZwEventCommon(
				iZwOpenEvent,
				_ReturnAddress(),
				EventHandle,
				DesiredAccess,
				ObjectAttributes, 0, 0);
}

//------------------------------------------------------------------------------
// MmAllocateContiguousMemoryHook
//------------------------------------------------------------------------------
static PVOID NTAPI MmAllocateContiguousMemoryHook(
    __in SIZE_T NumberOfBytes,
    __in PHYSICAL_ADDRESS HighestAcceptableAddress)
{
	Method * m = GetMonitorMethods(0);
	void * ptr = 0;
	ASSERT( KPTR(m[iMmAllocateContiguousMemory].osapi) );
	ptr = ((void * (NTAPI *)(SIZE_T,PHYSICAL_ADDRESS)) \
			m[iMmAllocateContiguousMemory].osapi)(\
				NumberOfBytes,
				HighestAcceptableAddress);
	if( ptr )
		MonAlloc(ptr, _ReturnAddress(), NumberOfBytes, (ptr_t)0,
			MmContiguousMemoryMemType);
	return ptr;
}

//------------------------------------------------------------------------------
// MmAllocateContiguousMemorySpecifyCacheHook
//------------------------------------------------------------------------------
static PVOID NTAPI MmAllocateContiguousMemorySpecifyCacheHook(
    __in SIZE_T NumberOfBytes,
    __in PHYSICAL_ADDRESS LowestAcceptableAddress,
    __in PHYSICAL_ADDRESS HighestAcceptableAddress,
    __in_opt PHYSICAL_ADDRESS BoundaryAddressMultiple,
    __in MEMORY_CACHING_TYPE CacheType)
{
	Method * m = GetMonitorMethods(0);
	void * ptr = 0;
	ASSERT( KPTR(m[iMmAllocateContiguousMemorySpecifyCache].osapi) );
	ptr = ((void * (NTAPI *)(SIZE_T,PHYSICAL_ADDRESS,PHYSICAL_ADDRESS, \
			PHYSICAL_ADDRESS,MEMORY_CACHING_TYPE)) \
			m[iMmAllocateContiguousMemorySpecifyCache].osapi)(\
				NumberOfBytes,
				LowestAcceptableAddress,
				HighestAcceptableAddress,
				BoundaryAddressMultiple,
				CacheType);
	if( ptr )
		MonAlloc(ptr, _ReturnAddress(), NumberOfBytes, (ptr_t)CacheType,
			MmContiguousMemoryMemType);
	return ptr;
}

//------------------------------------------------------------------------------
// MmAllocateContiguousMemorySpecifyCacheNodeHook
//------------------------------------------------------------------------------
static PVOID NTAPI MmAllocateContiguousMemorySpecifyCacheNodeHook(
    __in SIZE_T NumberOfBytes,
    __in PHYSICAL_ADDRESS LowestAcceptableAddress,
    __in PHYSICAL_ADDRESS HighestAcceptableAddress,
    __in_opt PHYSICAL_ADDRESS BoundaryAddressMultiple,
    __in MEMORY_CACHING_TYPE CacheType,
    __in NODE_REQUIREMENT PreferredNode)
{
	Method * m = GetMonitorMethods(0);
	void * ptr = 0;
	ASSERT( KPTR(m[iMmAllocateContiguousMemorySpecifyCacheNode].osapi) );
	ptr = ((void * (NTAPI *)(SIZE_T,PHYSICAL_ADDRESS,PHYSICAL_ADDRESS, \
			PHYSICAL_ADDRESS,MEMORY_CACHING_TYPE,NODE_REQUIREMENT)) \
			m[iMmAllocateContiguousMemorySpecifyCacheNode].osapi)(\
				NumberOfBytes,
				LowestAcceptableAddress,
				HighestAcceptableAddress,
				BoundaryAddressMultiple,
				CacheType,
				PreferredNode);
	if( ptr )
		MonAlloc(ptr, _ReturnAddress(), NumberOfBytes, (ptr_t)CacheType,
			MmContiguousMemoryMemType);
	return ptr;
}

//------------------------------------------------------------------------------
// MmFreeContiguousMemoryHook
//------------------------------------------------------------------------------
static VOID NTAPI MmFreeContiguousMemoryHook(__in PVOID BaseAddress)
{
	Method * m = GetMonitorMethods(0);
	MonFree(BaseAddress);
	ASSERT( KPTR(m[iMmFreeContiguousMemory].osapi) );
	((void (NTAPI*)(PVOID)) \
			m[iMmFreeContiguousMemory].osapi)(BaseAddress);
}

//------------------------------------------------------------------------------
// MmFreeContiguousMemorySpecifyCacheHook
//------------------------------------------------------------------------------
static VOID NTAPI MmFreeContiguousMemorySpecifyCacheHook(
    __in_bcount (NumberOfBytes) PVOID BaseAddress,
    __in SIZE_T NumberOfBytes,
    __in MEMORY_CACHING_TYPE CacheType)
{
	Method * m = GetMonitorMethods(0);
	MonFree(BaseAddress);
	ASSERT( KPTR(m[iMmFreeContiguousMemorySpecifyCache].osapi) );
	((void (NTAPI*)(PVOID, SIZE_T, MEMORY_CACHING_TYPE)) \
			m[iMmFreeContiguousMemorySpecifyCache].osapi)(
				BaseAddress, NumberOfBytes, CacheType);
}

//------------------------------------------------------------------------------
// MmMapIoSpaceHook
//------------------------------------------------------------------------------
static PVOID NTAPI MmMapIoSpaceHook(
    __in PHYSICAL_ADDRESS PhysicalAddress,
    __in SIZE_T NumberOfBytes,
    __in MEMORY_CACHING_TYPE CacheType)
{
	Method * m = GetMonitorMethods(0);
	void * ptr = 0;
	ASSERT( KPTR(m[iMmMapIoSpace].osapi) );
	ptr = ((void * (NTAPI *)(PHYSICAL_ADDRESS,SIZE_T,MEMORY_CACHING_TYPE)) \
			m[iMmMapIoSpace].osapi)(\
				PhysicalAddress,
				NumberOfBytes,
				CacheType);
	if( ptr )
		MonAlloc(ptr, _ReturnAddress(), NumberOfBytes, (ptr_t)CacheType,
			IoMapMemType);
	return ptr;
}

//------------------------------------------------------------------------------
// MmUnmapIoSpaceHook
//------------------------------------------------------------------------------
static VOID NTAPI MmUnmapIoSpaceHook(
    __in_bcount (NumberOfBytes) PVOID BaseAddress,
    __in SIZE_T NumberOfBytes)
{
	Method * m = GetMonitorMethods(0);
	MonFree(BaseAddress);
	ASSERT( KPTR(m[iMmUnmapIoSpace].osapi) );
	((void (NTAPI*)(PVOID, SIZE_T)) \
			m[iMmUnmapIoSpace].osapi)(BaseAddress, NumberOfBytes);
}

//------------------------------------------------------------------------------
// IoGetDeviceInterfacesHook
//------------------------------------------------------------------------------
extern NTSTATUS NTAPI IoGetDeviceInterfacesHook(
    __in CONST GUID *InterfaceClassGuid,
    __in_opt PDEVICE_OBJECT PhysicalDeviceObject,
    __in ULONG Flags,
    __out PWSTR *SymbolicLinkList)
{
	Method * m = GetMonitorMethods(0);
	ASSERT( KPTR(m[iIoGetDeviceInterfaces].osapi) );
	NTSTATUS status = ((NTSTATUS (NTAPI *)(CONST GUID *,PDEVICE_OBJECT,ULONG, \
			PWSTR *)) \
			m[iIoGetDeviceInterfaces].osapi)(\
				InterfaceClassGuid,
				PhysicalDeviceObject,
				Flags,
				SymbolicLinkList);
	if( NT_SUCCESS(status) && \
		KPTR(SymbolicLinkList) && \
		KPTR(*SymbolicLinkList) ) {
		MonAlloc(*SymbolicLinkList, _ReturnAddress(), wcslen(*SymbolicLinkList),
			(ptr_t)'enoN', IoGetDeviceInterfacesMemType);
	}
	return status;
}

//------------------------------------------------------------------------------
// SeQueryInformationTokenHook
//------------------------------------------------------------------------------
static size_tr GetTokenInfoSize(TOKEN_INFORMATION_CLASS TokenInformationClass)
{
	switch(TokenInformationClass) {
		case TokenDefaultDacl:
			return sizeof(TOKEN_DEFAULT_DACL);
		case TokenGroups:
		case TokenRestrictedSids:
		case TokenLogonSid:
			return sizeof(TOKEN_GROUPS);
		case TokenCapabilities:
		case TokenDeviceGroups:
			return sizeof(TOKEN_GROUPS)+sizeof(SID_AND_ATTRIBUTES);
		case TokenImpersonationLevel:
			return sizeof(SECURITY_IMPERSONATION_LEVEL);
		case TokenOwner:
			return sizeof(TOKEN_OWNER);
		case TokenPrimaryGroup:
			return sizeof(TOKEN_PRIMARY_GROUP);
		case TokenPrivileges:
			return sizeof(TOKEN_PRIVILEGES);
		case TokenSessionId:
			return sizeof(ULONG);
		case TokenSource:
			return sizeof(TOKEN_SOURCE);
		case TokenStatistics:
			return sizeof(TOKEN_STATISTICS);
		case TokenType:
			return sizeof(TOKEN_TYPE);
		case TokenUser:
			return sizeof(TOKEN_USER);
		case TokenIntegrityLevel:
		case TokenSandBoxInert:
		case TokenVirtualizationAllowed:
		case TokenVirtualizationEnabled:
		case TokenUIAccess:
		case TokenIsAppContainer:
		case TokenAppContainerNumber:
		case TokenHasRestrictions:
			return sizeof(ULONG);
		case TokenAppContainerSid:
			return sizeof(TOKEN_APPCONTAINER_INFORMATION);
		case TokenUserClaimAttributes:
		case TokenDeviceClaimAttributes:
			return sizeof(CLAIM_SECURITY_ATTRIBUTES_INFORMATION);
		case TokenGroupsAndPrivileges:
			return sizeof(TOKEN_GROUPS_AND_PRIVILEGES);
		case TokenSessionReference:
		case TokenAuditPolicy:
			return sizeof(void *);
		case TokenOrigin:
			return sizeof(TOKEN_ORIGIN);
		case TokenElevationType:
			return sizeof(TOKEN_ELEVATION_TYPE);
		case TokenLinkedToken:
			return sizeof(TOKEN_LINKED_TOKEN);
		case TokenElevation:
			return sizeof(TOKEN_ELEVATION);
		case TokenAccessInformation:
			return sizeof(TOKEN_ACCESS_INFORMATION);
		case TokenMandatoryPolicy:
			return sizeof(TOKEN_MANDATORY_POLICY);
	}
	LOG_WARN("can`t detect info size for %u", TokenInformationClass);
	return sizeof(void *);
}

//------------------------------------------------------------------------------
// SeQueryInformationTokenHook
//------------------------------------------------------------------------------
static NTSTATUS NTAPI SeQueryInformationTokenHook(
    __in PACCESS_TOKEN Token,
    __in TOKEN_INFORMATION_CLASS TokenInformationClass,
    __deref_out PVOID *TokenInformation)
{
	Method * m = GetMonitorMethods(0);
	ASSERT( KPTR(m[iSeQueryInformationToken].osapi) );
	NTSTATUS status = ((NTSTATUS (NTAPI *)(PACCESS_TOKEN, \
			TOKEN_INFORMATION_CLASS, PVOID *)) \
			m[iSeQueryInformationToken].osapi)(\
				Token,
				TokenInformationClass,
				TokenInformation);
	if( NT_SUCCESS(status) && \
		KPTR(TokenInformation) && \
		KPTR(*TokenInformation) ) {
		MonAlloc(*TokenInformation, _ReturnAddress(), 
			GetTokenInfoSize(TokenInformationClass),
			(ptr_t)'enoN', SeQueryInformationTokenMemType);
	}
	return status;
}

//------------------------------------------------------------------------------
// GetImport
//------------------------------------------------------------------------------
static void * GetImport(__in __nullterminated const unsigned char * name)
{
	void * function = NULL;
	ANSI_STRING as = {0};
	UNICODE_STRING us = {0};

	RtlInitAnsiString(&as, (PCSZ)name);
	if( !NT_SUCCESS(RtlAnsiStringToUnicodeString(&us, &as, TRUE)) )
		return NULL;
	function = MmGetSystemRoutineAddress(&us);
	if( !function ) {
		LOG_WARN("MmGetSystemRoutineAddress(\"%s\") ... not found", name);
	}
	RtlFreeUnicodeString(&us);

	return function;
}

//------------------------------------------------------------------------------
// GetMonitorMethods
//------------------------------------------------------------------------------
extern Method * GetMonitorMethods(__out_opt unsigned long * totalMethods)
{
	static signed init = FALSE;
	static Method m[] = {\
		{0, 0, "ExAllocatePoolWithTag",         ExAllocatePoolWithTagHook},
		{0, 0, "ExFreePoolWithTag",             ExFreePoolWithTagHook},
		{0, 0, "ExAllocatePool",                ExAllocatePoolHook},
		{0, 0, "ExAllocatePoolWithQuota",       ExAllocatePoolWithQuotaHook},
		{0, 0, "ExAllocatePoolWithTagPriority", ExAllocatePoolWithTagPriorityHook},
		{0, 0, "ExAllocatePoolWithQuotaTag",    ExAllocatePoolWithQuotaTagHook},
		{0, 0, "ExFreePool",                    ExFreePoolHook},
		{0, 0, "MmAllocateContiguousMemory",    MmAllocateContiguousMemoryHook},
		{0, 0, "MmAllocateContiguousMemorySpecifyCache",  MmAllocateContiguousMemorySpecifyCacheHook},
		{0, 0, "MmAllocateContiguousMemorySpecifyCacheNode",  MmAllocateContiguousMemorySpecifyCacheNodeHook},
		{0, 0, "MmFreeContiguousMemory",        MmFreeContiguousMemoryHook},
		{0, 0, "MmFreeContiguousMemorySpecifyCache",  MmFreeContiguousMemorySpecifyCacheHook},
		{0, 0, "MmMapIoSpace",                  MmMapIoSpaceHook},
		{0, 0, "MmUnmapIoSpace",                MmUnmapIoSpaceHook},
		{0, 0, "PsCreateSystemThread",          PsCreateSystemThreadHook},        
		{0, 0, "PsTerminateSystemThread",       PsTerminateSystemThreadHook},     
		{0, 0, "IoCreateSystemThread",          IoCreateSystemThreadHook},        
		{0, 0, "PsCreateSystemThreadEx",        PsCreateSystemThreadExHook},      
		{0, 0, "ZwClose",                       ZwCloseHook},                     
		{0, 0, "NtClose",                       NtCloseHook},                     
		{0, 0, "ObCloseHandle",                 ObCloseHandleHook},               
		{0, 0, "IoCreateNotificationEvent",     IoCreateNotificationEventHook},   
		{0, 0, "IoCreateSynchronizationEvent",  IoCreateSynchronizationEventHook},
		{0, 0, "NtCreateEvent",                 NtCreateEventHook},               
		{0, 0, "ZwCreateEvent",                 ZwCreateEventHook},
		{0, 0, "NtOpenEvent",                 	NtOpenEventHook},               
		{0, 0, "ZwOpenEvent",                 	ZwOpenEventHook},
		{0, 0, "IoGetDeviceInterfaces",        	IoGetDeviceInterfacesHook},
		{0, 0, "SeQueryInformationToken",     	SeQueryInformationTokenHook}};

	if( totalMethods ) {
		if( !init ) {
			HookApiIndex index = 0;
			C_ASSERT( sizeof(m)/sizeof(m[0]) == iMax );
			while( index < iMax ) {
				m[index].osapi = GetImport(m[index].apiname);
				index++;
			}
			init = TRUE;
		}
		*totalMethods = iMax;
	}
	return (Method *)&m;
}
