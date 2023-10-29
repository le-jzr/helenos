
#ifndef COMMON_INCLUDE_UCHAR_H_
#define COMMON_INCLUDE_UCHAR_H_

#include <stdint.h>
#include <_bits/mbstate_t.h>

typedef uint_least16_t char16_t;
typedef uint_least32_t char32_t;

size_t mbrtoc16(char16_t *restrict, const char *restrict, size_t, mbstate_t *restrict);
size_t c16rtomb(char *restrict, char16_t, mbstate_t *restrict);
size_t mbrtoc32(char32_t *restrict, const char *restrict, size_t, mbstate_t *restrict);
size_t c32rtomb(char *restrict, char32_t, mbstate_t *restrict);

#endif /* COMMON_INCLUDE_UCHAR_H_ */
