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
// OsModuleInfo
//------------------------------------------------------------------------------
static signed OsModuleInfo(void *ptr, ANSI_STRING * as, OsModuleData * mod)
{
	RTL_PROCESS_MODULES * pm = 0;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	ULONG pmSize = PAGE_SIZE, index = 0;
	ANSI_STRING mname = {0};

	ASSERT( KPTR(mod) );
	mod->base = 0;
	mod->imageSize = 0;

	if( !KPTR(ptr) && !KPTR(as) )
		ptr = _ReturnAddress();

	ASSERT( (KPTR(ptr) && !KPTR(as)) || (!KPTR(ptr) && KPTR(as)) );

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

		if( pm->Modules[index].OffsetToFileName >= \
				sizeof(pm->Modules[index].FullPathName) )
			continue;

		if( KPTR(as) ) {
			RtlInitAnsiString(&mname, \
				(PCSZ)&pm->Modules[index].FullPathName[pm->Modules[index].OffsetToFileName]);
			if( RtlEqualString(as, &mname, TRUE) )
				break;
		}

		if( (KPTR(ptr) && (ptr >= pm->Modules[index].ImageBase && \
			(char *)ptr < (((char *)pm->Modules[index].ImageBase) + \
				pm->Modules[index].ImageSize))) ) 
			break;
	}

	if( index < pm->NumberOfModules ) {
		UNICODE_STRING us = {0};

		ASSERT( pm->Modules[index].OffsetToFileName < \
			sizeof(pm->Modules[index].FullPathName) );

		if( !as )
			RtlInitAnsiString(&mname, \
				(PCSZ)&pm->Modules[index].FullPathName[pm->Modules[index].OffsetToFileName]);

		ASSERT( KPTR(mname.Buffer) );
		ASSERT( mname.Length >= sizeof(WCHAR) );

		if( NT_SUCCESS(RtlAnsiStringToUnicodeString(&us, &mname, TRUE)) ) {
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

//------------------------------------------------------------------------------
// OsModuleInfoFromPointer
//------------------------------------------------------------------------------
extern signed OsModuleInfoFromPointer(void *ptr, OsModuleData * mod)
{
	return OsModuleInfo(ptr, (ANSI_STRING *)0, mod);
}

//------------------------------------------------------------------------------
// OsModuleInfoFromPointer
//------------------------------------------------------------------------------
extern signed OsModuleInfoFromName(UniStrConst *name, OsModuleData * mod)
{
	signed res = FALSE;
	UNICODE_STRING us = {(USHORT)name->size, \
						(USHORT)name->size+sizeof(WCHAR), \
						(PWCH)name->str};
	ANSI_STRING as = {0};

	ASSERT( KPTR(name) );
	ASSERT( KPTR(name->str) );
	ASSERT( name->size >= sizeof(uni_char) );

	if( !NT_SUCCESS(RtlUnicodeStringToAnsiString(&as, &us, TRUE) ) )
		return FALSE;

	res = OsModuleInfo((void *)0, &as, mod);
	RtlFreeAnsiString(&as);
	return res;
}
