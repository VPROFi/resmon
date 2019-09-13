#ifdef _KERNEL_MODE
#error This code only for user mode
#endif

#include <common/checkptr.h>
//==============================================================================
// pointer api
//------------------------------------------------------------------------------
// GetLowHighUPtr
//------------------------------------------------------------------------------
static void * GetLowHighUPtr( signed getLow )
{
	static SYSTEM_INFO sinf = {0};
	if( !sinf.lpMaximumApplicationAddress )
        GetSystemInfo(&sinf);

	if( getLow )
		return sinf.lpMinimumApplicationAddress;

	return sinf.lpMaximumApplicationAddress;
}

//------------------------------------------------------------------------------
// GetLowValidUPtr
//------------------------------------------------------------------------------
void * GetLowValidUPtr(void)
{
	return GetLowHighUPtr( TRUE );
}

//------------------------------------------------------------------------------
// GetHighValidUPtr
//------------------------------------------------------------------------------
void * GetHighValidUPtr(void)
{
	return GetLowHighUPtr( FALSE );
}

//------------------------------------------------------------------------------
// PointerCheck
//------------------------------------------------------------------------------
__checkReturn signed PointerCheck(__in const void * const ptr, PointerType type)
{
	static SYSTEM_INFO sinf	= {0};
	SIZE_T returnedBytes = 0;
	static MEMORY_BASIC_INFORMATION mbi = {0};

	if( type != UserModePointer )
		return FALSE;

	if( !sinf.lpMaximumApplicationAddress )
		GetSystemInfo(&sinf);

	if( ptr < sinf.lpMinimumApplicationAddress ||
		ptr > sinf.lpMaximumApplicationAddress )
		return FALSE;

	returnedBytes = VirtualQuery(ptr, &mbi, sizeof(mbi));

	if( returnedBytes != sizeof(mbi) )
        return FALSE;

	if( mbi.State & (MEM_FREE | MEM_RESERVE) )
		return FALSE;

	return TRUE;
}

//------------------------------------------------------------------------------
// ValidPtr
//------------------------------------------------------------------------------
extern signed ValidPtr(void * p)
{
	return UPTR(p);
}
