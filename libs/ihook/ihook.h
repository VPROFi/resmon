#ifndef __IHOOK_H__
#define __IHOOK_H__

#include <modedep/modedep.h>

#ifdef __cplusplus
extern "C"
{
#endif

long SetImportHooks(OsModuleData * mod);
void RestoreImportHooks(void);

#ifdef __cplusplus
}
#endif

#endif // __IHOOK_H__
