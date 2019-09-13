#include <shared/config.h>
#include <common/dbglog.h>
#include <common/checkptr.h>
#include <resmon/resmonlib.h>
#include <ntimage.h>

DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD DriverUnload;
static NTSTATUS IsFilePresent(__inout UNICODE_STRING * fullPath);
static NTSTATUS CreateGdiInfo(__in UNICODE_STRING * registryPath);

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, DriverUnload)
#pragma alloc_text (PAGE, IsFilePresent)
#pragma alloc_text (PAGE, CreateGdiInfo)
#endif

#define MAX_UNICODE_STRING_SIZE (65535)
#define KEY_INFO_SIZE (sizeof(KEY_VALUE_FULL_INFORMATION)+MAX_UNICODE_STRING_SIZE)

typedef struct _SYSTEM_GDI_DRIVER_INFORMATION
{
    UNICODE_STRING  DriverName;
    PVOID  ImageAddress; 
    PVOID  SectionPointer; 
	NTSTATUS (NTAPI * driverEntry)(__in struct _DRIVER_OBJECT * drv, __in PUNICODE_STRING reg);
    PIMAGE_EXPORT_DIRECTORY  ExportSectionPointer;
    ULONG  ImageLength;
} SYSTEM_GDI_DRIVER_INFORMATION, *PSYSTEM_GDI_DRIVER_INFORMATION;

#define SYSTEM_LOAD_GDI_DRIVER 54
#define SYSTEM_UNLOAD_GDI_DRIVER 27

NTSTATUS
NTAPI
ZwSetSystemInformation(
    __in ULONG SystemInformationClass,
    __in_bcount_opt(SystemInformationLength) PVOID SystemInformation,
    __in ULONG SystemInformationLength
    );

static struct {
	KEY_VALUE_FULL_INFORMATION * keyInfo;
	SYSTEM_GDI_DRIVER_INFORMATION sgdi;
	void (NTAPI * driverUnload)(__in struct _DRIVER_OBJECT * driverObject);
} ldr = {0};

#define SystemModuleInformation 11
typedef struct _RTL_PROCESS_MODULE_INFORMATION {
	HANDLE Section;
	PVOID MappedBase;
	PVOID ImageBase;
	ULONG ImageSize;
	ULONG Flags;
	USHORT LoadOrderIndex;
	USHORT InitOrderIndex;
	USHORT LoadCount;
	USHORT OffsetToFileName;
	UCHAR  FullPathName[ 256 ];
} RTL_PROCESS_MODULE_INFORMATION, *PRTL_PROCESS_MODULE_INFORMATION;

typedef struct _RTL_PROCESS_MODULES {
	ULONG NumberOfModules;
	RTL_PROCESS_MODULE_INFORMATION Modules[ 1 ];
} RTL_PROCESS_MODULES, *PRTL_PROCESS_MODULES;

NTSTATUS
NTAPI
ZwQuerySystemInformation(
    __in ULONG SystemInformationClass,
    __inout_bcount_opt(SystemInformationLength) PVOID SystemInformation,
    __in ULONG SystemInformationLength,
	__out PULONG ReturnLength
    );

#define DRVPATH_PREFIX L"\\SystemRoot\\System32\\Drivers\\"
#define DRVPATH_PREFIX_SIZE (sizeof(DRVPATH_PREFIX) - sizeof(WCHAR))
#define DRVPATH_SUFFIX L".check"
#define DRVPATH_SUFFIX_SIZE (sizeof(DRVPATH_SUFFIX) - sizeof(WCHAR))
//------------------------------------------------------------------------------
// IsFilePresent
//------------------------------------------------------------------------------
static NTSTATUS IsFilePresent(__inout UNICODE_STRING * fullPath)
{
	NTSTATUS status = STATUS_SUCCESS;
	HANDLE fileHandle = NULL;
	OBJECT_ATTRIBUTES oa = {0};
	IO_STATUS_BLOCK ioStatus = {0};

	if( (fullPath->Length + DRVPATH_SUFFIX_SIZE) > fullPath->MaximumLength )
		return STATUS_UNSUCCESSFUL;
	RtlMoveMemory(&fullPath->Buffer[fullPath->Length/sizeof(WCHAR)], DRVPATH_SUFFIX, DRVPATH_SUFFIX_SIZE);
	fullPath->Length += DRVPATH_SUFFIX_SIZE;

	InitializeObjectAttributes(
				&oa,
				fullPath,
				OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
				NULL,
				NULL);

	status = IoCreateFile(
				&fileHandle,
				0,
				&oa,
				&ioStatus,
				NULL,
				0,
				0,
				FILE_OPEN,
				FILE_SYNCHRONOUS_IO_NONALERT |\
				FILE_NON_DIRECTORY_FILE |\
				FILE_COMPLETE_IF_OPLOCKED |\
				FILE_OPEN_FOR_BACKUP_INTENT,
				NULL,
				0,
				CreateFileTypeNone,
				NULL,
				IO_NO_PARAMETER_CHECKING );
	if( NT_SUCCESS(status) ) {
		ZwClose(fileHandle);
		return status;
	}

	return status;
}

//------------------------------------------------------------------------------
// CreateGdiInfo
//------------------------------------------------------------------------------
static NTSTATUS CreateGdiInfo(__in UNICODE_STRING * registryPath)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	OBJECT_ATTRIBUTES oa = {0};
	HANDLE keyHandle = 0;

	InitializeObjectAttributes(&oa,	registryPath, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,	NULL, NULL);
	status = ZwOpenKey(&keyHandle, KEY_READ, &oa);
	if( !NT_SUCCESS(status) ) {
		LOG_ERROR("ZwOpenKey(%wZ) failure status 0x%08X", registryPath, status);
		return status;
	}

	do {
		ULONG infoSize = sizeof(KEY_VALUE_FULL_INFORMATION);
		UNICODE_STRING us = {0};
		RtlInitUnicodeString(&us, L"ImagePath");

		do {
			ldr.keyInfo = (KEY_VALUE_FULL_INFORMATION *)ExAllocatePoolWithTag(PagedPool, infoSize+DRVPATH_PREFIX_SIZE+DRVPATH_SUFFIX_SIZE, (ULONG)'DAOL');
			if( !ldr.keyInfo ) {
				LOG_ERROR("ExAllocatePoolWithTag(size = %u) failure", infoSize+DRVPATH_PREFIX_SIZE+DRVPATH_SUFFIX_SIZE);
				status = STATUS_INSUFFICIENT_RESOURCES;
				break;
			}

			RtlZeroMemory(ldr.keyInfo, infoSize);

			status = ZwQueryValueKey(keyHandle,
									&us,
									KeyValueFullInformation,
									ldr.keyInfo,
									infoSize,
									&infoSize);

			if( NT_SUCCESS(status) )
				break;

			ExFreePoolWithTag(ldr.keyInfo, (ULONG)'DAOL');
			ldr.keyInfo = 0;

		} while(status == STATUS_INFO_LENGTH_MISMATCH || status == STATUS_BUFFER_OVERFLOW);

		if( !NT_SUCCESS(status) ) {
			LOG_ERROR("ZwQueryValueKey(%wZ\\%wZ) failure status 0x%08X", registryPath, &us, status);
			break;
		}

		if( (ldr.keyInfo->Type != REG_EXPAND_SZ && ldr.keyInfo->Type != REG_SZ) || \
			ldr.keyInfo->DataLength < sizeof(WCHAR)*2 || ldr.keyInfo->DataLength > MAX_UNICODE_STRING_SIZE ) {
			LOG_ERROR("ZwQueryValueKey(%wZ\\%wZ) incorrect type %u or size %u", registryPath, &us, ldr.keyInfo->Type, ldr.keyInfo->DataLength);
			status = STATUS_OBJECT_TYPE_MISMATCH;
			break;
		}

		ldr.sgdi.DriverName.Buffer = (PWCH)(((PUCHAR)ldr.keyInfo)+ldr.keyInfo->DataOffset);
		ldr.sgdi.DriverName.Length = (USHORT)ldr.keyInfo->DataLength-sizeof(WCHAR);
		ldr.sgdi.DriverName.MaximumLength = ldr.sgdi.DriverName.Length+DRVPATH_PREFIX_SIZE+DRVPATH_SUFFIX_SIZE;

		if( ldr.sgdi.DriverName.Buffer[0] != '\\' ) {
			LONG index = ldr.sgdi.DriverName.Length/sizeof(WCHAR);
			while( --index && ldr.sgdi.DriverName.Buffer[index] != '\\' );
			if( index <= 0 ) {
				RtlMoveMemory(&ldr.sgdi.DriverName.Buffer[DRVPATH_PREFIX_SIZE/sizeof(WCHAR)], ldr.sgdi.DriverName.Buffer, ldr.sgdi.DriverName.Length);
				RtlMoveMemory(ldr.sgdi.DriverName.Buffer, DRVPATH_PREFIX, DRVPATH_PREFIX_SIZE);
				ldr.sgdi.DriverName.Length += DRVPATH_PREFIX_SIZE;
			} else {
				index++;
				RtlMoveMemory(&ldr.sgdi.DriverName.Buffer[DRVPATH_PREFIX_SIZE/sizeof(WCHAR)], &ldr.sgdi.DriverName.Buffer[index], ldr.sgdi.DriverName.Length-(index*sizeof(WCHAR)));
				RtlMoveMemory(ldr.sgdi.DriverName.Buffer, DRVPATH_PREFIX, DRVPATH_PREFIX_SIZE);
				ldr.sgdi.DriverName.Length = (USHORT)(DRVPATH_PREFIX_SIZE+(ldr.sgdi.DriverName.Length-(index*sizeof(WCHAR))));
			}
		}

		status = IsFilePresent(&ldr.sgdi.DriverName);
		if( !NT_SUCCESS(status) ) {
			LOG_ERROR("check driver path %wZ not found, status 0x%08X", &ldr.sgdi.DriverName, status);
			break;
		}

	#pragma warning(suppress: 4127)
	} while(0);
	ZwClose(keyHandle);
	if( !NT_SUCCESS(status) ) {
		if( ldr.keyInfo )
			ExFreePoolWithTag(ldr.keyInfo, (ULONG)'DAOL');
		ldr.keyInfo = 0;
	}
	return status;
}

//------------------------------------------------------------------------------
// DriverEntry
//------------------------------------------------------------------------------
__drv_functionClass(DRIVER_INITIALIZE)
__drv_sameIRQL
NTSTATUS NTAPI DriverEntry(
	__in struct _DRIVER_OBJECT *driverObject,
	__in PUNICODE_STRING registryPath )
{

	NTSTATUS status = STATUS_UNSUCCESSFUL;

	LOG_INFO(": ************************************ ");
	LOG_INFO(": resmon driver wrapper load at 0x%p", driverObject->DriverStart);
	LOG_INFO(": Built on "__DATE__" at "__TIME__"");
	LOG_INFO(": ************************************ ");

	if( !KPTR(registryPath) || 
		!KPTR(registryPath->Buffer) || 
		registryPath->Length < sizeof(WCHAR) ) {
		LOG_ERROR("incorrect registry path");
		return STATUS_UNSUCCESSFUL;
	}

	status = CreateGdiInfo(registryPath);
	if( !NT_SUCCESS(status) )
		return status;

	do {
		status = ZwSetSystemInformation(SYSTEM_LOAD_GDI_DRIVER, &ldr.sgdi, sizeof(ldr.sgdi));
		if( !NT_SUCCESS(status) ) {
			LOG_ERROR("ZwSetSystemInformation(SYSTEM_LOAD_GDI_DRIVER, %wZ) failure status 0x%08X\n", \
				&ldr.sgdi.DriverName, status);
			break;
		}

		LOG_INFO("<--- load %wZ at 0x%p (size 0x%08X), DriverEntry at 0x%p, Section at %p\n", \
			&ldr.sgdi.DriverName, ldr.sgdi.ImageAddress, ldr.sgdi.ImageLength, ldr.sgdi.driverEntry, ldr.sgdi.SectionPointer);

		InstallResMonitoringSystem(ldr.sgdi.ImageAddress, MON_DEFAULT_LIMITS);

		status = ldr.sgdi.driverEntry(driverObject, registryPath);
		if( !NT_SUCCESS(status) ) {
			NTSTATUS unloadStatus = 0;
			LOG_ERROR("DriverEntry(%wZ) failure status 0x%08X\n", &ldr.sgdi.DriverName, status);

			UninstallResMonitoringSystem();
			unloadStatus = ZwSetSystemInformation(SYSTEM_UNLOAD_GDI_DRIVER, &ldr.sgdi.SectionPointer, sizeof(void *));
			if( !NT_SUCCESS(unloadStatus) ) {
				LOG_ERROR("ZwSetSystemInformation(SYSTEM_UNLOAD_GDI_DRIVER, %wZ) failure status 0x%08X\n", &ldr.sgdi.DriverName, unloadStatus);
			}
			break;
		}

	#pragma warning(suppress: 4127)
	} while(0);

	if( !NT_SUCCESS(status) ) {
		if( ldr.keyInfo )
			ExFreePoolWithTag(ldr.keyInfo, (ULONG)'DAOL');
		return status;
	}

	if( driverObject->DriverUnload ) {
		ldr.driverUnload = driverObject->DriverUnload;
		driverObject->DriverUnload = DriverUnload;
	}
	return status;
}

//------------------------------------------------------------------------------
// DriverUnload
//------------------------------------------------------------------------------
__drv_functionClass(DRIVER_UNLOAD)
__drv_requiresIRQL(PASSIVE_LEVEL)
__drv_sameIRQL
VOID NTAPI DriverUnload(
	__in struct _DRIVER_OBJECT *driverObject )
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	driverObject->DriverUnload = ldr.driverUnload;
	ldr.driverUnload(driverObject);
	UninstallResMonitoringSystem();
	driverObject->DriverUnload = DriverUnload;

	status = ZwSetSystemInformation(SYSTEM_UNLOAD_GDI_DRIVER, &ldr.sgdi.SectionPointer, sizeof(void *));
	if( !NT_SUCCESS(status) ) {
		LOG_ERROR("ZwSetSystemInformation(SYSTEM_UNLOAD_GDI_DRIVER, %wZ) failure status 0x%08X\n", &ldr.sgdi.DriverName, status);
	}

	LOG_INFO("unload %wZ at 0x%p (size 0x%08X), DriverEntry at 0x%p, Section at %p\n", \
			&ldr.sgdi.DriverName, ldr.sgdi.ImageAddress, ldr.sgdi.ImageLength, ldr.sgdi.driverEntry, ldr.sgdi.SectionPointer);

	if( ldr.keyInfo )
		ExFreePoolWithTag(ldr.keyInfo, (ULONG)'DAOL');

	LOG_INFO(": ************************************ ");
	LOG_INFO(": resmon unload");
	LOG_INFO(": ************************************ ");
	return;
}
