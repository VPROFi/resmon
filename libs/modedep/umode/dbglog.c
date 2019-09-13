#ifdef _KERNEL_MODE
#error This code only for user mode
#endif

#define _CRT_SECURE_NO_WARNINGS
#include <common/dbglog.h>
#include <stdio.h>
#include <stdarg.h>

#ifdef DBG
//------------------------------------------------------------------------------
// dbgoutput
//------------------------------------------------------------------------------
#pragma warning( push )
#pragma warning( disable:4212 )
signed __cdecl dbgoutput(const char * msg, uni_char * prefix_place, ...)
{
	char outMsg[MAX_MSG_LEN + 1];
	va_list arguments;

	if( !msg )
		return FALSE;
	*(&prefix_place) = getmoduleprefix(_ReturnAddress());
	va_start(arguments, msg);

	if( _vsnprintf(outMsg, MAX_MSG_LEN, msg, arguments) < 0  ) {
		OutputDebugStringA("dbglog error: _vsnprintf() error\n");
		return FALSE;
	}
	va_end(arguments);
	OutputDebugStringA(outMsg);
	return TRUE;
}
#pragma warning( pop )
#endif // DBG

//------------------------------------------------------------------------------
// DbgPrint
//------------------------------------------------------------------------------
extern signed __cdecl DbgPrint(const char * msg, ...)
{
	char outMsg[MAX_MSG_LEN + 1];
	va_list arguments;

	if( !msg )
		return FALSE;
	va_start(arguments, msg);

	if( _vsnprintf(outMsg, MAX_MSG_LEN, msg, arguments) < 0  ) {
		OutputDebugStringA("DbgPrint error: _vsnprintf() error\n");
		return FALSE;
	}
	va_end(arguments);
	OutputDebugStringA(outMsg);
	return TRUE;
}
