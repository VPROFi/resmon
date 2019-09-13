#ifndef __DBGLOG_H__
#define __DBGLOG_H__

#include <modedep/modedep.h>

#define DEBUG_MODULE_MAX_NAME_LEN 20
#define MAX_MSG_LEN (1024+DEBUG_MODULE_MAX_NAME_LEN)

#ifdef DBG
	EXTERN_C signed __cdecl dbgoutput(const char * msg, uni_char * prefix_place, ...);
	EXTERN_C uni_char * getmoduleprefix(void * returnAddress);

	#define LOG_PROTO(msg, level, ...) dbgoutput("%ws [%u] |" level "| " __FUNCTION__ "(): " msg "\n", (void *)0, OsGetCurrentThreadId(), ## __VA_ARGS__)
	#define LOG_ERROR(msg, ...) LOG_PROTO(msg, "ERROR", ## __VA_ARGS__) 
	#define LOG_INFO(msg, ...)  LOG_PROTO(msg, "INFO",  ## __VA_ARGS__) 
	#define LOG_WARN(msg, ...)  LOG_PROTO(msg, "WARN",  ## __VA_ARGS__) 

#else
	#define LOG_ERROR(msg, ...) __noop
	#define LOG_INFO(msg, ...)  __noop
	#define LOG_WARN(msg, ...)  __noop
#endif // DBG

#endif // __DBGLOG_H__
