#ifndef PLATFORM_H
#define PLATFORM_H

#ifdef __clang__
#       define __PURE__
#       define __CONST__
#       define __DEPRECATED__ __attribute__ ((deprecated))
#       define __SENTINEL__
#       define __HOT__
#       define __COLD__
#       define __NONNULL__(...)
#       define __NORETURN__
#       define __UNUSED__
#       define __WARN_UNUSED__
#       define __ALIGNED__(size) decl
#elif defined(__GNUC__)
#       define __PURE__ __attribute__((pure))
#       define __CONST__ __attribute__((const))
#       define __DEPRECATED__ __attribute__ ((deprecated))
#       define __SENTINEL__ __attribute__ ((sentinel))
#       define __HOT__ __attribute__ ((hot))
#       define __COLD__ __attribute__ ((cold))
#       define __NONNULL__(...) __attribute__((nonnull (__VA_ARGS__)))
#       define __NORETURN__ __attribute__((noreturn))
#       define __UNUSED__ __attribute__ ((unused))
#       define __WARN_UNUSED__ __attribute__((warn_unused_result))
#       define __ALIGNED__(size) __attribute__((aligned(size)))
#elif defined(WIN32)
#       define __PURE__
#       define __CONST__
#       define __DEPRECATED__ __declspec(deprecated)
#       define __SENTINEL__
#       define __HOT__
#       define __COLD__
#       define __NONNULL__(...)
#       define __NORETURN__
#       define __UNUSED__
#       define __WARN_UNUSED__
#       define __ALIGNED__(size) __declspec(align(size))
#else
#       error Unknown compiler/platform
#endif

#endif /* PLATFORM_H */
