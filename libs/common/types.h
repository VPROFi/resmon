#ifndef __TYPES_H__
#define __TYPES_H__

//******************************************************************************
// Глобальные определения, структуры и перечисления
// данный хидер OS независимый
//******************************************************************************

#if !defined(UNUSED)
#define UNUSED(p) {(p)=(p);}
#endif

#if defined(_WIN64)
typedef unsigned long long ptr_t;
typedef unsigned long long size_tr;
typedef signed long long ssize_tr;
#define SIZE_TR_STR "%I64u"
#else
typedef unsigned long ptr_t;
typedef unsigned long size_tr;
typedef signed long ssize_tr;
#define SIZE_TR_STR "%u"
#endif

//------------------------------------------------------------------------------
// работаем только с unicode
//------------------------------------------------------------------------------

typedef unsigned short uni_char;

typedef struct {
	uni_char *   str;
	size_tr      size; // Размер в байтах без завершающего нуля
} UniStr;

typedef struct {
	const uni_char * str;
	size_tr          size; // Размер в байтах без завершающего нуля
} UniStrConst;

#define INIT_CONST_UNISTR(uniStr, constStr) (uniStr)->str = (uni_char *)(constStr); \
					(uniStr)->size = sizeof(constStr)-sizeof(uni_char)

//------------------------------------------------------------------------------
// отдельный вид utf8
//------------------------------------------------------------------------------
typedef struct {
	unsigned char * str;
	unsigned int size; // Размер в байтах без завершающего нуля
} Utf8Str;

typedef struct {
	const unsigned char * str;
	unsigned int size; // Размер в байтах без завершающего нуля
} Utf8StrConst;

#define INIT_CONST_UTF8STR(utf8Str, constStr) (utf8Str)->str = (unsigned char * )(constStr); \
					(utf8Str)->size = sizeof(constStr)-sizeof(unsigned char)

//------------------------------------------------------------------------------
// Базовые типы
//------------------------------------------------------------------------------
typedef struct ListEntry ListEntry;

struct ListEntry {
	ListEntry * fLink;
	ListEntry * bLink;
};

typedef struct {
	void *  data;
	size_tr size;
} Buffer;

typedef enum {
	ssUndefined,
	ssInit,
	ssWork,
	ssUninit,
	ssCheck,
	ssMaximum = 0XFFFFFFFFL
} SystemState;

//------------------------------------------------------------------------------
// Все структуры и объекты типизированны
//------------------------------------------------------------------------------
#define OBJECT_UNKNOWN_SIZE 0
typedef struct {
	unsigned long type; // ObjectType
	size_tr       size;
} ObjectInfo;

typedef enum {
	UnknownObjectType,
	ResNodeObjectType,		// Объект нода
	ResContextObjectType,	// Функциональная система мониторинга ресурсов
	ResSyncObjectType,		// Объект синхронизации
	ResMemoryObjectType,	// Объект участок памяти
	ResThreadObjectType,	// Объект тред
	TaskContextObjectType,	// Функциональная система управления задачами
	TaskObjectType,			// Объект задача
	MaximumObjectType,
	MaximumValueObjectType = 0xFFFF
} ObjectType;

#endif // __TYPES_H__
