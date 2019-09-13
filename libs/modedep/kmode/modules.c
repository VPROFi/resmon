#ifndef _KERNEL_MODE
#error This code only for kenel mode
#endif
#include <common/dbglog.h>
#include <common/checkptr.h>
#include <modedep/osdep.h>

#define TAG 'sdoM'

//------------------------------------------------------------------------------
// OsFreeModuleData
//------------------------------------------------------------------------------
extern void OsFreeModuleData(OsModuleData * mod)
{
	ASSERT( KPTR(mod->basename.str) );
	ExFreePoolWithTag(mod->basename.str, TAG);
	mod->basename.str = 0;
}

//------------------------------------------------------------------------------
// OsBaseAndDataFromPointer
//------------------------------------------------------------------------------
signed OsBaseAndDataFromPointer(void *ptr, OsModuleData * mod)
{
	RTL_PROCESS_MODULES * pm = 0;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	ULONG pmSize = PAGE_SIZE, index = 0;

	ASSERT( KPTR(mod) );
	mod->base = 0;
	mod->imageSize = 0;

	if( !KPTR(ptr) )
		ptr = _ReturnAddress();

	do {
		pm = (RTL_PROCESS_MODULES *)ExAllocatePoolWithTag(PagedPool, pmSize, TAG);
		if( !pm ) {
			status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}

		RtlZeroMemory(pm, pmSize);

		status = ZwQuerySystemInformation(
					SystemModuleInformation,
					pm,
					pmSize,
					&pmSize);
		if( NT_SUCCESS(status) )
			break;

		ExFreePoolWithTag(pm, TAG);
		pm = 0;

	} while( status == STATUS_INFO_LENGTH_MISMATCH );
	
	if( !NT_SUCCESS(status) )
		return FALSE;

	for(index=0 ; index < pm->NumberOfModules; index++ ) {
		if( ptr >= pm->Modules[index].ImageBase && \
			(char *)ptr < (((char *)pm->Modules[index].ImageBase) + \
				pm->Modules[index].ImageSize))
			break;
	}

	if( index < pm->NumberOfModules ) {
		ANSI_STRING as = {0};
		UNICODE_STRING us = {0};

		ASSERT( pm->Modules[index].OffsetToFileName <= \
			sizeof(pm->Modules[index].FullPathName) );

		RtlInitAnsiString(&as, \
			(PCSZ)&pm->Modules[index].FullPathName[pm->Modules[index].OffsetToFileName]);

		if( NT_SUCCESS(RtlAnsiStringToUnicodeString(&us, &as, TRUE)) ) {
			mod->basename.str = ExAllocatePoolWithTag(
								NonPagedPool,
								us.Length+sizeof(uni_char), TAG);
			if( mod->basename.str ) {
				RtlMoveMemory(mod->basename.str,
					us.Buffer, us.Length);
				mod->basename.size = us.Length;
				mod->basename.str[us.Length/sizeof(uni_char)] = 0;
				mod->base = pm->Modules[index].ImageBase;
				mod->imageSize = pm->Modules[index].ImageSize;
			}
			RtlFreeUnicodeString(&us);
		}
	}

	ExFreePoolWithTag(pm, TAG);
	return KPTR(mod->base) ? TRUE:FALSE;
}
