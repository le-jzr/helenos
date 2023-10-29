
#include <uchar.h>
#include <limits.h>
#include <stdbool.h>

#if __STDC_HOSTED__
#include <errno.h>
#define _set_ilseq() (errno = EILSEQ)
#else
#define _set_ilseq() ((void) 0)
#endif

#define RETVAL_ILSEQ      ((size_t) -1)
#define RETVAL_INCOMPLETE ((size_t) -2)
#define RETVAL_CONTINUED  ((size_t) -3)

static bool is_low_surrogate(char16_t c)
{
	return c >= 0xDC00 && c < 0xE000;
}

static bool is_high_surrogate(char16_t c)
{
	return c >= 0xD800 && c < 0xDC00;
}

static bool is_surrogate(char16_t c)
{
	return c >= 0xD800 && c < 0xE000;
}

static bool is_continuation(uint8_t c)
{
	return (c & 0xC0) == 0x80;
}

static bool is_1_byte(uint8_t c)
{
	return (c & 0x80) == 0;
}

static bool is_2_byte(uint8_t c)
{
	return (c & 0xE0) == 0xC0;
}

static bool is_3_byte(uint8_t c)
{
	return (c & 0xF0) == 0xE0;
}

static bool is_4_byte(uint8_t c)
{
	return (c & 0xF8) == 0xF0;
}

size_t mbrtoc32(char32_t *c, const char *s, size_t n, mbstate_t *mb)
{
	if (n == 0)
		return RETVAL_INCOMPLETE;

	size_t i = 0;

	if (!mb->continuation) {
		/* Clean slate, read initial byte. */

		uint8_t b = s[i++];

		if (is_1_byte(b)) {
			*c = b;
			return 1;
		}

		if (is_continuation(b)) {
			/* unexpected continuation byte */
			_set_ilseq();
			return RETVAL_ILSEQ;
		}

		/*
		 * The value stored into `continuation` is designed to have
		 * just enough leading ones that after shifting in one less than
		 * the expected number of continuation bytes, the most significant
		 * bit becomes zero. (The field is 16b wide.)
		 */

		if (is_2_byte(b)) {
			/* 2 byte encoding               110xxxxx */
			mb->continuation = b ^ 0b0000000011000000;

		} else if (is_3_byte(b)) {
			/* 3 byte encoding               1110xxxx */
			mb->continuation = b ^ 0b1111110011100000;

		} else if (is_4_byte(b)) {
			/* 4 byte encoding               11110xxx */
			mb->continuation = b ^ 0b1111111100000000;
		}
	}

	while (i < n) {
		/* Read continuation bytes. */

		if (!is_continuation(s[i])) {
			_seq_ilseq();
			return RETVAL_ILSEQ;
		}

		/* Top bit becomes zero just before the last byte is shifted in. */
		if (!(mb->continuation & 0x8000)) {
			*c = ((char32_t) mb->continuation) << 6 | (s[i++] & 0x3f);
			mb->continuation = 0;
			return i;
		}

		mb->continuation = mb->continuation << 6 | (s[i++] & 0x3f);
	}

	return RETVAL_INCOMPLETE;
}

#define UTF8_CONT(c, shift) (0x80 | (((c) >> (shift)) & 0x3F))

size_t c32rtomb(char *s, char32_t c, mbstate_t *mb)
{
	uint8_t bytes[4];

	/* 1 byte encoding */
	if (c < 0x80) {
		s[0] = c;
		return 1;
	}

	/* 2 byte encoding */
	if (c < 0x800) {
		s[0] = 0b11000000 | (c >> 6);
		s[1] = UTF8_CONT(c, 0);
		return 2;
	}

	/* 3 byte encoding */
	if (c < 0x10000) {
		if (is_surrogate(c)) {
			/* illegal range for an unicode code point */
			_set_ilseq();
			return RETVAL_ILSEQ;
		}

		s[0] = 0b11100000 | (c >> 12);
		s[1] = UTF8_CONT(c, 6);
		s[2] = UTF8_CONT(c, 0);
		return 3;
	}

	/* 4 byte encoding */
	if (c < 0x110000) {
		s[0] = 0b11110000 | (c >> 18);
		s[1] = UTF8_CONT(c, 12);
		s[2] = UTF8_CONT(c, 6);
		s[3] = UTF8_CONT(c, 0);
		return 4;
	}

	_set_ilseq();
	return RETVAL_ILSEQ;

}

size_t mbrtoc16(char16_t *c, const char *s, size_t n, mbstate_t *mb)
{
	if (mb->continuation) {
		*c = mb->continuation;
		mb->continuation = 0;
		return RETVAL_CONTINUED;
	}

	char32_t c32 = 0;
	size_t ret = mbrtoc32(&c32, s, n, mb);
	if (ret < INT_MAX) {
		if (c32 < 0x10000) {
			*c = c32;
		} else {
			/* Encode UTF-16 surrogates. */
			mb->continuation = (c32 & 0x3FF) + 0xDC00;
			*c = (c32 >> 10) + 0xD7C0;
		}
		return ret;
	}

	return ret;
}

size_t c16rtomb(char *s, char16_t c, mbstate_t *mb)
{
	if (!is_surrogate(c)) {
		if (mb->continuation) {
			_set_ilseq();
			return RETVAL_ILSEQ;
		}

		return c32rtomb(s, c, mb);
	}

	if (!mb->continuation) {
		mb->continuation = c;
		return 0;
	}

	char32_t c32;

	/* Decode UTF-16 surrogates. */
	if (is_low_surrogate(mb->continuation) && is_high_surrogate(c)) {
		c32 = ((c - 0xD7C0) << 10) | (mb->continuation - 0xDC00);
	} else if (is_high_surrogate(mb->continuation) && is_low_surrogate(c)) {
		c32 = ((mb->continuation - 0xD7C0) << 10) | (c - 0xDC00);
	} else {
		_set_ilseq();
		return RETVAL_ILSEQ;
	}

	return c32rtomb(s, c32, mb);
}
