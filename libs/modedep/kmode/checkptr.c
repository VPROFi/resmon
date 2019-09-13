#ifndef _KERNEL_MODE
#error This code only for kenel mode
#endif

#include <common/dbglog.h>
#include <common/checkptr.h>
//==============================================================================
// pointer api
//------------------------------------------------------------------------------
// GetLowValidUPtr
//------------------------------------------------------------------------------
extern void * GetLowValidUPtr(void)
{
	return MM_LOWEST_USER_ADDRESS;
}
//------------------------------------------------------------------------------
// GetHighValidUPtr
//------------------------------------------------------------------------------
extern void * GetHighValidUPtr(void)
{
	return MM_HIGHEST_USER_ADDRESS;
}
//------------------------------------------------------------------------------
// GetLowValidKPtr
//------------------------------------------------------------------------------
extern void * GetLowValidKPtr(void)
{
	return MM_SYSTEM_RANGE_START;
}
//------------------------------------------------------------------------------
// GetHighValidKPtr
//------------------------------------------------------------------------------
extern void * GetHighValidKPtr(void)
{
#ifndef MM_SYSTEM_SPACE_END
#define MM_SYSTEM_SPACE_END 0xFFFFFFFFFFFFFFFFUI64
#endif
	return (void *)MM_SYSTEM_SPACE_END;
}

//------------------------------------------------------------------------------
// PointerCheck
//------------------------------------------------------------------------------
extern __checkReturn signed PointerCheck(__in const void * const ptr, PointerType type)
{
	if ( type != UserModePointer && ptr < MM_SYSTEM_RANGE_START ) {
		LOG_ERROR("address %p with type != UserModePointer in user mode space.", ptr);
		return FALSE;
    }

	switch (type) {

		// 1. MmIsAddressValid == TRUE
		// 2. MmIsNonPagedSystemAddressValid == TRUE
		case NonPagedPoolPointer:
			if( !MmIsAddressValid((void *)ptr) )
			return FALSE;
			#pragma warning( push )
			// 'MmIsNonPagedSystemAddressValid' was declared deprecated
			#pragma warning(disable: 4996)
			#if defined(_PREFAST_)
			// Consider using '(see documentation)' instead of 
			// 'MmIsNonPagedSystemAddressValid'. Reason: Obsolete.
			// MmIsNonPagedSystemAddressValid only valid insize __cli _sti asm block
			#pragma warning(disable: 28159)
			#endif
			if( !MmIsNonPagedSystemAddressValid((void *)ptr) )
				return FALSE;
			return TRUE;

		// 1. Use only if IRQL < DISPATCH_LEVEL
		// 2. MmIsNonPagedSystemAddressValid == FALSE
		case PagedPoolPointer:

			if( KeGetCurrentIrql() >= DISPATCH_LEVEL )
			return FALSE;
			// 2. Is this ptr NonPaged memory
			if( MmIsNonPagedSystemAddressValid((void *)ptr) )
				return FALSE; 
			#pragma warning( pop )
			return TRUE;

		// 1. Use only if IRQL < DISPATCH_LEVEL
		// 2. ptr in range from MM_LOWEST_USER_ADDRESS to MM_HIGHEST_USER_ADDRESS
		case UserModePointer:
			if( KeGetCurrentIrql() >= DISPATCH_LEVEL )
				return FALSE;
			if( ptr > MM_HIGHEST_USER_ADDRESS || ptr < MM_LOWEST_USER_ADDRESS )
				return FALSE;
			return TRUE;

		default:
			return FALSE;
	}
}

//------------------------------------------------------------------------------
// ValidPtr
//------------------------------------------------------------------------------
extern signed ValidPtr(void * p)
{
	return KPTR(p);
}
