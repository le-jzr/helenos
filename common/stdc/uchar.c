
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

static bool _is_low_surrogate(char16_t c)
{
	return c >= 0xDC00 && c < 0xE000;
}

static bool _is_high_surrogate(char16_t c)
{
	return c >= 0xD800 && c < 0xDC00;
}

static bool _is_surrogate(char16_t c)
{
	return c >= 0xD800 && c < 0xE000;
}

static bool _is_continuation(uint8_t c)
{
	return (c & 0xC0) == 0x80;
}

static bool _is_1_byte(uint8_t c)
{
	return (c & 0x80) == 0;
}

static bool _is_2_byte(uint8_t c)
{
	return (c & 0xE0) == 0xC0;
}

static bool _is_3_byte(uint8_t c)
{
	return (c & 0xF0) == 0xE0;
}

static bool _is_4_byte(uint8_t c)
{
	return (c & 0xF8) == 0xF0;
}

size_t mbrtoc32(char32_t *c, const char *s, size_t n, mbstate_t *mb)
{
	static mbstate_t global_state = { };

	if (n == 0)
		return RETVAL_INCOMPLETE;

	if (!mb)
		mb = &global_state;

	char32_t dummy;

	if (!c)
		c = &dummy;

	if (!s) {
		// Equivalent to mbrtoc32(NULL, "", 1, mb).
		if (mb->continuation) {
			_set_ilseq();
			return RETVAL_ILSEQ;
		} else {
			return 0;
		}
	}

	size_t i = 0;

	if (!mb->continuation) {
		/* Clean slate, read initial byte. */

		uint8_t b = s[i++];

		if (_is_1_byte(b)) {
			*c = b;
			return b == 0 ? 0 : 1;
		}

		if (_is_continuation(b)) {
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

		if (_is_2_byte(b)) {
			/* 2 byte encoding               110xxxxx */
			mb->continuation = b ^ 0b0000000011000000;

		} else if (_is_3_byte(b)) {
			/* 3 byte encoding               1110xxxx */
			mb->continuation = b ^ 0b1111110011100000;

		} else if (_is_4_byte(b)) {
			/* 4 byte encoding               11110xxx */
			mb->continuation = b ^ 0b1111111100000000;
		}
	}

	while (i < n) {
		/* Read continuation bytes. */

		if (!_is_continuation(s[i])) {
			_set_ilseq();
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
	if (!s) {
		// Equivalent to c32rtomb(buf, L’\0’, mb).
		return 1;
	}

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
		if (_is_surrogate(c)) {
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
	static mbstate_t global_state = { };

	if (!mb)
		mb = &global_state;

	char16_t dummy;

	if (!c)
		c = &dummy;

	if (!s) {
		// Equivalent to mbrtoc16(NULL, "", 1, mb).
		if (mb->continuation) {
			_set_ilseq();
			return RETVAL_ILSEQ;
		} else {
			return 0;
		}
	}

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
	static mbstate_t global_state = { };

	if (!mb)
		mb = &global_state;

	if (!s) {
		// Equivalent to c16rtomb(buf, L’\0’, mb).
		if (mb->continuation) {
			_set_ilseq();
			return RETVAL_ILSEQ;
		} else {
			return 1;
		}
	}

	if (!_is_surrogate(c)) {
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
	if (_is_low_surrogate(mb->continuation) && _is_high_surrogate(c)) {
		c32 = ((c - 0xD7C0) << 10) | (mb->continuation - 0xDC00);
	} else if (_is_high_surrogate(mb->continuation) && _is_low_surrogate(c)) {
		c32 = ((mb->continuation - 0xD7C0) << 10) | (c - 0xDC00);
	} else {
		_set_ilseq();
		return RETVAL_ILSEQ;
	}

	mb->continuation = 0;
	return c32rtomb(s, c32, mb);
}
