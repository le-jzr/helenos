#ifndef __BITS_LIMITS_H_
#define __BITS_LIMITS_H_

#include <_bits/macros.h>

/* _MIN macros for unsigned types are non-standard (and of course, always 0),
 * but we already have them for some reason, so whatever.
 */

#define CHAR_BIT __CHAR_BIT__

#define SCHAR_MIN __SCHAR_MIN__
#define SCHAR_MAX __SCHAR_MAX__

#define UCHAR_MIN 0
#define UCHAR_MAX __UCHAR_MAX__

#define CHAR_MIN __CHAR_MIN__
#define CHAR_MAX __CHAR_MAX__

#define MB_LEN_MAX 16

#define SHRT_MIN __SHRT_MIN__
#define SHRT_MAX __SHRT_MAX__

#define USHRT_MIN 0
#define USHRT_MAX __USHRT_MAX__

#define INT_MIN __INT_MIN__
#define INT_MAX __INT_MAX__

#define UINT_MIN 0U
#define UINT_MAX __UINT_MAX__

#define LONG_MIN __LONG_MIN__
#define LONG_MAX __LONG_MAX__

#define ULONG_MIN 0UL
#define ULONG_MAX __ULONG_MAX__

#define LLONG_MIN __LLONG_MIN__
#define LLONG_MAX __LLONG_MAX__

#define ULLONG_MIN 0ULL
#define ULLONG_MAX __ULLONG_MAX__

#endif // __BITS_LIMITS_H_
