
#ifndef LTE_EXPORT_H
#define LTE_EXPORT_H

#ifdef LTE_STATIC_DEFINE
#  define LTE_EXPORT
#  define LTE_NO_EXPORT
#else
#  ifndef LTE_EXPORT
#    ifdef lte_EXPORTS
        /* We are building this library */
#      define LTE_EXPORT __attribute__((visibility("default")))
#    else
        /* We are using this library */
#      define LTE_EXPORT __attribute__((visibility("default")))
#    endif
#  endif

#  ifndef LTE_NO_EXPORT
#    define LTE_NO_EXPORT __attribute__((visibility("hidden")))
#  endif
#endif

#ifndef LTE_DEPRECATED
#  define LTE_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef LTE_DEPRECATED_EXPORT
#  define LTE_DEPRECATED_EXPORT LTE_EXPORT LTE_DEPRECATED
#endif

#ifndef LTE_DEPRECATED_NO_EXPORT
#  define LTE_DEPRECATED_NO_EXPORT LTE_NO_EXPORT LTE_DEPRECATED
#endif

/* NOLINTNEXTLINE(readability-avoid-unconditional-preprocessor-if) */
#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef LTE_NO_DEPRECATED
#    define LTE_NO_DEPRECATED
#  endif
#endif

// Undefine the *_EXPORT symbols for non-Windows based builds
#ifndef NS_MSVC
#undef LTE_EXPORT
#define LTE_EXPORT
#undef LTE_NO_EXPORT
#define LTE_NO_EXPORT
#endif
#endif /* LTE_EXPORT_H */
