#include <shared/config.h>
#include <common/dbglog.h>
#include <resmon/resmonlib.h>
//******************************************************************************
// Functions placement
//******************************************************************************
EXTERN_C_START
DRIVER_INITIALIZE DriverEntry;
NTSTATUS
DriverEntry(__in struct _DRIVER_OBJECT * drvObject,
			__in PUNICODE_STRING regPath);
EXTERN_C_END

DRIVER_UNLOAD resmonDriverUnload;
static void resmonDriverUnload(__in struct _DRIVER_OBJECT * drvObject);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, resmonDriverUnload)
#endif

void * ptr = 0;
HANDLE eventHandle = 0;
HANDLE eventHandleNt = 0;
HANDLE eventHandleZw = 0;
HANDLE fileHandle = 0;
//------------------------------------------------------------------------------
// resmonDriverUnload
//------------------------------------------------------------------------------
__drv_functionClass(DRIVER_UNLOAD)
__drv_requiresIRQL(PASSIVE_LEVEL)
__drv_sameIRQL
static void resmonDriverUnload(__in struct _DRIVER_OBJECT * drvObject)
{
    PAGED_CODE();

	MonLogAllResources();


	ZwClose(eventHandleNt);

	ZwClose(eventHandleZw);

	ZwClose(eventHandle);


	LOG_INFO(": ************************************ ");
	LOG_INFO(": resmon unload");
	LOG_INFO(": ************************************ ");

	ExFreePoolWithTag(ptr, 'TSET');

	UninstallResMonitoringSystem();
	return;
}

NTSYSAPI 
NTSTATUS
NTAPI
NtCreateEvent(
  OUT PHANDLE             EventHandle,
  IN ACCESS_MASK          DesiredAccess,
  IN POBJECT_ATTRIBUTES   ObjectAttributes OPTIONAL,
  IN EVENT_TYPE           EventType,
  IN BOOLEAN              InitialState );

//------------------------------------------------------------------------------
// DriverEntry
//------------------------------------------------------------------------------
__drv_functionClass(DRIVER_INITIALIZE)
__drv_sameIRQL
__drv_requiresIRQL(PASSIVE_LEVEL)
NTSTATUS
DriverEntry(
    __in struct _DRIVER_OBJECT * drvObject,
    __in PUNICODE_STRING regPath)
{
	NTSTATUS status = STATUS_SUCCESS, status1 = 0;
	UNICODE_STRING eventName = {0};
	UNICODE_STRING fileName = {0};
	OBJECT_ATTRIBUTES oa = {0};
	IO_STATUS_BLOCK  isb = {0};
	PKEVENT event = 0;
	static UniStrConst name = {L"ntoskrnl.exe", sizeof(L"ntoskrnl.exe")-sizeof(uni_char)};
	OsModuleData mod = {0};

	ASSERT( drvObject != 0 && regPath != 0 );
	LOG_INFO(": ************************************ ");
	LOG_INFO(": resmon load at 0x%p", drvObject->DriverStart);
	LOG_INFO(": Built on "__DATE__" at "__TIME__"");
	LOG_INFO(": ************************************ ");
	drvObject->DriverUnload = resmonDriverUnload;

	if( OsModuleInfoFromName(&name, &mod) ) {
		LOG_INFO("kernel base %p", mod.base);
		OsFreeModuleData(&mod);
	}

	InstallResMonitoringSystem(0, MON_DEFAULT_LIMITS);

	ptr = ExAllocatePoolWithTag(NonPagedPool, 1024, 'TSET');

	RtlInitUnicodeString(&eventName, L"\\BaseNamedObjects\\Xxx");

	event = IoCreateNotificationEvent(
				&eventName,
				&eventHandle);

	status1 = NtCreateEvent(
		&eventHandleNt,
		EVENT_QUERY_STATE,
		NULL,
		SynchronizationEvent,
		FALSE);


	status1 = ZwCreateEvent(
		&eventHandleZw,
		EVENT_QUERY_STATE,
		NULL,
		SynchronizationEvent,
		FALSE);

	RtlInitUnicodeString(&fileName, L"\\??\\c:\\file.dat");

    InitializeObjectAttributes(
	    &oa,
    	&fileName,
        OBJ_CASE_INSENSITIVE,
	    NULL,
        NULL);

    status1 = NtCreateFile(
	    &fileHandle,
        FILE_GENERIC_READ | FILE_GENERIC_WRITE | DELETE,
	    &oa,
    	&isb,
        NULL,
	    FILE_ATTRIBUTE_NORMAL,
    	FILE_SHARE_READ,
        FILE_OVERWRITE_IF,
	    FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
    	NULL,
        0);

	return status;
}
