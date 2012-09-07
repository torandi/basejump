#ifndef _MSC_VER
#error This header should only be included when compiling under MSVC.
#endif

#ifdef  _WIN64
typedef unsigned __int64 ssize_t;
#elif defined(_WIN32)
typedef signed int ssize_t;
#endif
