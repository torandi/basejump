#ifndef _MSC_VER
#error This header should only be included when compiling under MSVC.
#endif

#define strdup(x) _strdup(x)
#define snprintf sprintf_s
