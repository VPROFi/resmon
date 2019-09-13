#ifndef __CHECKPTR_H__
#define __CHECKPTR_H__

#include <modedep/modedep.h>
//------------------------------------------------------------------------------
// pointer check api
//------------------------------------------------------------------------------
typedef enum _PointerType {
	NotUseUnknown,		// do not use
	NonPagedPoolPointer,
	PagedPoolPointer,
	UserModePointer,
	MaximumPointerType
} PointerType;
EXTERN_C signed PointerCheck(const void * const ptr, PointerType type);
EXTERN_C void * GetLowValidUPtr(void);
EXTERN_C void * GetHighValidUPtr(void);
EXTERN_C void * GetLowValidKPtr(void);
EXTERN_C void * GetHighValidKPtr(void);
EXTERN_C signed ValidPtr(void * p);

//------------------------------------------------------------------------------
// fast check
//------------------------------------------------------------------------------
#define UPTR(p) \
		((void *)(p) >= GetLowValidUPtr() && \
		(void *)(p) <= GetHighValidUPtr())
#define KPTR(p) \
		((void *)(p) >= GetLowValidKPtr() && \
		(void *)(p) <= GetHighValidKPtr())
#define PTR(p) ValidPtr((p))
//------------------------------------------------------------------------------
// slow check
//------------------------------------------------------------------------------
#define PTR_USER(p) PointerCheck((const void * const)(p), UserModePointer)
#ifdef _KERNEL_MODE
#define PTR_NONPAGED(p) PointerCheck((const void * const)(p), NonPagedPoolPointer)
#define PTR_PAGED(p) PointerCheck((const void * const)(p), PagedPoolPointer)
#endif

#endif // __CHECKPTR_H__
