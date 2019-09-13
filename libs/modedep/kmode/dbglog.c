#ifndef _KERNEL_MODE
#error This code only for kenel mode
#endif

#ifdef DBG
#include <common/dbglog.h>
#include <stdarg.h>
//------------------------------------------------------------------------------
// dbgoutput
//------------------------------------------------------------------------------
#pragma warning( push )
#pragma warning( disable:4212 )
extern int __cdecl _vsnprintf(char* buffer, size_t bufferCount, const char * format, va_list argList);
signed __cdecl dbgoutput(const char * msg, uni_char * prefix_place, ...)
{
	char outMsg[MAX_MSG_LEN + 1];
	va_list arguments;

	if( !msg )
		return FALSE;
	*(&prefix_place) = getmoduleprefix(_ReturnAddress());
	va_start(arguments, msg);

	if( _vsnprintf(outMsg, MAX_MSG_LEN, msg, arguments) < 0  ) {
		DbgPrint("dbglog error: _vsnprintf() error\n");
		return FALSE;
	}
	va_end(arguments);
	DbgPrint(outMsg);
	return TRUE;
}
#pragma warning( pop )
#endif // DBG
