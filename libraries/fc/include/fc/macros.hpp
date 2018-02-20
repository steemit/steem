#pragma once

/// Place to put some general purpose utility macros...

#define _GET_NTH_ARG(_1, _2, _3, _4, _5, N, ...) N

#define _fe_0(_call, ...)
#define _fe_1(_call, x) _call(x)
#define _fe_2(_call, x, ...) _call(x) _fe_1(_call, __VA_ARGS__)
#define _fe_3(_call, x, ...) _call(x) _fe_2(_call, __VA_ARGS__)
#define _fe_4(_call, x, ...) _call(x) _fe_3(_call, __VA_ARGS__)

#define MACRO_FOR_EACH(x, ...) \
   _GET_NTH_ARG("ignored", ##__VA_ARGS__, \
   _fe_4, _fe_3, _fe_2, _fe_1, _fe_0)(x, ##__VA_ARGS__)

#define DO_PRAGMA(x) _Pragma (#x)

#ifdef __GNUC__

#define FC_UNUSED1(x) (void)x;

/// Macro to prevent unused variable warnings printed by some compilers.
#define FC_UNUSED(...) MACRO_FOR_EACH(FC_UNUSED1, ##__VA_ARGS__)

#else /// __GNUC__

/// Macro to prevent unused variable warnings printed by some compilers.
#define FC_UNUSED(...) DO_PRAGMA(unused(__VA_ARGS__))
#endif /// __GNUC__

/// Macro to force printing of TODO message at compiler output.
#define FC_TODO(msg) DO_PRAGMA(message("TODO: " #msg))
