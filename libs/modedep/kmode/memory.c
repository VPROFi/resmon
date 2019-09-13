#ifndef _KERNEL_MODE
#error This code only for kenel mode
#endif

#include <common/dbglog.h>
#include <common/checkptr.h>
//------------------------------------------------------------------------------
// OsMoveMemory
//------------------------------------------------------------------------------
void OsMoveMemory(void * dst, void *src, size_tr size)
{
	RtlMoveMemory(dst, src, size);
}
//------------------------------------------------------------------------------
// RestoreMemoryAccess
//------------------------------------------------------------------------------
extern signed RestoreMemoryAccess(void * ctx)
{
	MDL * mdl = (MDL *)ctx;
	ASSERT( PTR_NONPAGED(mdl) );
	ASSERT( mdl->Size >= sizeof(MDL) );
	ASSERT( KPTR(mdl->MappedSystemVa) );
	ASSERT( mdl->MdlFlags & (MDL_MAPPED_TO_SYSTEM_VA | MDL_SOURCE_IS_NONPAGED_POOL) );
	ASSERT( mdl->MdlFlags & MDL_PAGES_LOCKED );
	ASSERT( mdl->ByteOffset <= mdl->ByteCount );

    if( mdl->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA )
		MmUnmapLockedPages(mdl->MappedSystemVa, mdl);
    if( mdl->MdlFlags & MDL_PAGES_LOCKED )
		MmUnlockPages(mdl);
	IoFreeMdl(mdl);
	return TRUE;
}

//------------------------------------------------------------------------------
// GetMemoryAccess
//------------------------------------------------------------------------------
extern signed GetMemoryAccess(void * ptr, void ** rwptr, void ** ctx)
{
	MDL * mdl = 0;

	ASSERT( KPTR(ptr) );
	ASSERT( KPTR(rwptr) );
	ASSERT( KPTR(ctx) );

	if( !KPTR(*ctx) ) {
		mdl = IoAllocateMdl(ptr, PAGE_SIZE, FALSE, FALSE, NULL);
		if( !mdl ) {
			LOG_ERROR("can`t allocate mdl");
			return FALSE;
		}
	} else {
		mdl = (MDL *)*ctx;
		ASSERT( PTR_NONPAGED(mdl) );
		ASSERT( mdl->Size >= sizeof(MDL) );
		ASSERT( KPTR(mdl->MappedSystemVa) );
		ASSERT( mdl->MdlFlags & (MDL_MAPPED_TO_SYSTEM_VA | MDL_SOURCE_IS_NONPAGED_POOL) );
		ASSERT( mdl->MdlFlags & MDL_PAGES_LOCKED );
		ASSERT( mdl->ByteOffset <= mdl->ByteCount );
		if( mdl->StartVa <= ptr && \
			((char *)mdl->StartVa+mdl->ByteCount) >= (char *)ptr+sizeof(void *) ) {
			ASSERT( ((ULONG_PTR)((char *)ptr - (char *)mdl->StartVa) + (mdl->ByteOffset)) < mdl->ByteCount );
			*rwptr = (char *)mdl->MappedSystemVa - (mdl->ByteOffset) + ((char *)ptr - (char *)mdl->StartVa);
			return TRUE;
		}
		RestoreMemoryAccess(mdl);
	}

	__try {
		MmProbeAndLockPages(mdl, KernelMode, IoWriteAccess);
	} __except(EXCEPTION_EXECUTE_HANDLER) {
		LOG_ERROR("MmProbeAndLockPages()" \
			"exception 0x%08X",	GetExceptionCode());
		IoFreeMdl(mdl);
		return FALSE;
	}

	*rwptr = MmGetSystemAddressForMdlSafe(mdl, NormalPagePriority);
	if( !(*rwptr) ) {
		LOG_ERROR("can`t get system address");
		MmUnlockPages(mdl);
		IoFreeMdl(mdl);
		return FALSE;
	}
	*ctx = mdl;
	return TRUE;
}
