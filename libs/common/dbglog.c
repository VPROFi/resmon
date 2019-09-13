#ifdef DBG
#include <common/dbglog.h>
#include <common/checkptr.h>
//==============================================================================
// debug api
C_ASSERT( sizeof("unknown") < DEBUG_MODULE_MAX_NAME_LEN );
//------------------------------------------------------------------------------
// getmoduleprefix
//------------------------------------------------------------------------------
extern uni_char * getmoduleprefix(void * returnAddress)
{
	static uni_char prefix[DEBUG_MODULE_MAX_NAME_LEN] = {0};
	if( !(*prefix) ) {
		OsModuleData mod = {0};
		if( OsBaseAndDataFromPointer(returnAddress, &mod) ) {
			ASSERT( PTR(mod.basename.str) );
			ASSERT( mod.basename.size >= sizeof(uni_char) );
			OsMoveMemory(prefix, mod.basename.str, 
				(mod.basename.size+sizeof(uni_char) < \
				(DEBUG_MODULE_MAX_NAME_LEN)*sizeof(uni_char)) ? \
				mod.basename.size+sizeof(uni_char): \
				(DEBUG_MODULE_MAX_NAME_LEN-1)*sizeof(uni_char));
			OsFreeModuleData(&mod);
		} else
			OsMoveMemory(prefix, L"unknown", sizeof(L"unknown"));

		ASSERT( prefix[DEBUG_MODULE_MAX_NAME_LEN-1] == 0 );
	}
	return prefix;
}
#endif // DBG
