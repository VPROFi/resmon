#include <common/checkptr.h>
#include <common/dbglog.h>
#include <resmon/resmon.h>
#include <ihook/ihook.h>

//------------------------------------------------------------------------------
// Формат лога
//------------------------------------------------------------------------------
#define RESLOG(msg, ...) DbgPrint("%ws" LOG_PREFIX msg "\n", MonPrefix(), ## __VA_ARGS__)
//------------------------------------------------------------------------------
// Структуры хранения информации
//------------------------------------------------------------------------------
// common node
typedef struct ResNode ResNode;
struct ResNode {
	ObjectInfo	objInfo;
	unsigned char height;
	ResNode * left;
	ResNode * right;
	void * key;
	void * returnAddress;
	void * createThreadId;
	size_tr volatile refCnt;
};
//------------------------------------------------------------------------------
// objects
//------------------------------------------------------------------------------
// memory
typedef struct {
	ResNode node;
	ptr_t tag;
	size_tr size;
	unsigned long mtype;
} ResMemory;
// thread
typedef struct {
	ResNode node;
	void * startAddress;
	void * threadId;
	ResObjectCounters counters;
} ResThread;
// sync
typedef struct {
	ResNode node;
	SyncType syncType;
	union {
		struct {
			signed manualReset;
			signed initialState;
		} event;
		struct {
			signed initialOwner;
		} mutex;
		struct {
			signed long initialCount;
			signed long maximumCount;
		} semaphore;
		struct {
			unsigned long parametr1;
			unsigned long parametr2;
		} unknown;
	} ctx;
} ResSync;

//------------------------------------------------------------------------------
// objects holder
//------------------------------------------------------------------------------
typedef enum {
	ThreadResList,
	SyncResList,
	MemoryResList, // должен быть последним
	MaximumResList
} ResourceListIndex;

C_ASSERT( (MemoryResList + 1) == MaximumResList );

typedef struct {
	ResNode * avl;
	void * accessSemaphore;
	void * accessCs;
	size_tr volatile totalResources;
} RsList;

//------------------------------------------------------------------------------
// common context
//------------------------------------------------------------------------------
typedef struct {
	size_tr upperLimit;
	size_tr volatile totalAllocs;
} MonAllocs;

#define TOTAL_UPPER_LIMITS 32
#define COUNT_OF(x) (sizeof(x)/sizeof((x)[0]))
C_ASSERT( TOTAL_UPPER_LIMITS >= 4 );
#define MINIMUM_UPPER_LIMIT_START 16
#define MEDIUM_UPPER_LIMIT_START (((size_tr)MINIMUM_UPPER_LIMIT_START << (TOTAL_UPPER_LIMITS/2)) > 16384 ? \
								((size_tr)MINIMUM_UPPER_LIMIT_START << (TOTAL_UPPER_LIMITS/2)):16384)

#define HIGH_UPPER_LIMIT_START (((size_tr)MEDIUM_UPPER_LIMIT_START << (TOTAL_UPPER_LIMITS/4)) > 1048576 ? \
								((size_tr)MEDIUM_UPPER_LIMIT_START << (TOTAL_UPPER_LIMITS/4)):1048576)

typedef struct {
	signed long         threadUsed;
	signed long         syncObjectUsed;
	ssize_tr            memoryUsed;
} MonSnapShot;

typedef struct {
	ObjectInfo          objInfo;
	long volatile		state;
	ssize_tr volatile	resmonMemoryUsed;
	RsList				rsLists[MaximumResList];

	void *              shutDownEvent;
	void *              monitorThread;
	void *				mainThread;

	ResObjectCounters	counters;

	// snapshot

	MonSnapShot         sshot;

	// limits

	MonLimits           mlim;

	// mem details

	MonAllocs			mema[TOTAL_UPPER_LIMITS];

	// mon module

	OsModuleData		mod;

} ResContext;

//------------------------------------------------------------------------------
// атомарный доступ к переменным
//------------------------------------------------------------------------------
#if defined(_WIN64)
#define INTERLOCKED_ADD(val, add) _InterlockedExchangeAdd64( \
				(LONGLONG volatile *)(val),	(LONGLONG)(add))
#define INTERLOCKED_SUB(val, add) _InterlockedExchangeAdd64( \
				(LONGLONG volatile *)(val),	-((LONGLONG)(add)))
#define INTERLOCKED_INC(val) _InterlockedIncrement64((LONGLONG volatile *)(val))
#define INTERLOCKED_DEC(val) _InterlockedDecrement64((LONGLONG volatile *)(val))
#define INTERLOCKED_OR(val, add) _InterlockedOr64((LONGLONG volatile *)(val), add)

#else
#define INTERLOCKED_ADD(val, add) _InterlockedExchangeAdd((LONG volatile *)(val), \
				(LONG)(add))
#define INTERLOCKED_SUB(val, add) _InterlockedExchangeAdd((LONG volatile *)(val), \
				-((LONG)(add)))
#define INTERLOCKED_INC(val) _InterlockedIncrement((LONG volatile *)(val))
#define INTERLOCKED_DEC(val) _InterlockedDecrement((LONG volatile *)(val))
#define INTERLOCKED_OR(val, add) _InterlockedOr((LONG volatile *)(val), add)
#endif // sizeof(ULONG_PTR) == 32

//------------------------------------------------------------------------------
// GetResourceContext
//------------------------------------------------------------------------------
static ResContext * GetResourceContext(void)
{
	static ResContext ctx = {0};
	return &ctx;
}

//------------------------------------------------------------------------------
// GetResourceList
//------------------------------------------------------------------------------
static RsList * GetResourceList(ResourceListIndex index)
{
	ResContext * resCtx = GetResourceContext();
	ASSERT( index < MaximumResList );
	return &resCtx->rsLists[index];
}

//------------------------------------------------------------------------------
// ResMonitorState
//------------------------------------------------------------------------------
static SystemState ResMonitorState(
		SystemState needState,
		SystemState fromState )
{
	ResContext * rctx = GetResourceContext();
	return InterlockedCompareExchange(
							&rctx->state,
							(long)needState,
							(long)fromState );
}

//------------------------------------------------------------------------------
// AddResmonMemoryUsed
//------------------------------------------------------------------------------
extern ssize_tr AddResmonMemoryUsed(size_tr add)
{
	ResContext * rctx = GetResourceContext();
	ASSERT( rctx->resmonMemoryUsed >= 0 );
	return INTERLOCKED_ADD(&rctx->resmonMemoryUsed, add);
}
//------------------------------------------------------------------------------
// SubResmonMemoryUsed
//------------------------------------------------------------------------------
extern ssize_tr SubResmonMemoryUsed(size_tr sub)
{
	ResContext * rctx = GetResourceContext();
	ASSERT( rctx->resmonMemoryUsed >= (ssize_tr)sub );
	return INTERLOCKED_SUB(&rctx->resmonMemoryUsed, sub);
}

//------------------------------------------------------------------------------
// AllocObject
//------------------------------------------------------------------------------
static void * AllocObject(	ObjectType type,
							void * returnAddress,
							size_tr size)
{
	ResNode * obj = (ResNode *)OsAllocObject(size);

	ASSERT( ResMonitorState(ssWork, ssWork) == ssWork || \
			ResMonitorState(ssWork, ssWork) == ssInit );

	ASSERT( size >= sizeof(ResNode) );

	if( PTR(obj) ) {
		obj->objInfo.type = type;
		obj->objInfo.size = size;
		obj->returnAddress = returnAddress;
		obj->createThreadId = OsGetCurrentThreadId();
		AddResmonMemoryUsed(size);
	}
	return obj;
}

//------------------------------------------------------------------------------
// FreeObject
//------------------------------------------------------------------------------
static void FreeObject(ResNode * obj)
{
	ASSERT( ResMonitorState(ssWork, ssWork) == ssWork || \
			ResMonitorState(ssWork, ssWork) == ssUninit );
	ASSERT( obj->objInfo.size >= sizeof(ResNode) );

	SubResmonMemoryUsed(obj->objInfo.size);
	OsFreeObject(obj);
}

//------------------------------------------------------------------------------
// MonPrefix
//------------------------------------------------------------------------------
static uni_char * MonPrefix(void)
{
	ResContext * rctx = GetResourceContext();
	ASSERT( PTR(rctx->mod.basename.str) );
	return rctx->mod.basename.str;
}


//------------------------------------------------------------------------------
// AvlCreateResNode
//------------------------------------------------------------------------------
static ResNode * AvlCreateResNode(void * k, ResNode * n)
{
	ASSERT( PTR(n) );
	ASSERT( n->objInfo.size >= sizeof(ResNode) );

	n->key = k;
	n->left = n->right = 0; n->height = 1;
	return n;
}

//------------------------------------------------------------------------------
// AvlHeight
//------------------------------------------------------------------------------
static unsigned char AvlHeight(ResNode * p)
{
	return p?p->height:0;
}

//------------------------------------------------------------------------------
// AvlBFactor
//------------------------------------------------------------------------------
static int AvlBFactor(ResNode * p)
{
	return AvlHeight(p->right)-AvlHeight(p->left);
}

//------------------------------------------------------------------------------
// AvlFixHeight
//------------------------------------------------------------------------------
static void AvlFixHeight(ResNode * p)
{
	unsigned char hl = AvlHeight(p->left);
	unsigned char hr = AvlHeight(p->right);
	p->height = (hl>hr?hl:hr)+1;
}

//------------------------------------------------------------------------------
// AvlRotateRight
//------------------------------------------------------------------------------
static ResNode * AvlRotateRight(ResNode * p)
{
	ResNode * q = p->left;
	p->left = q->right;
	q->right = p;
	AvlFixHeight(p);
	AvlFixHeight(q);
	return q;
}

//------------------------------------------------------------------------------
// AvlRotateLeft
//------------------------------------------------------------------------------
static ResNode * AvlRotateLeft(ResNode * q)
{
	ResNode * p = q->right;
	q->right = p->left;
	p->left = q;
	AvlFixHeight(q);
	AvlFixHeight(p);
	return p;
}

//------------------------------------------------------------------------------
// AvlBalance
//------------------------------------------------------------------------------
static ResNode * AvlBalance(ResNode * p)
{
	AvlFixHeight(p);
	if( AvlBFactor(p) == 2 ) {
		if( AvlBFactor(p->right) < 0 )
			p->right = AvlRotateRight(p->right);
		return AvlRotateLeft(p);
	}
	if( AvlBFactor(p) == -2 ) {
		if( AvlBFactor(p->left) > 0  )
			p->left = AvlRotateLeft(p->left);
		return AvlRotateRight(p);
	}
	return p;
}

//------------------------------------------------------------------------------
// AvlInsert
//------------------------------------------------------------------------------
static ResNode * AvlInsert(ResNode * p, void * k, ResNode * n)
{
	if( !p ) return AvlCreateResNode(k, n);
	if( k<p->key )
		p->left = AvlInsert(p->left, k, n);
	else
		p->right = AvlInsert(p->right, k, n);
	return AvlBalance(p);
}

//------------------------------------------------------------------------------
// AvlFindMin
//------------------------------------------------------------------------------
static ResNode * AvlFindMin(ResNode * p)
{
	return p->left?AvlFindMin(p->left):p;
}

//------------------------------------------------------------------------------
// AvlGetResNode
//------------------------------------------------------------------------------
static ResNode * AvlGetResNode(ResNode * p, void * k)
{
	if( !p ) return (ResNode * )0;
	if(p->key < k) return AvlGetResNode(p->right, k);
	if(p->key > k) return AvlGetResNode(p->left, k);
	return p;
}

//------------------------------------------------------------------------------
// AvlRemoveMin
//------------------------------------------------------------------------------
static ResNode * AvlRemoveMin(ResNode * p)
{
	if( p->left==0 )
		return p->right;
	p->left = AvlRemoveMin(p->left);
	return AvlBalance(p);
}

//------------------------------------------------------------------------------
// AvlRemoveNodeByKey
//------------------------------------------------------------------------------
static ResNode * AvlRemoveNodeByKey(ResNode * p, void * k, ResNode ** removed)
{
	if( !p ) return 0;
	if( k < p->key )
		p->left = AvlRemoveNodeByKey(p->left, k, removed);
	else if( k > p->key )
		p->right = AvlRemoveNodeByKey(p->right, k, removed);
	else {
		ResNode * q = p->left;
		ResNode * r = p->right;
		ASSERT( *removed == 0 );
		*removed = p;
		if( !r ) return q;
		p = AvlFindMin(r);
		p->right = AvlRemoveMin(r);
		p->left = q;
	}
	return AvlBalance(p);
}

//------------------------------------------------------------------------------
// AvlRemoveAllNodesByTag
//------------------------------------------------------------------------------
static ResNode * AvlRemoveAllNodesByTag(ResNode * p, ptr_t tag)
{
	if( !p ) return (ResNode *)0;
	if( p->right )
		p->right = AvlRemoveAllNodesByTag(p->right, tag);
	if( p->left )
		p->left = AvlRemoveAllNodesByTag(p->left, tag);
	if( ((ResMemory * )p)->tag == tag ) {
		ResNode * q = p->left;
		ResNode * r = p->right;
		RsList * rsList = GetResourceList( MemoryResList );
		INTERLOCKED_SUB(&rsList->totalResources, ((ResMemory * )p)->size);
		FreeObject(p);
		if( !r ) return q;
		p = AvlFindMin(r);
		p->right = AvlRemoveMin(r);
		p->left = q;
	}
	return AvlBalance(p);
}

//------------------------------------------------------------------------------
// AqureResourceList
//------------------------------------------------------------------------------
static RsList * AcquireResourceList(
	__in ResourceListIndex index,
	__in signed allocResource)
{
	RsList * rsList = GetResourceList( index );
	SystemState	state = ResMonitorState(ssWork, ssWork);

	if( state != ssWork && (state != ssUninit || allocResource) )
		return 0;

	if( allocResource && rsList->accessSemaphore ) {

		if( OsWaitForSingleObject(rsList->accessSemaphore, INFINITE) != OsWaitObject0 ) {
			LOG_ERROR("OsWaitForSingleObject(accessSemaphore) error 0x%08X", \
						OsGetLastError());
			return NULL;
		}

		// check shutdown status
		if( ResMonitorState(ssWork, ssWork) != ssWork ) {
			LOG_ERROR("state != ssWork");
			OsReleaseSemaphore(rsList->accessSemaphore, 1, NULL);
			return NULL;
		}
	}

	OsAcquireResource(rsList->accessCs);

	return rsList;
}

//------------------------------------------------------------------------------
// Работа с памятью
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// MonAlloc
//------------------------------------------------------------------------------
extern void MonAlloc(void * adr,
					void * returnAddress,
					size_tr size,
					ptr_t tag,
					unsigned long mtype)
{
	RsList * rsList = AcquireResourceList(MemoryResList, TRUE);

	if( !rsList )
		return;

	if( PTR(adr) ) {
		ResMemory * resMem = (ResMemory *)AllocObject(ResMemoryObjectType,
											returnAddress, sizeof(ResMemory));
		if( PTR(resMem) ) {
			unsigned long mindex = 0;
			ResContext * resCtx = GetResourceContext();

			while(	mindex < COUNT_OF(resCtx->mema) ) {
				if( size <= resCtx->mema[mindex].upperLimit ) {
					INTERLOCKED_INC(&resCtx->mema[mindex].totalAllocs);
					break;
				}
				mindex++;
			}

			ASSERT( adr != 0 );
			resMem->size = size;
			resMem->tag = tag;
			resMem->mtype = mtype;

			ASSERT( AvlGetResNode(rsList->avl, adr) == 0 );

			rsList->avl = AvlInsert(rsList->avl, adr, (ResNode *)resMem);

			ASSERT( PTR(rsList->avl) );
			ASSERT( rsList->avl->objInfo.type == ResMemoryObjectType );
			ASSERT( rsList->avl->objInfo.size == sizeof(ResMemory));

			INTERLOCKED_ADD(&rsList->totalResources, size);
			ASSERT( rsList->totalResources >= size );
		}
	}

	ASSERT( rsList->accessSemaphore == 0 );
	OsReleaseResource(rsList->accessCs);
   	return;
}
//------------------------------------------------------------------------------
// MonFree
//------------------------------------------------------------------------------
extern void MonFree(void * ptr)
{
	RsList * rsList = AcquireResourceList(MemoryResList, FALSE);
	ResMemory * resMem = 0;

	if( !rsList )
		return;

	do {

		rsList->avl = AvlRemoveNodeByKey(rsList->avl, ptr, (ResNode **)&resMem);

		if( !resMem ) {
			LOG_WARN("pointer not found %p", ptr);
			break;
		}

		ASSERT( resMem->node.objInfo.type == ResMemoryObjectType );
		ASSERT( resMem->node.objInfo.size == sizeof(ResMemory) );
		ASSERT( rsList->totalResources >= resMem->size );

		INTERLOCKED_SUB(&rsList->totalResources, resMem->size);
		FreeObject((ResNode *)resMem);

	#pragma warning( suppress:4127 )
	} while(0);

	ASSERT( rsList->accessSemaphore == 0 );
	OsReleaseResource(rsList->accessCs);
	return;
}

//------------------------------------------------------------------------------
// MonFreeAllByTag
//------------------------------------------------------------------------------
extern void MonFreeAllByTag(ptr_t tag)
{
	RsList * rsList = AcquireResourceList(MemoryResList, FALSE);
	ResMemory * resMem = 0;

	if( !rsList )
		return;

	rsList->avl = AvlRemoveAllNodesByTag(rsList->avl, tag);

	ASSERT( rsList->accessSemaphore == 0 );
	OsReleaseResource(rsList->accessCs);
	return;
}

//------------------------------------------------------------------------------
// MonCreateThreadBegin
//------------------------------------------------------------------------------
extern void * MonCreateThreadBegin(void)
{
	return AcquireResourceList(ThreadResList, TRUE);
}

//------------------------------------------------------------------------------
// MonCreateThreadEnd
//------------------------------------------------------------------------------
extern void MonCreateThreadEnd(
						void * beginCtx,
						void * thread,
						void * returnAddress,
						void * startAddress,
						void * threadId)
{
	RsList * rsList = (RsList *)beginCtx;

	if( !PTR(beginCtx) )
		return;

	if( thread ) {
		ResThread *	resThread = (ResThread *)AllocObject(ResThreadObjectType,
												returnAddress,
												sizeof(ResThread));
		if( resThread ) {
			ASSERT( PTR(startAddress) );

			resThread->startAddress = startAddress;
			resThread->threadId = threadId;

			rsList->avl = AvlInsert(rsList->avl, thread, (ResNode *)resThread);

			ASSERT( PTR(rsList->avl) );
			ASSERT( rsList->avl->objInfo.type == ResThreadObjectType );
			ASSERT( rsList->avl->objInfo.size == sizeof(ResThread) );

			INTERLOCKED_INC(&rsList->totalResources);
		}
	}

	ASSERT( rsList->accessCs != 0 );
	OsReleaseResource(rsList->accessCs);

	if( !thread ) {
		ASSERT( rsList->accessSemaphore != 0 );
		OsReleaseSemaphore(rsList->accessSemaphore, 1, NULL);
	}
   	return;
}

//------------------------------------------------------------------------------
// MonAppendMonitoringTread
//------------------------------------------------------------------------------
extern signed MonAppendMonitoringTread(void * thread,
						void * returnAddress)
{
	RsList * rsList = AcquireResourceList(ThreadResList, TRUE);
	if( !AvlGetResNode(rsList->avl, thread) ) {
		MonCreateThreadEnd(
					(void*)rsList,
					thread,
					returnAddress,
					OsGetThreadStartAddress(thread),
					OsGetThreadId(thread));
		return TRUE;
	} else {
		OsReleaseResource(rsList->accessCs);
		OsReleaseSemaphore(rsList->accessSemaphore, 1, NULL);
	}
	return FALSE;
}

//------------------------------------------------------------------------------
// MonCreateSyncBegin
//------------------------------------------------------------------------------
extern void * MonCreateSyncBegin(void)
{
	return AcquireResourceList(SyncResList, TRUE);
}

//------------------------------------------------------------------------------
// MonCreateSyncEnd
//------------------------------------------------------------------------------
extern void MonCreateSyncEnd(
						void * beginCtx,
						void * sync,
						SyncType syncType,
						void * returnAddress,
						unsigned long parametr1,
						unsigned long parametr2)
{
	RsList * rsList = (RsList *)beginCtx;

	if( !PTR(beginCtx) )
		return;

	if( sync ) {
		ResSync * resSync = (ResSync *)AllocObject(ResSyncObjectType,
												returnAddress,
												sizeof(ResSync));
		if( resSync ) {

			resSync->node.objInfo.type = ResSyncObjectType;
			resSync->node.objInfo.size = sizeof(ResSync);

			ASSERT( syncType == EventSync || \
					syncType == MutexSync || \
					syncType == SemaphoreSync || \
					syncType == ResourceSync );

			resSync->syncType = syncType;

			// don`t care
			resSync->ctx.unknown.parametr1 = parametr1;
			resSync->ctx.unknown.parametr2 = parametr2;
    
			rsList->avl = AvlInsert(rsList->avl, sync, (ResNode *)resSync);
    
			ASSERT( PTR(rsList->avl) );
			ASSERT( rsList->avl->objInfo.type == ResSyncObjectType );
			ASSERT( rsList->avl->objInfo.size == sizeof(ResSync) );
    
			INTERLOCKED_INC(&rsList->totalResources);
		}
	}

	ASSERT( rsList->accessCs != 0 );
	OsReleaseResource(rsList->accessCs);

	if( !sync ) {
		ASSERT( rsList->accessSemaphore != 0 );
		OsReleaseSemaphore(rsList->accessSemaphore, 1, NULL);
	}
   	return;
}

//------------------------------------------------------------------------------
// MonClose
//------------------------------------------------------------------------------
extern void MonClose(void * obj)
{
	RsList * rsList = AcquireResourceList(SyncResList, FALSE);
	ResNode * resNode = 0;

	do {
		if( !rsList )
			break;

		rsList->avl = AvlRemoveNodeByKey(rsList->avl, obj, &resNode);
		if( resNode ) {
			ASSERT( resNode->objInfo.type == ResSyncObjectType );
			ASSERT( resNode->objInfo.size == sizeof(ResSync) );
			break;
		}
		OsReleaseResource(rsList->accessCs);

		rsList = AcquireResourceList(ThreadResList, FALSE);
		if( !rsList )
			break;
		rsList->avl = AvlRemoveNodeByKey(rsList->avl, obj, &resNode);
		if( resNode ) {
			ASSERT( resNode->objInfo.type == ResThreadObjectType );
			ASSERT( resNode->objInfo.size == sizeof(ResThread) );
			break;
		}

	#pragma warning( suppress:4127 )
	} while(0);

	if( resNode ) {
		ASSERT( PTR(rsList) );
		ASSERT( rsList->totalResources != 0 );
		INTERLOCKED_DEC(&rsList->totalResources);
		FreeObject(resNode);
	}
	
	if( rsList )
		OsReleaseResource(rsList->accessCs);

	if( resNode ) {
		ASSERT( PTR(rsList) );
		ASSERT( rsList->accessSemaphore != 0 );
		OsReleaseSemaphore(rsList->accessSemaphore, 1, NULL);
	}

	return;
}

//------------------------------------------------------------------------------
// ProcessThreadNodes
//------------------------------------------------------------------------------
static void ProcessThreadNodes(
				ResThread * resThread,
				const ResSystemTimes * timesInit,
				ResPercentTimes * pcnt)
{
	if(resThread->node.left)
		ProcessThreadNodes((ResThread *)resThread->node.left, timesInit, pcnt);

	if(resThread->node.right)
		ProcessThreadNodes((ResThread *)resThread->node.right, timesInit, pcnt);

	ASSERT( resThread->node.objInfo.type == ResThreadObjectType);
	ASSERT( resThread->node.objInfo.size == sizeof(ResThread));

	OsGetThreadPerfomance(
			resThread->node.key,
			&resThread->counters,
			timesInit);

	pcnt->cpuUserUsed += resThread->counters.pcnt.cpuUserUsed;
	pcnt->cpuKrnlUsed += resThread->counters.pcnt.cpuKrnlUsed;
	pcnt->cpuTotalUsed += resThread->counters.pcnt.cpuTotalUsed;

	return;
}

//------------------------------------------------------------------------------
// UpdateThreadsPerfomance
//------------------------------------------------------------------------------
static void UpdateThreadsPerfomance(
				const ResSystemTimes * timesInit,
				ResPercentTimes * pcnt,
				unsigned long * threadTotalUsed )
{
	RsList * rsList = NULL;

	ASSERT( pcnt->cpuUserUsed == 0 );
	ASSERT( pcnt->cpuKrnlUsed == 0 );
	ASSERT( pcnt->cpuTotalUsed == 0 );
	*threadTotalUsed = 0;

	rsList = AcquireResourceList(ThreadResList, FALSE);
	if( !rsList ) {
		LOG_ERROR("can`t acquire thread resource list");
		return;
	}

	if( PTR(rsList->avl) )
		ProcessThreadNodes((ResThread *)rsList->avl, timesInit, pcnt);

	*threadTotalUsed = (unsigned long)rsList->totalResources;

	OsReleaseResource(rsList->accessCs);
	return;
}

//------------------------------------------------------------------------------
// MonUpdateTimes
//------------------------------------------------------------------------------
extern void MonUpdateTimes(
				unsigned long long krnlTime,
				unsigned long long userTime,
				ResObjectCounters * cnt,
				const ResSystemTimes * timesInit)
{
	ResSystemTimes resTimes = {0};
	unsigned long long totalTime = 0;
	if(!OsGetSystemTimes(&resTimes) )
		return;

	ASSERT(krnlTime >= cnt->abst.KernelTime);
	ASSERT(userTime >= cnt->abst.UserTime);
	ASSERT(resTimes.kernelTime >= timesInit->kernelTime);
	ASSERT(resTimes.userTime >= timesInit->userTime);

	totalTime = (resTimes.kernelTime + resTimes.userTime) - \
		(timesInit->kernelTime + timesInit->userTime);

	if( totalTime ) {
		cnt->pcnt.cpuKrnlUsed = (unsigned char)( \
			((krnlTime - cnt->abst.KernelTime)*100) / totalTime );
		cnt->pcnt.cpuUserUsed = (unsigned char)( \
			((userTime - cnt->abst.UserTime)*100) / totalTime );
		cnt->pcnt.cpuTotalUsed = (unsigned char)( \
			(((userTime + krnlTime) - \
			(cnt->abst.KernelTime + cnt->abst.UserTime))*100) / totalTime );
	}
}

//------------------------------------------------------------------------------
// ResMonitorThread
//------------------------------------------------------------------------------
static unsigned long __stdcall ResMonitorThread(void * shutDownEvent)
{
	ResSystemTimes timesInit = {0};
	ResObjectCounters pcounters = {0};
	ResContext * resCtx = GetResourceContext();
	long exitStatus = 0;
	unsigned long lastError = OS_ERROR_NOT_READY;

	LOG_INFO(" ... start");

	ASSERT(shutDownEvent != NULL);
	if( !OsGetSystemTimes(&timesInit) ) {
		return OsExitThread(OsGetLastError());
	}

	do {
		RsList * rsList = NULL;
		unsigned long resMonInterval = 0, threadTotalUsed = 0, syncTotalUsed = 0;
		size_tr memoryTotalUsed = 0;

		resCtx->counters.pcnt.cpuUserUsed = 0;
		resCtx->counters.pcnt.cpuKrnlUsed = 0;
		resCtx->counters.pcnt.cpuTotalUsed = 0;

		// threads and cpu used
		UpdateThreadsPerfomance(
					&timesInit,
					&resCtx->counters.pcnt,
					&threadTotalUsed );

		if( resCtx->counters.pcnt.cpuTotalUsed )
			RESLOG("all threads used %u%% cpu (kernel %u%%, user %u%%)", \
				resCtx->counters.pcnt.cpuTotalUsed, \
				resCtx->counters.pcnt.cpuKrnlUsed, \
				resCtx->counters.pcnt.cpuUserUsed);

		OsGetProcessPerfomance(&pcounters, &timesInit);

		if( pcounters.pcnt.cpuTotalUsed || pcounters.diskUsed )
			RESLOG("process [%u] used %u%% cpu (kernel %u%%, user %u%%) " \
				"disk used %I64u bytes", OsGetCurrentProcessId(), \
				pcounters.pcnt.cpuTotalUsed, pcounters.pcnt.cpuKrnlUsed, \
				pcounters.pcnt.cpuUserUsed, pcounters.diskUsed );

		// sync object used
		rsList = AcquireResourceList(SyncResList, FALSE);
		if( rsList ) {
			syncTotalUsed = (unsigned long)rsList->totalResources;
			OsReleaseResource(rsList->accessCs);
		} else {
			LOG_ERROR("can`t acquire sync resource list");
		}

		// memory used
		rsList = AcquireResourceList(MemoryResList, FALSE);
		if( rsList ) {
			memoryTotalUsed = rsList->totalResources;
			OsReleaseResource(rsList->accessCs);
		} else {
			LOG_ERROR("can`t acquire memory resource list");
		}

		resCtx->sshot.threadUsed = threadTotalUsed;
		resCtx->sshot.syncObjectUsed = syncTotalUsed;
		resCtx->sshot.memoryUsed = memoryTotalUsed;

		RESLOG("threads used %u, sync object used %u, memory used %u", \
			resCtx->sshot.threadUsed, \
			resCtx->sshot.syncObjectUsed, \
			resCtx->sshot.memoryUsed);

		// fist time (lastError == ERROR_NOT_READY) not informative
		if( lastError != OS_ERROR_NOT_READY ) {

			if( resCtx->sshot.memoryUsed > resCtx->mlim.maxMemoryUsedLimit ) {
				// Превышение лимита памяти - выводим логи
				RESLOG("update memory limit: from 0x%p to 0x%p", \
					resCtx->mlim.maxMemoryUsedLimit, resCtx->sshot.memoryUsed);
			}

			if( resCtx->counters.pcnt.cpuTotalUsed > resCtx->mlim.maxCpuUsedLimit ) {

				// Единственно что можем сделать - увеличить паузу 
				// между снятием показателей
				ASSERT(	resCtx->mlim.resMonInterval*2 > resCtx->mlim.resMonInterval );
				resCtx->mlim.resMonInterval += \
				(resCtx->mlim.resMonInterval * (resCtx->counters.pcnt.cpuTotalUsed - resCtx->mlim.maxCpuUsedLimit))/100;

				RESLOG("update cpu used limit overflow from %u%% to %u%%", \
					resCtx->mlim.maxCpuUsedLimit, resCtx->counters.pcnt.cpuTotalUsed);
				RESLOG("monitoring pause update to %u ms", resCtx->mlim.resMonInterval);

				resCtx->mlim.maxCpuUsedLimit = resCtx->counters.pcnt.cpuTotalUsed;

			}

			if( resCtx->sshot.threadUsed >= resCtx->mlim.maxThreadUsedLimit ) {
				// Если можем продолжить в указанных
				// условиях - увеличим лимит
				rsList = GetResourceList( ThreadResList );
				if( OsReleaseSemaphore(rsList->accessSemaphore,
					(resCtx->sshot.threadUsed - resCtx->mlim.maxThreadUsedLimit)+1,
					NULL) ) {
					RESLOG("update thread limit: %u to %u", \
						resCtx->mlim.maxThreadUsedLimit, resCtx->sshot.threadUsed);
					resCtx->mlim.maxThreadUsedLimit = resCtx->sshot.threadUsed;
				} else {
					LOG_ERROR("ReleaseSemaphore(thread) error %u", \
								OsGetLastError());
				}
			}

			if( resCtx->sshot.syncObjectUsed >= resCtx->mlim.maxSyncObjectUsedLimit ) {
				// Если можем продолжить в указанных
				// условиях - увеличим лимит
				// Без захвата ресурса освобождаем семафор
				rsList = GetResourceList( SyncResList );
				if( OsReleaseSemaphore(rsList->accessSemaphore,
					(resCtx->sshot.syncObjectUsed - resCtx->mlim.maxSyncObjectUsedLimit)+1,
					NULL) ) {
					RESLOG("update sync limit: %u to %u", \
					resCtx->mlim.maxSyncObjectUsedLimit, resCtx->sshot.syncObjectUsed);
					resCtx->mlim.maxSyncObjectUsedLimit = resCtx->sshot.syncObjectUsed;
				} else {
					LOG_ERROR("ReleaseSemaphore(sync) error %u", \
								OsGetLastError());
				}
			}

			if( resCtx->counters.diskUsed >= resCtx->mlim.maxDiskUsedLimit ) {
				RESLOG("update disk used limit overflow from %I64u to %I64u", \
					resCtx->mlim.maxDiskUsedLimit, resCtx->counters.diskUsed);
				resCtx->mlim.maxDiskUsedLimit = resCtx->counters.diskUsed;
			}
		}

		resMonInterval = resCtx->mlim.resMonInterval;
		ASSERT(resMonInterval > 0);

		OsGetSystemTimes(&timesInit);

		lastError = OsWaitForSingleObject(
						shutDownEvent,
						resMonInterval);

		if( lastError != OsWaitObject0 &&
			lastError != OsWaitTimeout ) {
			LOG_ERROR("WaitForSingleObject(shutDownEvent) error %u", \
						OsGetLastError());
		}

	} while(lastError == OsWaitTimeout);

	LOG_INFO(" ... finish");
	exitStatus = OsExitThread(lastError);
	return exitStatus;
}
//------------------------------------------------------------------------------
// ProcessMemoryLogNodes
//------------------------------------------------------------------------------
static void ProcessMemoryLogNodes(ResMemory * resMem)
{
	if(resMem->node.left)
		ProcessMemoryLogNodes((ResMemory *)resMem->node.left);

	if(resMem->node.right)
		ProcessMemoryLogNodes((ResMemory *)resMem->node.right);

	ASSERT( resMem->node.objInfo.type == ResMemoryObjectType);
	ASSERT( resMem->node.objInfo.size >= sizeof(ResMemory));

	RESLOG("base 0x%p size %u tag %p (%s)", \
		(char *)resMem->node.key, resMem->size, \
		resMem->tag,
		GetMemoryTypeName(resMem->mtype));
	RESLOG("created by [%u] thread", resMem->node.createThreadId);
	RESLOG("returnAddress 0x%p", resMem->node.returnAddress );
	RESLOG("---------------------------------------------------");
	return;
}

//------------------------------------------------------------------------------
// CheckMemoryResources
//------------------------------------------------------------------------------
static signed CheckMemoryResources(void)
{
	RsList * rsList = AcquireResourceList(MemoryResList, FALSE);
	if( !rsList ) {
		LOG_ERROR("can`t acquire memory resource list");
		return FALSE;
	}

	#if defined(_WIN64)
	RESLOG("------------------ memory usage %I64d -----------------", \
				rsList->totalResources);
	#else
	RESLOG("------------------ memory usage %u -----------------", \
				rsList->totalResources);
	#endif

	if(rsList->avl)
		ProcessMemoryLogNodes((ResMemory *)rsList->avl);
	OsReleaseResource(rsList->accessCs);
	return rsList->avl ? FALSE:TRUE;
}

//------------------------------------------------------------------------------
// ProcessThreadLogNodes
//------------------------------------------------------------------------------
static void ProcessThreadLogNodes(ResThread * resThread)
{
	if(resThread->node.left)
		ProcessThreadLogNodes((ResThread *)resThread->node.left);

	if(resThread->node.right)
		ProcessThreadLogNodes((ResThread *)resThread->node.right);

	ASSERT( resThread->node.objInfo.type == ResThreadObjectType);
	ASSERT( resThread->node.objInfo.size == sizeof(ResThread));

	RESLOG("handle(ptr) 0x%p id %u", \
				resThread->node.key, resThread->threadId);
	if( resThread->startAddress == ResMonitorThread ) {
		RESLOG("start address 0x%p (ResMonitorThread)", resThread->startAddress);
	} else {
		ResContext * resCtx = GetResourceContext();
		RESLOG("start address 0x%p%s", resThread->startAddress, \
			resCtx->mainThread == resThread->node.key ? " (MainThread)":"");
	}
	RESLOG("thread create time 0x%I64X", \
		resThread->counters.abst.CreateTime );
	RESLOG("thread exit time 0x%I64X", resThread->counters.abst.ExitTime );
	RESLOG("thread [%u] used %u%% cpu (kernel %u%%, user %u%%)", \
		resThread->threadId, resThread->counters.pcnt.cpuTotalUsed, \
		resThread->counters.pcnt.cpuKrnlUsed, \
		resThread->counters.pcnt.cpuUserUsed);
	RESLOG("created by [%u] thread", resThread->node.createThreadId);
	RESLOG("returnAddress 0x%p", resThread->node.returnAddress );
	RESLOG("---------------------------------------------------");
	return;
}

//------------------------------------------------------------------------------
// CheckThreadResources
//------------------------------------------------------------------------------
static signed CheckThreadResources(void)
{
	RsList * rsList = AcquireResourceList(ThreadResList, FALSE);
	if( !rsList ) {
		LOG_ERROR("can't acquire thread resource list");
		return FALSE;
	}

	RESLOG("------------------ thread usage %u -----------------", \
				(unsigned long)rsList->totalResources);

	if(rsList->avl)
		ProcessThreadLogNodes((ResThread *)rsList->avl);

	OsReleaseResource(rsList->accessCs);
	return rsList->avl ? FALSE:TRUE;
}

//------------------------------------------------------------------------------
// ProcessSyncLogNodes
//------------------------------------------------------------------------------
static void ProcessSyncLogNodes(ResSync * resSync)
{
	if(resSync->node.left)
		ProcessSyncLogNodes((ResSync *)resSync->node.left);

	if(resSync->node.right)
		ProcessSyncLogNodes((ResSync *)resSync->node.right);

	ASSERT( resSync->node.objInfo.type == ResSyncObjectType);
	ASSERT( resSync->node.objInfo.size == sizeof(ResSync));

	switch(resSync->syncType) {
		case EventSync:
			RESLOG("event %p type: %s reset, initial state: %s", \
				resSync->node.key, \
				resSync->ctx.event.manualReset ? "manual": "auto",\
				resSync->ctx.event.initialState ? \
				"signaled":"nonsignaled");
			break;
		case MutexSync:
			RESLOG("mutex %p initial state: %s", \
				resSync->node.key, \
				resSync->ctx.mutex.initialOwner ? "acquire": "free");
			break;
		case SemaphoreSync:
			RESLOG("semaphore %p initial count: %d maximum count %d", \
				resSync->node.key, \
				resSync->ctx.semaphore.initialCount, \
				resSync->ctx.semaphore.maximumCount);
			break;
		case ResourceSync:
			RESLOG("resource %p", resSync->node.key);
			break;
		default:
			LOG_ERROR("incorrect sync object");
			break;
	}
	RESLOG("created by [%u] thread", resSync->node.createThreadId);
	RESLOG("returnAddress 0x%p", resSync->node.returnAddress );
	RESLOG("---------------------------------------------------");
	return;
}

//------------------------------------------------------------------------------
// CheckSyncResources
//------------------------------------------------------------------------------
static signed CheckSyncResources(void)
{
	RsList * rsList = AcquireResourceList(SyncResList, FALSE);
	if( !rsList ) {
		LOG_ERROR("can't acquire sync resource list");
		return FALSE;
	}
	RESLOG("--------------- sync object usage %u ---------------", \
				(unsigned long)rsList->totalResources);

	if(rsList->avl)
		ProcessSyncLogNodes((ResSync *)rsList->avl);

	OsReleaseResource(rsList->accessCs);
	return rsList->avl ? FALSE:TRUE;
}

//------------------------------------------------------------------------------
// FreeResources
//------------------------------------------------------------------------------
static FreeResources(void)
{
	ResourceListIndex resIndex = 0;
	ResContext * resCtx = GetResourceContext();

	ASSERT( ResMonitorState(ssUndefined, ssUndefined) != ssUndefined );

	while( resIndex <  MaximumResList ) {
		RsList * rsList = GetResourceList(resIndex++);

		ASSERT( PTR(rsList) );

		if( rsList->accessSemaphore ) {
			// Освобождаем хотя бы один семафор,
			// остальные по цепочке освободит AcquireResourceList
			OsReleaseSemaphore(rsList->accessSemaphore, 1, 0);
		}

		// Ожидаем когда все заврешат работу
		OsAcquireResource(rsList->accessCs);
		OsReleaseResource(rsList->accessCs);
		if( rsList->accessSemaphore ) {
			OsClose(rsList->accessSemaphore);
			rsList->accessSemaphore = 0;
		}
		OsDeleteResourceObject(rsList->accessCs);
		rsList->accessCs = 0;


		// Все ресурсы на этом этапе должны быть освобождены
		ASSERT( rsList->avl == 0 );
		ASSERT( rsList->totalResources == 0 );
	}

	ASSERT( resCtx->resmonMemoryUsed == 0 );

	OsFreeModuleData(&resCtx->mod);

	resCtx->objInfo.type = UnknownObjectType;
	resCtx->objInfo.size = 0;
}

//------------------------------------------------------------------------------
// CreateResources
//------------------------------------------------------------------------------
static signed CreateResources(MonLimits * limits)
{
	ResourceListIndex resIndex = 0;
	ResContext * resCtx = GetResourceContext();
	unsigned long mindex = 0, regcnt = COUNT_OF(resCtx->mema);
	signed res = TRUE;
	static MonLimits defLimits = {
			MON_MAXIMUM_MEMORY_USAGE,
			MON_MAXIMUM_DISK_USAGE,
			MON_MAXIMUM_THREAD_USAGE,
			MON_MAXIMUM_SYNCHRONIZATION_OBJECT_USAGE,
			MON_MAXIMUM_CPU_USAGE,
			MON_DEFAULT_RES_MONITORING_INTERVAL};

	ASSERT( ResMonitorState(ssUndefined, ssUndefined) != ssUndefined );

	resCtx->objInfo.type = ResContextObjectType;
	resCtx->objInfo.size = sizeof(ResContext);

	if( limits == MON_DEFAULT_LIMITS )
		limits = &defLimits;

	ASSERT( PTR(limits) );

	OsMoveMemory(&resCtx->mlim, limits, sizeof(MonLimits));

	ASSERT( resCtx->mlim.maxThreadUsedLimit >= 1 );
	ASSERT( resCtx->mlim.maxSyncObjectUsedLimit >= 2 );

	resCtx->resmonMemoryUsed = 0;

	while(	mindex < regcnt ) {
		// set any values if nedeed
		static size_tr upperLimit[1] = {MINIMUM_UPPER_LIMIT_START};
		/*	48,
			96,
			192,
			384,
			768,
			1024,
			2048,
			4096,
			16384,
			32768,
			65536,
			1048576,
			4194304,
			16777216,
			33554432,
			0xFFFFFFFFFFFFFFFF};*/
		if( mindex == regcnt-1 )
			resCtx->mema[mindex].upperLimit = (size_tr)(-1);
		else if( mindex < COUNT_OF(upperLimit) )
			resCtx->mema[mindex].upperLimit = upperLimit[mindex];
		else if( mindex < (regcnt/2) )
			resCtx->mema[mindex].upperLimit = ((size_tr)MINIMUM_UPPER_LIMIT_START) << mindex;
		else if( mindex < ((regcnt/2)+(regcnt/4)) )
			resCtx->mema[mindex].upperLimit = ((size_tr)MEDIUM_UPPER_LIMIT_START) << (mindex-(regcnt/2));
		else
			resCtx->mema[mindex].upperLimit = ((size_tr)HIGH_UPPER_LIMIT_START) << (mindex - ((regcnt/2)+(regcnt/4)));

		resCtx->mema[mindex].totalAllocs = 0;
		mindex++;
	}

	while(	resIndex <  MaximumResList  ) {
		RsList * rsList = GetResourceList(resIndex++);
		ASSERT( PTR(rsList) );
		ASSERT(rsList->avl == NULL);
		ASSERT(rsList->accessSemaphore == NULL);
		ASSERT(rsList->totalResources == 0);
    
		// Все ресурсы имеют синхронизацию доступа
		rsList->accessCs = OsCreateResourceObject();
		if( !rsList->accessCs ) {
			res = FALSE;
			break;
		}
    
		// Все кроме памяти имеют лимит с шагом 1
		if( resIndex == (MemoryResList+1) )
			continue;

		ASSERT( resCtx->mlim.maxSyncObjectUsedLimit > 0 );
		ASSERT( resCtx->mlim.maxThreadUsedLimit > 0 );
   
		if( resIndex == (ThreadResList+1) )
			rsList->accessSemaphore = OsCreateSemaphore(
					resCtx->mlim.maxThreadUsedLimit,
					OS_MAXIMUM_LIMIT_VALUE);
		else if( resIndex == (SyncResList+1) )
			rsList->accessSemaphore = OsCreateSemaphore(
					resCtx->mlim.maxSyncObjectUsedLimit,
					OS_MAXIMUM_LIMIT_VALUE);
    
		if( !rsList->accessSemaphore ) {
			res = FALSE;
			break;
		}
	}

	if( !res )
		FreeResources();

	return res;
}

//------------------------------------------------------------------------------
// MemoryUsedStatistic
//------------------------------------------------------------------------------
static void MemoryUsedStatistic(void)
{
	unsigned long mindex = 0;
	size_tr begin = 0;
	ResContext * resCtx = GetResourceContext();

	RESLOG("-------------------- allocations detail ----------------------------");
#if defined(_WIN64)
	RESLOG("| size range                            | total allocs");
#else
	RESLOG("| size range            | total allocs");
#endif

	while(	mindex < COUNT_OF(resCtx->mema) ) {
		if( resCtx->mema[mindex].totalAllocs )
			RESLOG("|0x%08p - 0x%08p| 0x%p(" SIZE_TR_STR ")",
				begin,
				resCtx->mema[mindex].upperLimit,
				resCtx->mema[mindex].totalAllocs,
				resCtx->mema[mindex].totalAllocs);
		begin = resCtx->mema[mindex].upperLimit;
		mindex++;
	}
	RESLOG("--------------------------------------------------------------------");
}

//------------------------------------------------------------------------------
// IsAllThreadsTerminated
//------------------------------------------------------------------------------
static signed IsAllThreadsTerminated(ResThread * resThread)
{
	signed res = 1;
	if(resThread->node.left)
		res = IsAllThreadsTerminated((ResThread *)resThread->node.left);

	if(resThread->node.right)
		res = IsAllThreadsTerminated((ResThread *)resThread->node.right);

	ASSERT( resThread->node.objInfo.type == ResThreadObjectType);
	ASSERT( resThread->node.objInfo.size == sizeof(ResThread));

	if( OsWaitForSingleObject(resThread->node.key, 0) == \
		OsWaitTimeout ) {
		RESLOG("thread %p active(not terminated)", \
			resThread->node.key);
		res = 0;
	}
	return res;
}

//------------------------------------------------------------------------------
// MonLogAllResources
//------------------------------------------------------------------------------
extern signed __stdcall MonLogAllResources(void)
{
	RsList * rsList = AcquireResourceList(MemoryResList, FALSE);
	ResContext * resCtx = GetResourceContext();
	signed notFree = FALSE;
	if( !rsList ) {
		//LOG_ERROR("can`t acquire memory resource list");
		return TRUE;
	}

	#if defined(_WIN64)
	RESLOG("------------ resmon memory usage %I64d ----------------", \
				resCtx->resmonMemoryUsed);
	RESLOG("------------------ memory usage %I64d -----------------", \
				rsList->totalResources);
	#else
	RESLOG("------------ resmon memory usage %u ----------------", \
				resCtx->resmonMemoryUsed);
	RESLOG("------------------ memory usage %u -----------------", \
				rsList->totalResources);
	#endif

	if(rsList->avl)
		ProcessMemoryLogNodes((ResMemory *)rsList->avl);

	notFree |= (rsList->avl ? TRUE:FALSE);
	OsReleaseResource(rsList->accessCs);

	rsList = AcquireResourceList(ThreadResList, FALSE);
	if( !rsList ) {
		LOG_ERROR("can't acquire thread resource list");
		return TRUE;
	}

	RESLOG("------------------ thread usage %u -----------------", \
				(unsigned long)rsList->totalResources);

	if(rsList->avl)
		ProcessThreadLogNodes((ResThread *)rsList->avl);

	notFree |= (rsList->avl ? TRUE:FALSE);
	OsReleaseResource(rsList->accessCs);

	rsList = AcquireResourceList(SyncResList, FALSE);
	if( !rsList ) {
		LOG_ERROR("can't acquire sync resource list");
		return TRUE;
	}
	RESLOG("--------------- sync object usage %u ---------------", \
				(unsigned long)rsList->totalResources);

	if(rsList->avl)
		ProcessSyncLogNodes((ResSync *)rsList->avl);

	notFree |= (rsList->avl ? TRUE:FALSE);
	OsReleaseResource(rsList->accessCs);

	MemoryUsedStatistic();

	return notFree;
}

//------------------------------------------------------------------------------
// UninstallResMonitoringSystem
//------------------------------------------------------------------------------
extern signed __stdcall UninstallResMonitoringSystem(void)
{
	SystemState	state = ssUndefined;
	ResContext * resCtx = GetResourceContext();
	signed res = FALSE;

	// Деинициализация возможна при переходе статуса ssWork->ssUninit
	if( ResMonitorState( ssUninit, ssWork ) != ssWork ) {
		LOG_ERROR("error state != ssWork");
		return FALSE;
	}

	ASSERT( resCtx->objInfo.type == ResContextObjectType );
	ASSERT( resCtx->objInfo.size = sizeof(ResContext) );

	if( resCtx->mainThread ) {
		RsList * rsList = 0;
		OsCloseThread(resCtx->mainThread);
		MonClose(resCtx->mainThread);
		resCtx->mainThread = 0;
	}

	if( resCtx->shutDownEvent ) {
		OsSetEvent( resCtx->shutDownEvent );
		if( resCtx->monitorThread ) {
			OsWaitForSingleObject(resCtx->monitorThread, OS_INFINITE);
			OsCloseThread(resCtx->monitorThread);
			MonClose(resCtx->monitorThread);
			resCtx->monitorThread = 0;
		}
		OsClose(resCtx->shutDownEvent);
		resCtx->shutDownEvent = 0;
	}

	RESLOG("=========== CHECK UNRELEASED RESOUCRES ============");
	if( MonLogAllResources() ) {
		RsList * rsList = AcquireResourceList(ThreadResList, FALSE);

		RESLOG("error ... not all resources free");

		if( rsList ) {
			if( rsList->avl ) {
				if( !IsAllThreadsTerminated((ResThread *)rsList->avl) ) {
					RESLOG("error can`t unload " \
						"... active thread(s) detect");
					OsReleaseResource(rsList->accessCs);
					state = ResMonitorState(ssWork, ssUninit);
					ASSERT( state == ssUninit );
					return FALSE;
				}
			}
			OsReleaseResource(rsList->accessCs);
		}

	}
	RESLOG("===================================================");

	RestoreImportHooks();

	state = ResMonitorState(ssCheck, ssUninit);
	ASSERT( state == ssUninit );

	FreeResources();

	state = ResMonitorState(ssUndefined, ssCheck);
	ASSERT( state == ssCheck );

	return res;
}
//------------------------------------------------------------------------------
// InstallResMonitoringSystem
//------------------------------------------------------------------------------
extern signed __stdcall InstallResMonitoringSystem(void * base, MonLimits * limits)
{
	SystemState	state = ssUndefined;
	ResContext * resCtx = GetResourceContext();
	signed res = FALSE;

	// Инициализация возможна при переходе статуса ssUndefined->ssInit
	if(	ResMonitorState(ssInit, ssUndefined) != ssUndefined	) {
		LOG_ERROR("error state != ssUndefined");
		return FALSE;
	}

	do {

		if( !CreateResources(limits) )
			break;

		if( !OsBaseAndDataFromPointer(PTR(base) ? base:_ReturnAddress(), \
				&resCtx->mod) ) {
			LOG_ERROR("OsBaseAndDataFromPointer() error");
			break;
		}

		ASSERT( PTR(resCtx->mod.base) );
		ASSERT( resCtx->mod.imageSize > 0 );
		ASSERT( PTR(resCtx->mod.basename.str) );

		if( SetImportHooks(&resCtx->mod) != 0 )
			break;

		state = ResMonitorState(ssWork, ssInit);
		ASSERT( state == ssInit );

		resCtx->shutDownEvent = OsCreateEvent(TRUE, FALSE);
		if( !resCtx->shutDownEvent ) {
			LOG_ERROR("OsCreateEvent(shutDownEvent) error");
			break;
		}

		resCtx->monitorThread = OsCreateThread(
							#pragma warning( suppress:4054 )
							(void *)ResMonitorThread,
							resCtx->shutDownEvent,
							0,
							NULL);
		if( !resCtx->monitorThread ) {
			LOG_ERROR("OsCreateThread(ResMonitorThread) error");
		}

		if( resCtx->monitorThread )
			MonAppendMonitoringTread(resCtx->monitorThread,
						#pragma warning( suppress:4054 )
						(void *)InstallResMonitoringSystem);

		resCtx->mainThread = OsOpenCurrentThread();
		if( resCtx->mainThread )
			MonAppendMonitoringTread(resCtx->mainThread,
						_ReturnAddress());

	#pragma warning( suppress:4127 )
	} while(0);

	if( !resCtx->monitorThread ) {
		if( ResMonitorState(ssWork, ssWork) == ssWork ) {
			UninstallResMonitoringSystem();
		} else {
			ASSERT( resCtx->shutDownEvent == 0 );
			FreeResources();
			state = ResMonitorState(ssUndefined, ssInit);
			ASSERT( state == ssInit );
		}
		res = FALSE;
	}

	return res;
}
