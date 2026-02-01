
#ifndef TML_API_H
#define TML_API_H

#ifdef TEXTMATE_STATIC
#  define TML_API
#  define TML_NO_EXPORT
#else
#  ifndef TML_API
#    ifdef tml_EXPORTS
        /* We are building this library */
#      define TML_API 
#    else
        /* We are using this library */
#      define TML_API 
#    endif
#  endif

#  ifndef TML_NO_EXPORT
#    define TML_NO_EXPORT 
#  endif
#endif

#ifndef TML_DEPRECATED
#  define TML_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef TML_DEPRECATED_EXPORT
#  define TML_DEPRECATED_EXPORT TML_API TML_DEPRECATED
#endif

#ifndef TML_DEPRECATED_NO_EXPORT
#  define TML_DEPRECATED_NO_EXPORT TML_NO_EXPORT TML_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef TML_NO_DEPRECATED
#    define TML_NO_DEPRECATED
#  endif
#endif

#endif /* TML_API_H */
