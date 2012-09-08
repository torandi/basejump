#ifndef _MSC_VER
#error This header should only be included when compiling under MSVC.
#endif

/* missing definition of ssize_t (signed version of size_t) */
#ifdef  _WIN64
typedef unsigned __int64 ssize_t;
#elif defined(_WIN32)
typedef signed int ssize_t;
#endif

/* missing definitions of M_PI and friends */
#define M_E 2.71828182845904523536
#define M_LOG2E 1.44269504088896340736
#define M_LOG10E 0.434294481903251827651
#define M_LN2 0.693147180559945309417
#define M_LN10 2.30258509299404568402
#define M_PI 3.14159265358979323846
#define M_PI_2 1.57079632679489661923
#define M_PI_4 0.785398163397448309616
#define M_1_PI 0.318309886183790671538
#define M_2_PI 0.636619772367581343076
#define M_1_SQRTPI 0.564189583547756286948
#define M_2_SQRTPI 1.12837916709551257390
#define M_SQRT2 1.41421356237309504880
#define M_SQRT_2 0.707106781186547524401

/* Windows.h must be included early since it defines crap which must be undefined */
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define NOCOMM
#define NOSOUND
#include <Windows.h>

/* Fuck you microsoft, sincerly - me */
#undef near
#undef far

/* missing functions */
#include <cmath>
static inline int isblank(char c) { return (c == ' ' || c == '\t'); };
static inline double round(double x){ return x >= 0.0  ? floor(x + 0.5)   : ceil(x - 0.5);   }
static inline float  roundf(float x){ return x >= 0.0f ? floorf(x + 0.5f) : ceilf(x - 0.5f); }
