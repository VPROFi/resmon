#ifndef _KERNEL_MODE
#error This code only for kernel mode
#endif
#include <common/dbglog.h>
#include <common/checkptr.h>
#include <modedep/modedep.h>
//------------------------------------------------------------------------------
// OsGetCurrentThreadId
//------------------------------------------------------------------------------
extern void * OsGetCurrentProcessId(void)
{
	return (void *)PsGetCurrentProcessId();
}
