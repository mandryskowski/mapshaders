#ifndef UTIL_H
#define UTIL_H

#if defined(_WIN32)
#  define MAPSHADERS_COMPILER_DLLEXPORT __declspec(dllexport)
#  define MAPSHADERS_COMPILER_DLLIMPORT __declspec(dllimport)
#elif defined(__GNUC__)
#  define MAPSHADERS_COMPILER_DLLEXPORT __attribute__ ((visibility ("default")))
#  define MAPSHADERS_COMPILER_DLLIMPORT __attribute__ ((visibility ("default")))
#else
#  define MAPSHADERS_COMPILER_DLLEXPORT
#  define MAPSHADERS_COMPILER_DLLIMPORT
#endif

#ifndef MAPSHADERS_DLL_SYMBOL
#  if defined(MAPSHADERS_STATIC_EXPORTS)
#    define MAPSHADERS_DLL_SYMBOL
#  elif defined(MAPSHADERS_SHARED_EXPORTS)
#    define MAPSHADERS_DLL_SYMBOL MAPSHADERS_COMPILER_DLLEXPORT
#  else
#    define MAPSHADERS_DLL_SYMBOL MAPSHADERS_COMPILER_DLLIMPORT
#  endif
#endif

#endif // UTIL_H