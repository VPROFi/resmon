#ifndef __OSDEP_H__
#define __OSDEP_H__

#include <modedep/modedep.h>

typedef enum _SYSTEM_INFORMATION_CLASS {
  SystemBasicInformation = 0x0,
  SystemProcessorInformation = 0x1,
  SystemPerformanceInformation = 0x2,
  SystemTimeOfDayInformation = 0x3,
  SystemPathInformation = 0x4,
  SystemProcessInformation = 0x5,
  SystemCallCountInformation = 0x6,
  SystemDeviceInformation = 0x7,
  SystemProcessorPerformanceInformation = 0x8,
  SystemFlagsInformation = 0x9,
  SystemCallTimeInformation = 0xa,
  SystemModuleInformation = 0xb,
  SystemLocksInformation = 0xc,
  SystemStackTraceInformation = 0xd,
  SystemPagedPoolInformation = 0xe,
  SystemNonPagedPoolInformation = 0xf,
  SystemHandleInformation = 0x10,
  SystemObjectInformation = 0x11,
  SystemPageFileInformation = 0x12,
  SystemVdmInstemulInformation = 0x13,
  SystemVdmBopInformation = 0x14,
  SystemFileCacheInformation = 0x15,
  SystemPoolTagInformation = 0x16,
  SystemInterruptInformation = 0x17,
  SystemDpcBehaviorInformation = 0x18,
  SystemFullMemoryInformation = 0x19,
  SystemLoadGdiDriverInformation = 0x1a,
  SystemUnloadGdiDriverInformation = 0x1b,
  SystemTimeAdjustmentInformation = 0x1c,
  SystemSummaryMemoryInformation = 0x1d,
  SystemMirrorMemoryInformation = 0x1e,
  SystemPerformanceTraceInformation = 0x1f,
  SystemObsolete0 = 0x20,
  SystemExceptionInformation = 0x21,
  SystemCrashDumpStateInformation = 0x22,
  SystemKernelDebuggerInformation = 0x23,
  SystemContextSwitchInformation = 0x24,
  SystemRegistryQuotaInformation = 0x25,
  SystemExtendServiceTableInformation = 0x26,
  SystemPrioritySeperation = 0x27,
  SystemVerifierAddDriverInformation = 0x28,
  SystemVerifierRemoveDriverInformation = 0x29,
  SystemProcessorIdleInformation = 0x2a,
  SystemLegacyDriverInformation = 0x2b,
  SystemCurrentTimeZoneInformation = 0x2c,
  SystemLookasideInformation = 0x2d,
  SystemTimeSlipNotification = 0x2e,
  SystemSessionCreate = 0x2f,
  SystemSessionDetach = 0x30,
  SystemSessionInformation = 0x31,
  SystemRangeStartInformation = 0x32,
  SystemVerifierInformation = 0x33,
  SystemVerifierThunkExtend = 0x34,
  SystemSessionProcessInformation = 0x35,
  SystemLoadGdiDriverInSystemSpace = 0x36,
  SystemNumaProcessorMap = 0x37,
  SystemPrefetcherInformation = 0x38,
  SystemExtendedProcessInformation = 0x39,
  SystemRecommendedSharedDataAlignment = 0x3a,
  SystemComPlusPackage = 0x3b,
  SystemNumaAvailableMemory = 0x3c,
  SystemProcessorPowerInformation = 0x3d,
  SystemEmulationBasicInformation = 0x3e,
  SystemEmulationProcessorInformation = 0x3f,
  SystemExtendedHandleInformation = 0x40,
  SystemLostDelayedWriteInformation = 0x41,
  SystemBigPoolInformation = 0x42,
  SystemSessionPoolTagInformation = 0x43,
  SystemSessionMappedViewInformation = 0x44,
  SystemHotpatchInformation = 0x45,
  SystemObjectSecurityMode = 0x46,
  SystemWatchdogTimerHandler = 0x47,
  SystemWatchdogTimerInformation = 0x48,
  SystemLogicalProcessorInformation = 0x49,
  SystemWow64SharedInformationObsolete = 0x4a,
  SystemRegisterFirmwareTableInformationHandler = 0x4b,
  SystemFirmwareTableInformation = 0x4c,
  SystemModuleInformationEx = 0x4d,
  SystemVerifierTriageInformation = 0x4e,
  SystemSuperfetchInformation = 0x4f,
  SystemMemoryListInformation = 0x50,
  SystemFileCacheInformationEx = 0x51,
  SystemThreadPriorityClientIdInformation = 0x52,
  SystemProcessorIdleCycleTimeInformation = 0x53,
  SystemVerifierCancellationInformation = 0x54,
  SystemProcessorPowerInformationEx = 0x55,
  SystemRefTraceInformation = 0x56,
  SystemSpecialPoolInformation = 0x57,
  SystemProcessIdInformation = 0x58,
  SystemErrorPortInformation = 0x59,
  SystemBootEnvironmentInformation = 0x5a,
  SystemHypervisorInformation = 0x5b,
  SystemVerifierInformationEx = 0x5c,
  SystemTimeZoneInformation = 0x5d,
  SystemImageFileExecutionOptionsInformation = 0x5e,
  SystemCoverageInformation = 0x5f,
  SystemPrefetchPatchInformation = 0x60,
  SystemVerifierFaultsInformation = 0x61,
  SystemSystemPartitionInformation = 0x62,
  SystemSystemDiskInformation = 0x63,
  SystemProcessorPerformanceDistribution = 0x64,
  SystemNumaProximityNodeInformation = 0x65,
  SystemDynamicTimeZoneInformation = 0x66,
  SystemCodeIntegrityInformation = 0x67,
  SystemProcessorMicrocodeUpdateInformation = 0x68,
  SystemProcessorBrandString = 0x69,
  SystemVirtualAddressInformation = 0x6a,
  SystemLogicalProcessorAndGroupInformation = 0x6b,
  SystemProcessorCycleTimeInformation = 0x6c,
  SystemStoreInformation = 0x6d,
  SystemRegistryAppendString = 0x6e,
  SystemAitSamplingValue = 0x6f,
  SystemVhdBootInformation = 0x70,
  SystemCpuQuotaInformation = 0x71,
  SystemNativeBasicInformation = 0x72,
  SystemErrorPortTimeouts = 0x73,
  SystemLowPriorityIoInformation = 0x74,
  SystemBootEntropyInformation = 0x75,
  SystemVerifierCountersInformation = 0x76,
  SystemPagedPoolInformationEx = 0x77,
  SystemSystemPtesInformationEx = 0x78,
  SystemNodeDistanceInformation = 0x79,
  SystemAcpiAuditInformation = 0x7a,
  SystemBasicPerformanceInformation = 0x7b,
  SystemQueryPerformanceCounterInformation = 0x7c,
  SystemSessionBigPoolInformation = 0x7d,
  SystemBootGraphicsInformation = 0x7e,
  SystemScrubPhysicalMemoryInformation = 0x7f,
  SystemBadPageInformation = 0x80,
  SystemProcessorProfileControlArea = 0x81,
  SystemCombinePhysicalMemoryInformation = 0x82,
  SystemEntropyInterruptTimingInformation = 0x83,
  SystemConsoleInformation = 0x84,
  SystemPlatformBinaryInformation = 0x85,
  SystemPolicyInformation = 0x86,
  SystemHypervisorProcessorCountInformation = 0x87,
  SystemDeviceDataInformation = 0x88,
  SystemDeviceDataEnumerationInformation = 0x89,
  SystemMemoryTopologyInformation = 0x8a,
  SystemMemoryChannelInformation = 0x8b,
  SystemBootLogoInformation = 0x8c,
  SystemProcessorPerformanceInformationEx = 0x8d,
  SystemCriticalProcessErrorLogInformation = 0x8e,
  SystemSecureBootPolicyInformation = 0x8f,
  SystemPageFileInformationEx = 0x90,
  SystemSecureBootInformation = 0x91,
  SystemEntropyInterruptTimingRawInformation = 0x92,
  SystemPortableWorkspaceEfiLauncherInformation = 0x93,
  SystemFullProcessInformation = 0x94,
  SystemKernelDebuggerInformationEx = 0x95,
  SystemBootMetadataInformation = 0x96,
  SystemSoftRebootInformation = 0x97,
  SystemElamCertificateInformation = 0x98,
  SystemOfflineDumpConfigInformation = 0x99,
  SystemProcessorFeaturesInformation = 0x9a,
  SystemRegistryReconciliationInformation = 0x9b,
  SystemEdidInformation = 0x9c,
  SystemManufacturingInformation = 0x9d,
  SystemEnergyEstimationConfigInformation = 0x9e,
  SystemHypervisorDetailInformation = 0x9f,
  SystemProcessorCycleStatsInformation = 0xa0,
  SystemVmGenerationCountInformation = 0xa1,
  SystemTrustedPlatformModuleInformation = 0xa2,
  SystemKernelDebuggerFlags = 0xa3,
  SystemCodeIntegrityPolicyInformation = 0xa4,
  SystemIsolatedUserModeInformation = 0xa5,
  SystemHardwareSecurityTestInterfaceResultsInformation = 0xa6,
  SystemSingleModuleInformation = 0xa7,
  SystemAllowedCpuSetsInformation = 0xa8,
  SystemVsmProtectionInformation = 0xa9,
  SystemInterruptCpuSetsInformation = 0xaa,
  SystemSecureBootPolicyFullInformation = 0xab,
  SystemCodeIntegrityPolicyFullInformation = 0xac,
  SystemAffinitizedInterruptProcessorInformation = 0xad,
  SystemRootSiloInformation = 0xae,
  SystemCpuSetInformation = 0xaf,
  SystemCpuSetTagInformation = 0xb0,
  SystemWin32WerStartCallout = 0xb1,
  SystemSecureKernelProfileInformation = 0xb2,
  SystemCodeIntegrityPlatformManifestInformation = 0xb3,
  SystemInterruptSteeringInformation = 0xb4,
  SystemSupportedProcessorArchitectures = 0xb5,
  SystemMemoryUsageInformation = 0xb6,
  SystemCodeIntegrityCertificateInformation = 0xb7,
  SystemPhysicalMemoryInformation = 0xb8,
  SystemControlFlowTransition = 0xb9,
  SystemKernelDebuggingAllowed = 0xba,
  SystemActivityModerationExeState = 0xbb,
  SystemActivityModerationUserSettings = 0xbc,
  SystemCodeIntegrityPoliciesFullInformation = 0xbd,
  SystemCodeIntegrityUnlockInformation = 0xbe,
  SystemIntegrityQuotaInformation = 0xbf,
  SystemFlushInformation = 0xc0,
  SystemProcessorIdleMaskInformation = 0xc1,
  SystemSecureDumpEncryptionInformation = 0xc2,
  SystemWriteConstraintInformation = 0xc3,
  SystemKernelVaShadowInformation = 0xc4,
  SystemHypervisorSharedPageInformation = 0xc5,
  SystemFirmwareBootPerformanceInformation = 0xc6,
  SystemCodeIntegrityVerificationInformation = 0xc7,
  SystemFirmwarePartitionInformation = 0xc8,
  SystemSpeculationControlInformation = 0xc9,
  SystemDmaGuardPolicyInformation = 0xca,
  SystemEnclaveLaunchControlInformation = 0xcb,
  MaxSystemInfoClass = 0xcc,
} SYSTEM_INFORMATION_CLASS;

#ifndef _KERNEL_MODE
typedef struct _CLIENT_ID
{
    HANDLE UniqueProcess;
    HANDLE UniqueThread;
} CLIENT_ID, *PCLIENT_ID;
typedef LONG KPRIORITY;
typedef struct _UNICODE_STRING
{
    USHORT Length;
    USHORT MaximumLength;
    _Field_size_bytes_part_(MaximumLength, Length) PWCH Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef const UNICODE_STRING *PCUNICODE_STRING;

typedef enum _KWAIT_REASON {
  Executive = 0x0,
  FreePage = 0x1,
  PageIn = 0x2,
  PoolAllocation = 0x3,
  DelayExecution = 0x4,
  Suspended = 0x5,
  UserRequest = 0x6,
  WrExecutive = 0x7,
  WrFreePage = 0x8,
  WrPageIn = 0x9,
  WrPoolAllocation = 0xa,
  WrDelayExecution = 0xb,
  WrSuspended = 0xc,
  WrUserRequest = 0xd,
  WrSpare0 = 0xe,
  WrQueue = 0xf,
  WrLpcReceive = 0x10,
  WrLpcReply = 0x11,
  WrVirtualMemory = 0x12,
  WrPageOut = 0x13,
  WrRendezvous = 0x14,
  WrKeyedEvent = 0x15,
  WrTerminated = 0x16,
  WrProcessInSwap = 0x17,
  WrCpuRateControl = 0x18,
  WrCalloutStack = 0x19,
  WrKernel = 0x1a,
  WrResource = 0x1b,
  WrPushLock = 0x1c,
  WrMutex = 0x1d,
  WrQuantumEnd = 0x1e,
  WrDispatchInt = 0x1f,
  WrPreempted = 0x20,
  WrYieldExecution = 0x21,
  WrFastMutex = 0x22,
  WrGuardedMutex = 0x23,
  WrRundown = 0x24,
  WrAlertByThreadId = 0x25,
  WrDeferredPreempt = 0x26,
  WrPhysicalFault = 0x27,
  MaximumWaitReason = 0x28,
} KWAIT_REASON;

typedef enum _PROCESSINFOCLASS {
  ProcessBasicInformation = 0x0,
  ProcessQuotaLimits = 0x1,
  ProcessIoCounters = 0x2,
  ProcessVmCounters = 0x3,
  ProcessTimes = 0x4,
  ProcessBasePriority = 0x5,
  ProcessRaisePriority = 0x6,
  ProcessDebugPort = 0x7,
  ProcessExceptionPort = 0x8,
  ProcessAccessToken = 0x9,
  ProcessLdtInformation = 0xa,
  ProcessLdtSize = 0xb,
  ProcessDefaultHardErrorMode = 0xc,
  ProcessIoPortHandlers = 0xd,
  ProcessPooledUsageAndLimits = 0xe,
  ProcessWorkingSetWatch = 0xf,
  ProcessUserModeIOPL = 0x10,
  ProcessEnableAlignmentFaultFixup = 0x11,
  ProcessPriorityClass = 0x12,
  ProcessWx86Information = 0x13,
  ProcessHandleCount = 0x14,
  ProcessAffinityMask = 0x15,
  ProcessPriorityBoost = 0x16,
  ProcessDeviceMap = 0x17,
  ProcessSessionInformation = 0x18,
  ProcessForegroundInformation = 0x19,
  ProcessWow64Information = 0x1a,
  ProcessImageFileName = 0x1b,
  ProcessLUIDDeviceMapsEnabled = 0x1c,
  ProcessBreakOnTermination = 0x1d,
  ProcessDebugObjectHandle = 0x1e,
  ProcessDebugFlags = 0x1f,
  ProcessHandleTracing = 0x20,
  ProcessIoPriority = 0x21,
  ProcessExecuteFlags = 0x22,
  ProcessTlsInformation = 0x23,
  ProcessCookie = 0x24,
  ProcessImageInformation = 0x25,
  ProcessCycleTime = 0x26,
  ProcessPagePriority = 0x27,
  ProcessInstrumentationCallback = 0x28,
  ProcessThreadStackAllocation = 0x29,
  ProcessWorkingSetWatchEx = 0x2a,
  ProcessImageFileNameWin32 = 0x2b,
  ProcessImageFileMapping = 0x2c,
  ProcessAffinityUpdateMode = 0x2d,
  ProcessMemoryAllocationMode = 0x2e,
  ProcessGroupInformation = 0x2f,
  ProcessTokenVirtualizationEnabled = 0x30,
  ProcessOwnerInformation = 0x31,
  ProcessWindowInformation = 0x32,
  ProcessHandleInformation = 0x33,
  ProcessMitigationPolicy = 0x34,
  ProcessDynamicFunctionTableInformation = 0x35,
  ProcessHandleCheckingMode = 0x36,
  ProcessKeepAliveCount = 0x37,
  ProcessRevokeFileHandles = 0x38,
  ProcessWorkingSetControl = 0x39,
  ProcessHandleTable = 0x3a,
  ProcessCheckStackExtentsMode = 0x3b,
  ProcessCommandLineInformation = 0x3c,
  ProcessProtectionInformation = 0x3d,
  ProcessMemoryExhaustion = 0x3e,
  ProcessFaultInformation = 0x3f,
  ProcessTelemetryIdInformation = 0x40,
  ProcessCommitReleaseInformation = 0x41,
  ProcessDefaultCpuSetsInformation = 0x42,
  ProcessAllowedCpuSetsInformation = 0x43,
  ProcessReserved1Information = 0x42,
  ProcessReserved2Information = 0x43,
  ProcessSubsystemProcess = 0x44,
  ProcessJobMemoryInformation = 0x45,
  ProcessInPrivate = 0x46,
  ProcessRaiseUMExceptionOnInvalidHandleClose = 0x47,
  ProcessIumChallengeResponse = 0x48,
  ProcessChildProcessInformation = 0x49,
  ProcessHighGraphicsPriorityInformation = 0x4a,
  ProcessSubsystemInformation = 0x4b,
  ProcessEnergyValues = 0x4c,
  ProcessPowerThrottlingState = 0x4d,
  ProcessReserved3Information = 0x4e,
  ProcessWin32kSyscallFilterInformation = 0x4f,
  ProcessDisableSystemAllowedCpuSets = 0x50,
  ProcessWakeInformation = 0x51,
  ProcessEnergyTrackingState = 0x52,
  ProcessManageWritesToExecutableMemory = 0x53,
  ProcessCaptureTrustletLiveDump = 0x54,
  ProcessTelemetryCoverage = 0x55,
  ProcessEnclaveInformation = 0x56,
  ProcessEnableReadWriteVmLogging = 0x57,
  ProcessUptimeInformation = 0x58,
  ProcessImageSection = 0x59,
  ProcessDebugAuthInformation = 0x5a,
  ProcessSystemResourceManagement = 0x5b,
  ProcessSequenceNumber = 0x5c,
  MaxProcessInfoClass = 0x5d,
} PROCESSINFOCLASS;

typedef enum _THREADINFOCLASS {
  ThreadBasicInformation = 0x0,
  ThreadTimes = 0x1,
  ThreadPriority = 0x2,
  ThreadBasePriority = 0x3,
  ThreadAffinityMask = 0x4,
  ThreadImpersonationToken = 0x5,
  ThreadDescriptorTableEntry = 0x6,
  ThreadEnableAlignmentFaultFixup = 0x7,
  ThreadEventPair_Reusable = 0x8,
  ThreadQuerySetWin32StartAddress = 0x9,
  ThreadZeroTlsCell = 0xa,
  ThreadPerformanceCount = 0xb,
  ThreadAmILastThread = 0xc,
  ThreadIdealProcessor = 0xd,
  ThreadPriorityBoost = 0xe,
  ThreadSetTlsArrayAddress = 0xf,
  ThreadIsIoPending = 0x10,
  ThreadHideFromDebugger = 0x11,
  ThreadBreakOnTermination = 0x12,
  ThreadSwitchLegacyState = 0x13,
  ThreadIsTerminated = 0x14,
  ThreadLastSystemCall = 0x15,
  ThreadIoPriority = 0x16,
  ThreadCycleTime = 0x17,
  ThreadPagePriority = 0x18,
  ThreadActualBasePriority = 0x19,
  ThreadTebInformation = 0x1a,
  ThreadCSwitchMon = 0x1b,
  ThreadCSwitchPmu = 0x1c,
  ThreadWow64Context = 0x1d,
  ThreadGroupInformation = 0x1e,
  ThreadUmsInformation = 0x1f,
  ThreadCounterProfiling = 0x20,
  ThreadIdealProcessorEx = 0x21,
  ThreadCpuAccountingInformation = 0x22,
  ThreadSuspendCount = 0x23,
  ThreadHeterogeneousCpuPolicy = 0x24,
  ThreadContainerId = 0x25,
  ThreadNameInformation = 0x26,
  ThreadSelectedCpuSets = 0x27,
  ThreadSystemThreadInformation = 0x28,
  ThreadActualGroupAffinity = 0x29,
  ThreadDynamicCodePolicyInfo = 0x2a,
  ThreadExplicitCaseSensitivity = 0x2b,
  ThreadWorkOnBehalfTicket = 0x2c,
  ThreadSubsystemInformation = 0x2d,
  ThreadDbgkWerReportActive = 0x2e,
  ThreadAttachContainer = 0x2f,
  ThreadManageWritesToExecutableMemory = 0x30,
  ThreadPowerThrottlingState = 0x31,
  MaxThreadInfoClass = 0x32,
} THREADINFOCLASS;

typedef struct _KERNEL_USER_TIMES
{
    LARGE_INTEGER CreateTime;
    LARGE_INTEGER ExitTime;
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
} KERNEL_USER_TIMES, *PKERNEL_USER_TIMES;

#endif // _KERNEL_MODE

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


typedef struct _SYSTEM_PERFORMANCE_INFORMATION
{
    LARGE_INTEGER IdleProcessTime;
    LARGE_INTEGER IoReadTransferCount;
    LARGE_INTEGER IoWriteTransferCount;
    LARGE_INTEGER IoOtherTransferCount;
    ULONG IoReadOperationCount;
    ULONG IoWriteOperationCount;
    ULONG IoOtherOperationCount;
    ULONG AvailablePages;
    ULONG CommittedPages;
    ULONG CommitLimit;
    ULONG PeakCommitment;
    ULONG PageFaultCount;
    ULONG CopyOnWriteCount;
    ULONG TransitionCount;
    ULONG CacheTransitionCount;
    ULONG DemandZeroCount;
    ULONG PageReadCount;
    ULONG PageReadIoCount;
    ULONG CacheReadCount;
    ULONG CacheIoCount;
    ULONG DirtyPagesWriteCount;
    ULONG DirtyWriteIoCount;
    ULONG MappedPagesWriteCount;
    ULONG MappedWriteIoCount;
    ULONG PagedPoolPages;
    ULONG NonPagedPoolPages;
    ULONG PagedPoolAllocs;
    ULONG PagedPoolFrees;
    ULONG NonPagedPoolAllocs;
    ULONG NonPagedPoolFrees;
    ULONG FreeSystemPtes;
    ULONG ResidentSystemCodePage;
    ULONG TotalSystemDriverPages;
    ULONG TotalSystemCodePages;
    ULONG NonPagedPoolLookasideHits;
    ULONG PagedPoolLookasideHits;
    ULONG AvailablePagedPoolPages;
    ULONG ResidentSystemCachePage;
    ULONG ResidentPagedPoolPage;
    ULONG ResidentSystemDriverPage;
    ULONG CcFastReadNoWait;
    ULONG CcFastReadWait;
    ULONG CcFastReadResourceMiss;
    ULONG CcFastReadNotPossible;
    ULONG CcFastMdlReadNoWait;
    ULONG CcFastMdlReadWait;
    ULONG CcFastMdlReadResourceMiss;
    ULONG CcFastMdlReadNotPossible;
    ULONG CcMapDataNoWait;
    ULONG CcMapDataWait;
    ULONG CcMapDataNoWaitMiss;
    ULONG CcMapDataWaitMiss;
    ULONG CcPinMappedDataCount;
    ULONG CcPinReadNoWait;
    ULONG CcPinReadWait;
    ULONG CcPinReadNoWaitMiss;
    ULONG CcPinReadWaitMiss;
    ULONG CcCopyReadNoWait;
    ULONG CcCopyReadWait;
    ULONG CcCopyReadNoWaitMiss;
    ULONG CcCopyReadWaitMiss;
    ULONG CcMdlReadNoWait;
    ULONG CcMdlReadWait;
    ULONG CcMdlReadNoWaitMiss;
    ULONG CcMdlReadWaitMiss;
    ULONG CcReadAheadIos;
    ULONG CcLazyWriteIos;
    ULONG CcLazyWritePages;
    ULONG CcDataFlushes;
    ULONG CcDataPages;
    ULONG ContextSwitches;
    ULONG FirstLevelTbFills;
    ULONG SecondLevelTbFills;
    ULONG SystemCalls;
    ULONGLONG CcTotalDirtyPages;
    ULONGLONG CcDirtyPageThreshold;
    LONGLONG ResidentAvailablePages;
    ULONGLONG SharedCommittedPages;
} SYSTEM_PERFORMANCE_INFORMATION, *PSYSTEM_PERFORMANCE_INFORMATION;

typedef struct _SYSTEM_THREAD_INFORMATION
{
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER CreateTime;
    ULONG WaitTime;
    PVOID StartAddress;
    CLIENT_ID ClientId;
    KPRIORITY Priority;
    LONG BasePriority;
    ULONG ContextSwitches;
    ULONG ThreadState;
    KWAIT_REASON WaitReason;
} SYSTEM_THREAD_INFORMATION, *PSYSTEM_THREAD_INFORMATION;

typedef struct _TEB *PTEB;

// private
typedef struct _SYSTEM_EXTENDED_THREAD_INFORMATION
{
    SYSTEM_THREAD_INFORMATION ThreadInfo;
    PVOID StackBase;
    PVOID StackLimit;
    PVOID Win32StartAddress;
    PTEB TebBase; // since VISTA
    ULONG_PTR Reserved2;
    ULONG_PTR Reserved3;
    ULONG_PTR Reserved4;
} SYSTEM_EXTENDED_THREAD_INFORMATION, *PSYSTEM_EXTENDED_THREAD_INFORMATION;

typedef struct _SYSTEM_PROCESS_INFORMATION
{
    ULONG NextEntryOffset;
    ULONG NumberOfThreads;
    LARGE_INTEGER WorkingSetPrivateSize; // since VISTA
    ULONG HardFaultCount; // since WIN7
    ULONG NumberOfThreadsHighWatermark; // since WIN7
    ULONGLONG CycleTime; // since WIN7
    LARGE_INTEGER CreateTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER KernelTime;
    UNICODE_STRING ImageName;
    KPRIORITY BasePriority;
    HANDLE UniqueProcessId;
    HANDLE InheritedFromUniqueProcessId;
    ULONG HandleCount;
    ULONG SessionId;
    ULONG_PTR UniqueProcessKey;
    SIZE_T PeakVirtualSize;
    SIZE_T VirtualSize;
    ULONG PageFaultCount;
    SIZE_T PeakWorkingSetSize;
    SIZE_T WorkingSetSize;
    SIZE_T QuotaPeakPagedPoolUsage;
    SIZE_T QuotaPagedPoolUsage;
    SIZE_T QuotaPeakNonPagedPoolUsage;
    SIZE_T QuotaNonPagedPoolUsage;
    SIZE_T PagefileUsage;
    SIZE_T PeakPagefileUsage;
    SIZE_T PrivatePageCount;
    LARGE_INTEGER ReadOperationCount;
    LARGE_INTEGER WriteOperationCount;
    LARGE_INTEGER OtherOperationCount;
    LARGE_INTEGER ReadTransferCount;
    LARGE_INTEGER WriteTransferCount;
    LARGE_INTEGER OtherTransferCount;
    SYSTEM_THREAD_INFORMATION Threads[1];
} SYSTEM_PROCESS_INFORMATION, *PSYSTEM_PROCESS_INFORMATION;

typedef struct _SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION
{
    LARGE_INTEGER IdleTime;
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER DpcTime;
    LARGE_INTEGER InterruptTime;
    ULONG InterruptCount;
} SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION, *PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION;


typedef struct _SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION_EX
{
    LARGE_INTEGER IdleTime;
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER DpcTime;
    LARGE_INTEGER InterruptTime;
    ULONG InterruptCount;
    ULONG Spare0;
    LARGE_INTEGER AvailableTime;
    LARGE_INTEGER Spare1;
    LARGE_INTEGER Spare2;
} SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION_EX, *PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION_EX;


EXTERN_C NTSTATUS __stdcall NtQuerySystemInformation(
    _In_ SYSTEM_INFORMATION_CLASS SystemInformationClass,
    _Out_writes_bytes_opt_(SystemInformationLength) PVOID SystemInformation,
    _In_ ULONG SystemInformationLength,
    _Out_opt_ PULONG ReturnLength
    );

EXTERN_C NTSTATUS __stdcall ZwQuerySystemInformation(
    _In_ SYSTEM_INFORMATION_CLASS SystemInformationClass,
    _Out_writes_bytes_opt_(SystemInformationLength) PVOID SystemInformation,
    _In_ ULONG SystemInformationLength,
    _Out_opt_ PULONG ReturnLength
    );

EXTERN_C NTSTATUS __stdcall NtQueryInformationThread(
    _In_ HANDLE ThreadHandle,
    _In_ THREADINFOCLASS ThreadInformationClass,
    _Out_writes_bytes_(ThreadInformationLength) PVOID ThreadInformation,
    _In_ ULONG ThreadInformationLength,
    _Out_opt_ PULONG ReturnLength
    );

EXTERN_C NTSTATUS __stdcall ZwQueryInformationThread(
    _In_ HANDLE ThreadHandle,
    _In_ THREADINFOCLASS ThreadInformationClass,
    _Out_writes_bytes_(ThreadInformationLength) PVOID ThreadInformation,
    _In_ ULONG ThreadInformationLength,
    _Out_opt_ PULONG ReturnLength
    );

EXTERN_C NTSTATUS __stdcall NtQueryInformationProcess(
    _In_ HANDLE ProcessHandle,
    _In_ PROCESSINFOCLASS ProcessInformationClass,
    _Out_writes_bytes_(ProcessInformationLength) PVOID ProcessInformation,
    _In_ ULONG ProcessInformationLength,
    _Out_opt_ PULONG ReturnLength
    );

EXTERN_C NTSTATUS __stdcall ZwQueryInformationProcess(
    _In_ HANDLE ProcessHandle,
    _In_ PROCESSINFOCLASS ProcessInformationClass,
    _Out_writes_bytes_(ProcessInformationLength) PVOID ProcessInformation,
    _In_ ULONG ProcessInformationLength,
    _Out_opt_ PULONG ReturnLength
    );

#endif // __OSDEP_H__
