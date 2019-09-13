#include "ihook.h"
#include <common/checkptr.h>
#include <common/dbglog.h>

#if defined(_WIN64)
#define IMAGE_NT_OPTIONAL_HDR_MAGIC IMAGE_NT_OPTIONAL_HDR64_MAGIC
#else
#define IMAGE_NT_OPTIONAL_HDR_MAGIC IMAGE_NT_OPTIONAL_HDR32_MAGIC
#endif

extern PIMAGE_NT_HEADERS __stdcall RtlImageNtHeader(__in const void * base);
extern void * __stdcall RtlImageDirectoryEntryToData(__in void * base, __in BOOLEAN MappedAsImage, __in USHORT DirectoryEntry, __in PULONG Size);
//------------------------------------------------------------------------------
// IsDiscardable
//------------------------------------------------------------------------------
static signed IsDiscardable(OsModuleData * mod, unsigned long offset)
{
	unsigned long index = 0;
	PIMAGE_NT_HEADERS ntHeaders = RtlImageNtHeader(mod->base);
	const IMAGE_SECTION_HEADER * section = IMAGE_FIRST_SECTION(ntHeaders);
	for( index=0; index < ntHeaders->FileHeader.NumberOfSections; \
			index++, section++) {

		if( section->VirtualAddress > mod->imageSize || \
			section->VirtualAddress > offset )
			continue;

		if(	section->Misc.VirtualSize )
			if( (section->VirtualAddress + section->Misc.VirtualSize) < offset )
				continue;
		else if( section->SizeOfRawData )
			if( (section->VirtualAddress + section->SizeOfRawData) < offset )
				continue;


		if( section->Characteristics & IMAGE_SCN_MEM_DISCARDABLE || \
			(*(ULONG *)section->Name == (ULONG)'TINI') )
			break;
		return FALSE;
	}
	return TRUE;
}

//------------------------------------------------------------------------------
// SetImports
//------------------------------------------------------------------------------
static NTSTATUS SetImports(
			__in OsModuleData * mod,
			__in IMAGE_IMPORT_DESCRIPTOR * imp)
{
	IMAGE_THUNK_DATA * addrThunk = 0, * endThunk = 0;
	void * memCtx = 0;
	NTSTATUS status = STATUS_SUCCESS;
	unsigned long totalMethods = 0;
	Method * methods = GetMonitorMethods(&totalMethods);

	ASSERT( PTR(methods) );
	ASSERT( totalMethods > 0 );

	if( imp->FirstThunk >= mod->imageSize ) {
		LOG_ERROR("import descriptor error");
		return STATUS_INVALID_IMAGE_FORMAT;
	}

	if( IsDiscardable(mod, imp->FirstThunk) ) {
		LOG_ERROR("imp->FirstThunk 0x%08X discardable - can`t set hooks", \
			imp->OriginalFirstThunk, imp->FirstThunk);
		return STATUS_NOT_IMPLEMENTED;
	}

	addrThunk = (IMAGE_THUNK_DATA *)((char *)mod->base + (ULONG)imp->FirstThunk);
	endThunk = (IMAGE_THUNK_DATA *)((char *)mod->base + mod->imageSize);

	while( PTR((void *)addrThunk->u1.Function) ) {
		unsigned long i = 0;
		while( i < totalMethods ) {
			if( methods[i].osapi == (void *)addrThunk->u1.Function ) {
				ASSERT( methods[i].osapiptr == 0 );
				if( !methods[i].osapiptr ) {
					void * rwptr = 0;
					ASSERT( PTR(methods[i].apihook) );
    				if( GetMemoryAccess(&addrThunk->u1.Function, &rwptr, &memCtx) ) {
						*(void **)rwptr = methods[i].apihook;
						methods[i].osapiptr = &addrThunk->u1.Function;
						LOG_INFO("%s set hook (0x%p->0x%p) at 0%p", \
							methods[i].apiname, \
							methods[i].osapi, \
							methods[i].apihook, \
							methods[i].osapiptr);
						break;
					}
				} else {
					LOG_WARN("hook for %s (0x%p->0x%p), already set at 0x%p," \
							" skip set position at 0x%p", \
							methods[i].apiname, \
							methods[i].osapi, \
							methods[i].apihook, \
							methods[i].osapiptr,
							&addrThunk->u1.Function);
					break;
				}
			}
			i++;
		}
		addrThunk++;
		if( addrThunk >= endThunk ) {
			LOG_ERROR("thunk chain over image end");
			status = STATUS_INVALID_IMAGE_FORMAT;
			break;
		}
	}

	if( memCtx )
		RestoreMemoryAccess(memCtx);

	return status;
}

//------------------------------------------------------------------------------
// SetImportHooks
//------------------------------------------------------------------------------
extern NTSTATUS SetImportHooks(__in OsModuleData * mod)
{
	NTSTATUS status = STATUS_INVALID_IMAGE_FORMAT;

	ASSERT( PTR(mod->base) );
	ASSERT( mod->imageSize > 0 );

	do {
		__try {

			unsigned long impSize = 0;
			IMAGE_IMPORT_DESCRIPTOR * imp = 0;
			PIMAGE_NT_HEADERS ntHeaders = 0;

			if( mod->imageSize < sizeof(IMAGE_NT_HEADERS) ) {
				LOG_ERROR("incorrect image");
				break;
			}

			ntHeaders = RtlImageNtHeader(mod->base);
			if( ntHeaders->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC ) {
				LOG_ERROR("incorrect image");
				break;
			}
			if( ntHeaders->OptionalHeader.SizeOfImage != mod->imageSize ) {
				LOG_ERROR("incorrect image size");
				break;
			}

			imp = (IMAGE_IMPORT_DESCRIPTOR *)RtlImageDirectoryEntryToData(
							mod->base,
							TRUE,
							IMAGE_DIRECTORY_ENTRY_IMPORT,
							&impSize);
			if( imp == 0 ) {
				LOG_WARN("image no imports");
				status = STATUS_SUCCESS;
				break;
			}

			while( impSize >= sizeof(IMAGE_IMPORT_DESCRIPTOR) && imp->Characteristics ) {
				status = SetImports(mod, imp);
				if( !NT_SUCCESS(status) )
					break;
				imp++;
				impSize -= sizeof(IMAGE_IMPORT_DESCRIPTOR);
			}
		#pragma warning(suppress: 6320 )
		} __except(EXCEPTION_EXECUTE_HANDLER) {
			LOG_ERROR("... exception 0x%08X", \
				GetExceptionCode());
			status = STATUS_NONCONTINUABLE_EXCEPTION;
		}
	#pragma warning( suppress:4127 )
	} while(0);

	if( !NT_SUCCESS(status) )
		RestoreImportHooks();

	return status;
}

//------------------------------------------------------------------------------
// RestoreImportHooks
//------------------------------------------------------------------------------
extern void RestoreImportHooks(void)
{
	unsigned int i = 0;
	void * memCtx = 0;
	unsigned long totalMethods = 0;
	Method * methods = GetMonitorMethods(&totalMethods);

	ASSERT( PTR(methods) );
	ASSERT( totalMethods > 0 );

	while( i < totalMethods ) {
		void * rwptr = 0;
		if( PTR(methods[i].osapiptr) ) {
			ASSERT( PTR(methods[i].osapi) );
			if( GetMemoryAccess(methods[i].osapiptr, &rwptr, &memCtx) ) {
				*(void **)rwptr = methods[i].osapi;
				methods[i].osapiptr = 0;
			}
		}
		i++;
	}
	if( memCtx )
		RestoreMemoryAccess(memCtx);
}
