#ifdef _KERNEL_MODE
#error This code only for user mode
#endif

#include <common/dbglog.h>
#include <common/checkptr.h>
#include <tchar.h>

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
	MEMORY_BASIC_INFORMATION * mbi = (MEMORY_BASIC_INFORMATION *)ctx;

	ASSERT( UPTR(mbi) );
	ASSERT( UPTR(mbi->BaseAddress) );
	ASSERT( mbi->RegionSize != 0 );

	if( !VirtualProtect(mbi->BaseAddress, \
						mbi->RegionSize, \
						mbi->Protect, \
						&mbi->Protect) ) {
		LOG_ERROR("VirtualProtect() restore for 0x%p error %u", \
			mbi->AllocationBase, GetLastError());
		return FALSE;
	}
	HeapFree(GetProcessHeap(), 0, ctx);
	return TRUE;
}

//------------------------------------------------------------------------------
// GetMemoryAccess
//------------------------------------------------------------------------------
extern signed GetMemoryAccess(void * ptr, void ** rwptr, void ** ctx)
{
	MEMORY_BASIC_INFORMATION * mbi = 0;

	ASSERT( UPTR(ptr) );
	ASSERT( UPTR(rwptr) );
	ASSERT( UPTR(ctx) );

    *rwptr = ptr;

	if( !UPTR(*ctx) ) {
		mbi = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MEMORY_BASIC_INFORMATION));
	} else {
		mbi = (MEMORY_BASIC_INFORMATION *)*ctx;

		if( mbi->BaseAddress <= ptr && \
			((char *)mbi->BaseAddress+mbi->RegionSize) >= (char *)ptr+sizeof(void *) )
			return TRUE;

		// restore old region if exists
		RestoreMemoryAccess(mbi);
	}

	if( VirtualQuery(ptr, mbi, sizeof(*mbi)) != sizeof(*mbi) ) {
		LOG_ERROR("VirtualQuery(0x%p) error %u\n", ptr, GetLastError());
		HeapFree(GetProcessHeap(), 0, mbi);
		return FALSE;
	}

	ASSERT( ptr >= mbi->BaseAddress && \
		((char *)ptr+sizeof(void *)) <= \
		((char *)mbi->BaseAddress + mbi->RegionSize) );

	*ctx = mbi;

	if( mbi->Protect == PAGE_EXECUTE_WRITECOPY )
		return TRUE;

	if( !VirtualProtect(mbi->BaseAddress, \
						mbi->RegionSize, \
						PAGE_EXECUTE_WRITECOPY, \
						&mbi->Protect) ) {
		LOG_ERROR("VirtualProtect(PAGE_EXECUTE_WRITECOPY) for 0x%p error %u\n", \
			mbi->BaseAddress, GetLastError());
		*ctx = 0;
		HeapFree(GetProcessHeap(), 0, mbi);
		return FALSE;
	}
	return TRUE;
}
