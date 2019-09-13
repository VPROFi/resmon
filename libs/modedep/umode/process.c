#ifdef _KERNEL_MODE
#error This code only for user mode
#endif
#include <common/dbglog.h>
#include <common/checkptr.h>
#include <modedep/modedep.h>
//------------------------------------------------------------------------------
// OsGetCurrentThreadId
//------------------------------------------------------------------------------
extern void * OsGetCurrentProcessId(void)
{
	return (void *)GetCurrentProcessId();
}
