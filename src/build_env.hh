//=============================================================================
/// Build environment
//=============================================================================

#ifndef _BUILD_ENV_HH_
#define _BUILD_ENV_HH_

// Windows, Visual Studio
#ifdef _MSC_VER
#include <stdarg.h>

#define snprintf  _snprintf
#define va_copy(dst, src)  (dst = src)

#else
#include <alloca.h>
#endif

#endif
