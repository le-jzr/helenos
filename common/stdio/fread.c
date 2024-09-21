
#include <stdio_core.h>
#include <stdio_file.h>

#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
<<<<<<< HEAD
#include <string.h>
=======
#include <mem.h>
>>>>>>> stdio_tmp
#include <macros.h>

#if __STDC_HOSTED__
#define _set_errno(expr) (errno = (expr))
#else
#define _set_errno(expr) ((void)(expr))
#endif

#define unimplemented() assert(!"unimplemented")

void flockfile(FILE *stream)
{
	if (stream->ops->lock)
		stream->ops->lock(stream->lock_handle);
}

int ftrylockfile(FILE *stream)
{
	if (stream->ops->try_lock) {
		return stream->ops->try_lock(stream->lock_handle) ? 0 : -1;
	} else {
		return 0;
	}
}

void funlockfile(FILE *stream)
{
	if (stream->ops->unlock)
		stream->ops->unlock(stream->lock_handle);
}

static bool _buffer_empty(FILE *stream)
{
	return stream->buffer_head == stream->buffer_tail;
}

static bool _buffer_full(FILE *stream)
{
	return stream->buffer_tail == stream->buffer_end || stream->buffer_head == stream->buffer_tail + 1;
}

static size_t _buffer_size(FILE *stream)
{
	return stream->buffer_end - stream->buffer;
}

<<<<<<< HEAD
static void _flush_before_write(FILE *stream)
{
	/* If buffer contains read data, flush buffer. */
=======
static bool _lazy_alloc_buffer(FILE *stream)
{
	if (stream->buffer != NULL)
		return true;

	uint8_t *buf = malloc(BUFSIZ);
	if (!buf)
		return false;

	stream->buffer = buf;
	stream->buffer_end = buf + BUFSIZ;
	stream->buffer_head = buf;
	stream->buffer_tail = buf;

	stream->allocated_buffer = true;
	return true;
}

static void _flush_before_write(FILE *stream)
{
	/* If buffer contains read data, flush it. */
>>>>>>> stdio_tmp
	if (stream->buffer_state == _bs_read) {
		stream->buffer_head = stream->buffer;
		stream->buffer_tail = stream->buffer;
		stream->buffer_state = _bs_empty;
<<<<<<< HEAD
	}
}

static size_t _partial_flush_buffer(FILE *stream)
=======

		/* If the stream is seekable, correct the current position. */
		if (stream->ops->seek) {
			stream->ops->seek(stream->stream_handle, -(long) stream->position_offset, SEEK_CUR);
			stream->position_offset = 0;
		}
	}
}

static size_t _try_flush_buffer(FILE *stream)
>>>>>>> stdio_tmp
{
	assert(stream->buffer_state != _bs_read);

	if (stream->buffer_head > stream->buffer_tail) {
		/* Split buffer. Write only the first portion. */

		assert(stream->btype != _IOLBF);

		size_t n = stream->ops->write(stream->stream_handle,
		    stream->buffer_head, stream->buffer_end - stream->buffer_head,
		    &stream->error);

		stream->buffer_head += n;

		if (stream->buffer_head == stream->buffer_end)
			stream->buffer_head = stream->buffer;

		return n;
	}

	uint8_t *tail = stream->buffer_tail;
	if (stream->btype == _IOLBF) {
		/* Try to output in only whole lines when line buffered. */

		uint8_t *endl = memrchr(stream->buffer_head, '\n', stream->buffer_tail - stream->buffer_head);
		if (endl) {
			while (endl < stream->buffer_tail && (*endl == '\n' || *endl == '\r'))
				endl++;

			tail = endl;
		}
	}

	size_t n = stream->ops->write(stream->stream_handle,
	    stream->buffer_head, tail - stream->buffer_head, &stream->error);

	stream->buffer_head += n;

	if (stream->buffer_head == stream->buffer_tail) {
		stream->buffer_head = stream->buffer;
		stream->buffer_tail = stream->buffer;
		stream->buffer_state = _bs_empty;
	}

	return n;
}

<<<<<<< HEAD
static void _partial_fill_buffer(FILE *stream)
=======
static void _try_fill_buffer(FILE *stream)
>>>>>>> stdio_tmp
{
	assert(stream->buffer_state != _bs_write);
	stream->buffer_state = _bs_read;

	/* We only read into buffer when it's empty. */
	assert(_buffer_empty(stream));

	size_t read = stream->ops->read(stream->stream_handle,
	    stream->buffer, stream->buffer_end - stream->buffer, &stream->error);

	stream->buffer_head = stream->buffer;
	stream->buffer_tail = stream->buffer + read;
<<<<<<< HEAD
=======
	stream->position_offset = read;
>>>>>>> stdio_tmp
}

static size_t _write_to_buffer(FILE *stream, const void *src, size_t size)
{
	assert(stream->buffer_state != _bs_read);
	stream->buffer_state = _bs_write;

	if (stream->buffer_tail == stream->buffer_end) {
		if (stream->buffer_head == stream->buffer)
			return 0;

		stream->buffer_tail = stream->buffer;
	}

	size_t n;

	if (stream->buffer_head > stream->buffer_tail) {
		n = stream->buffer_head - stream->buffer_tail - 1;
	} else {
		n = stream->buffer_end - stream->buffer_tail;
	}

	n = min(n, size);

	memcpy(stream->buffer_tail, src, n);
	stream->buffer_tail += n;

	return n;
}

static size_t _read_from_buffer(FILE *stream, void *dest, size_t size)
{
	assert(stream->buffer_state != _bs_write);
	assert(stream->buffer_head <= stream->buffer_tail);

	size_t n = stream->buffer_tail - stream->buffer_head;
	if (n > size) {
		memcpy(dest, stream->buffer_head, size);
		stream->buffer_head += size;
<<<<<<< HEAD
=======
		stream->position_offset -= size;
>>>>>>> stdio_tmp
		return size;
	} else {
		memcpy(dest, stream->buffer_head, n);
		stream->buffer_head = stream->buffer;
		stream->buffer_tail = stream->buffer;
<<<<<<< HEAD
=======
		stream->position_offset -= size;
>>>>>>> stdio_tmp
		return n;
	}
}

static int _flush_all(FILE *stream)
{
	while (!_buffer_empty(stream)) {
<<<<<<< HEAD
		if (_partial_flush_buffer(stream) == 0) {
=======
		if (_try_flush_buffer(stream) == 0) {
>>>>>>> stdio_tmp
			stream->error = true;
			return EOF;
		}
	}

<<<<<<< HEAD
	return EOF;
}

static bool _buffer_contains_newline(FILE *stream)
{
	assert(stream->buffer_head <= stream->buffer_tail);
	return memchr(stream->buffer_head, '\n', stream->buffer_tail - stream->buffer_head) != NULL;
=======
	return 0;
}

static uint8_t *_newline_in_buffer(FILE *stream)
{
	assert(stream->buffer_head <= stream->buffer_tail);
	return memchr(stream->buffer_head, '\n', stream->buffer_tail - stream->buffer_head);
>>>>>>> stdio_tmp
}

static void _flush_after_write(FILE *stream)
{
	if (stream->btype == _IOFBF ||
<<<<<<< HEAD
	    (stream->btype == _IOLBF && _buffer_contains_newline(stream))) {
=======
	    (stream->btype == _IOLBF && _newline_in_buffer(stream) != NULL)) {
>>>>>>> stdio_tmp

		_flush_all(stream);
	}
}

int fflush(FILE *stream)
{
	if (stream == NULL) {
		// TODO
		unimplemented();
	}

	flockfile(stream);
	int rc = _flush_all(stream);
	funlockfile(stream);
	return rc;
}

static size_t _fread(FILE *stream, void *dest, size_t total)
{
<<<<<<< HEAD
=======
	/* If buffer contains written data, flush buffer to output first. */
	if (stream->buffer_state == _bs_write && _flush_all(stream) == EOF)
		return 0;

>>>>>>> stdio_tmp
	/* If there are data in the buffer, copy those first. */
	size_t read = _read_from_buffer(stream, dest, total);

	while (read < total) {
		assert(_buffer_empty(stream));

<<<<<<< HEAD
		if (stream->btype == _IONBF || total - read >= _buffer_size(stream)) {
			/* If unbuffered, or the read is larger than the buffer, read directly. */
=======
		if (total - read >= _buffer_size(stream)) {
			/* If the read is larger than the buffer, read directly. */
>>>>>>> stdio_tmp
			size_t n = stream->ops->read(stream->stream_handle, dest + read, total - read, &stream->error);
			if (n == 0)
				break;

			read += n;
		} else {
<<<<<<< HEAD
			_partial_fill_buffer(stream);
=======
			_try_fill_buffer(stream);
>>>>>>> stdio_tmp
			if (_buffer_empty(stream))
				break;

			read += _read_from_buffer(stream, dest + read, total - read);
		}
	}

	if (!stream->error && read < total)
		stream->eof = true;

	return read;
}

<<<<<<< HEAD
=======
static int _fgetc(FILE *stream)
{
	uint8_t c;
	size_t read = _fread(stream, &c, 1);
	return read == 0 ? EOF : c;
}

static char *_fgets_unbuffered(FILE *stream, char *s, int total)
{
	/* No buffer to make this faster. */

	for (int n = 0; n + 1 < total; n++) {
		int c = _fgetc(stream);
		if (c == EOF) {
			if (n == 0)
				return NULL;

			s[n] = '\0';
			return s;
		}

		s[n] = c;

		if (c == '\n') {
			s[n + 1] = '\0';
			return s;
		}
	}

	s[total - 1] = '\0';
	return s;
}

static char *_fgets(FILE *stream, char *s, int total)
{
	if (total == 0)
		return NULL;

	if (stream->buffer == NULL)
		return _fgets_unbuffered(stream, s, total);

	/* If buffer contains written data, flush buffer to output first. */
	if (stream->buffer_state == _bs_write && _flush_all(stream) == EOF)
		return NULL;

	int n = 0;
	while (n + 1 < total) {
		if (_buffer_empty(stream)) {
			_try_fill_buffer(stream);

			if (_buffer_empty(stream))
				break;
		}

		/* Find newline in the buffer. */
		uint8_t *newline = _newline_in_buffer(stream);

		/* Don't read past the newline. */
		size_t max_read = total - n - 1;
		assert(newline >= stream->buffer_head);
		if (newline)
			max_read = min(max_read, (size_t) (newline - stream->buffer_head) + 1);

		n += _read_from_buffer(stream, s + n, max_read);

		if (newline)
			break;
	}

	if (n == 0 && total > 1)
		return NULL;

	s[n] = '\0';
	return s;
}


int fgetc(FILE *stream)
{
	flockfile(stream);
	int c = _fgetc(stream);
	funlockfile(stream);
	return c;
}

char *fgets(char *s, int n, FILE *stream)
{
	flockfile(stream);
	s = _fgets(stream, s, n);
	funlockfile(stream);
	return s;
}

>>>>>>> stdio_tmp
size_t fread(void *dest, size_t size, size_t nmemb, FILE *stream)
{
	if (size == 0 || nmemb == 0)
		return 0;

	if (SIZE_MAX / nmemb < size) {
		_set_errno(EINVAL);
		return 0;
	}

	size_t total = size * nmemb;

	flockfile(stream);
<<<<<<< HEAD

	/* If buffer contains written data, flush buffer to output first. */
	if (stream->buffer_state == _bs_write) {
		if (fflush(stream) == EOF) {
			funlockfile(stream);
			return 0;
		}
	}

	size_t read = _fread(stream, dest, total);

=======
	size_t read = _fread(stream, dest, total);
>>>>>>> stdio_tmp
	funlockfile(stream);
	return read / size;
}

static size_t _fwrite(FILE *stream, const void *src, size_t total)
{
	size_t written = 0;

	while (written < total) {
		if (_buffer_empty(stream) && total >= _buffer_size(stream)) {
			/* If data is too long to fit in buffer, write directly. */
			written += stream->ops->write(stream->stream_handle,
			    src + written, total - written, &stream->error);
		} else {
			if (_buffer_full(stream)) {
				/* Try to make some space in the buffer. */
<<<<<<< HEAD
				if (_partial_flush_buffer(stream) == 0) {
=======
				if (_try_flush_buffer(stream) == 0) {
>>>>>>> stdio_tmp
					stream->error = true;
					return written;
				}
			}
			written += _write_to_buffer(stream, src + written, total - written);
		}
	}

	return written;
}

<<<<<<< HEAD
static void _fputwc(FILE *stream, wchar_t wc)
{
	unimplemented();
}

=======
>>>>>>> stdio_tmp
size_t fwrite(const void *src, size_t size, size_t nmemb, FILE *stream)
{
	if (size == 0 || nmemb == 0)
		return 0;

	if (SIZE_MAX / nmemb < size) {
		_set_errno(EINVAL);
		return 0;
	}

	size_t total = size * nmemb;

	flockfile(stream);
<<<<<<< HEAD
=======

	if (!_lazy_alloc_buffer(stream)) {
		funlockfile(stream);
		_set_errno(ENOMEM);
		return 0;
	}

>>>>>>> stdio_tmp
	_flush_before_write(stream);
	size_t written = _fwrite(stream, src, total);
	_flush_after_write(stream);
	funlockfile(stream);
	return written / size;
}

<<<<<<< HEAD
struct specifier {
	bool flag_left_justified;
	bool flag_sign;
	bool flag_space_sign;
	bool flag_alternative;
	bool flag_zero_pad;
	int field_width;
	int precision;
	int length;
	int format;
};

struct input {
	void *p;
	int (*popchar)(void *);
	int (*current)(void *);
};

static int _current(struct input *in)
{
	return in->current(in->p);
}

static int _popchar(struct input *in)
{
	return in->popchar(in->p);
}

static bool _eat_char(struct input *in, int c)
{
	if (_current(in) != c)
		return false;

	_popchar(in);
	return true;
}

static int _eat_one(struct input *in, const char *cs)
{
	int c = _current(in);

	if (strchr(cs, c)) {
		_popchar(in);
		return c;
	}

	return 0;
}

static unsigned _read_number(struct input *in)
{
	unsigned number = 0;
	while (isdigit(_current(in))) {
		number *= 10;
		number += _popchar(in) - '0';
	}
	return number;
}

static void _parse_flags(struct specifier *spec, struct input *in)
{
	while (true) {
		switch (_eat_one(in, "-+ #0")) {
		case '-':
			spec->flag_left_justified = true;
			break;
		case '+':
			spec->flag_sign = true;
			break;
		case ' ':
			spec->flag_space_sign = true;
			break;
		case '#':
			spec->flag_alternative = true;
			break;
		case '0':
			spec->flag_zero_pad = true;
			break;
		default:
			return;
		}
	}
}

static void _parse_length(struct specifier *spec, struct input *in)
{
	switch (_eat_one(in, "hljztLw")) {
	case 'h':
		spec->length = _eat_char(in, 'h') ? sizeof(char) : sizeof(short);
		break;

	case 'l':
		spec->length = _eat_char(in, 'l') ? sizeof(long long) : sizeof(long);
		break;

	case 'j':
		spec->length = sizeof(intmax_t);
		break;

	case 'z':
		spec->length = sizeof(size_t);
		break;

	case 't':
		spec->length = sizeof(ptrdiff_t);
		break;

	case 'L':
		spec->length = sizeof(long double);
		break;

	case 'w':
		bool fast = _eat_char(in, 'f');

		switch (_read_number(in)) {
		case 8:
			spec->length = fast ? sizeof(int_fast8_t) : sizeof(int8_t);
			break;
		case 16:
			spec->length = fast ? sizeof(int_fast16_t) : sizeof(int16_t);
			break;
		case 32:
			spec->length = fast ? sizeof(int_fast32_t) : sizeof(int32_t);
			break;
		case 64:
			spec->length = fast ? sizeof(int_fast64_t) : sizeof(int64_t);
			break;
		}

		break;
	}
}

static struct specifier _parse_specifier(void *p, int (*popchar)(void *), int (*current)(void *), va_list arg)
{
	struct input in = {
		.p = p,
		.popchar = popchar,
		.current = current,
	};

	struct specifier spec = {
		.field_width = 0,
		.precision = -1,
		.length = 0,
	};

	_parse_flags(&spec, &in);

	/* Parse field width. */
	if (_eat_char(&in, '*')) {
		spec.field_width = va_arg(arg, int);
		if (spec.field_width < 0) {
			spec.flag_left_justified = true;
			spec.field_width = -spec.field_width;
		}
	} else {
		spec.field_width = _read_number(&in);
	}

	/* Parse precision. */
	if (_eat_char(&in, '.')) {
		if (_eat_char(&in, '*')) {
			spec.precision = va_arg(arg, int);
		} else {
			spec.precision = _read_number(&in);
		}
	}

	_parse_length(&spec, &in);

	spec.format = _popchar(&in);
	return spec;
}

static int64_t _get_signed(va_list arg, int length)
{
	switch (length) {
	case 0:
	case 1:
	case 2:
	case 4:
		return va_arg(arg, int);
	case 8:
		return va_arg(arg, int64_t);
	}

	return 0x1BADFEED;
}

static uint64_t _get_unsigned(va_list arg, int length)
{
	switch (length) {
	case 0:
	case 1:
	case 2:
	case 4:
		return va_arg(arg, unsigned);
	case 8:
		return va_arg(arg, uint64_t);
	}

	return 0x1BADFEED;
}

static void _reverse_buffer(char *buffer, int len)
{
	for (int i = 0; i < len / 2; i++) {
		char tmp = buffer[i];
		buffer[i] = buffer[len - i - 1];
		buffer[len - i - 1] = tmp;
	}
}

static int _convert_decimal(char *buffer, uint64_t val)
{
	int len = 0;

	/* Convert to base 10, sans sign. */
	while (val > 0) {
		buffer[len++] = '0' + val % 10;
		val /= 10;
	}

	_reverse_buffer(buffer, len);
	return len;
}

static const char _zeroes[] = "000000000000000000000000000000000000000000000000";
static const char _spaces[] = "                                                ";

static const char _digits_lowercase[] = "0123456789abcdefghijklmnopqrstuvwxyz";
static const char _digits_uppercase[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

#define _zeroes_len() (sizeof(_zeroes) - 1)
#define _spaces_len() (sizeof(_spaces) - 1)

static int _print_sign(FILE *stream, struct specifier *spec, bool negative)
{
	if (negative) {
		return _fwrite(stream, "-", 1);
	} else if (spec->flag_sign) {
		return _fwrite(stream, "+", 1);
	} else if (spec->flag_space_sign) {
		return _fwrite(stream, " ", 1);
	}

	return 0;
}

static int _print_padding(FILE *stream, int len, const char *padding, int padding_len)
{
	int n = 0;

	for (int i = len; i > 0; i -= padding_len) {
		n += _fwrite(stream, padding, min(padding_len, i));
	}

	return n;
}

static int _print_decimal(FILE *stream, struct specifier *spec, uint64_t val, bool negative)
{
	/* 20 decimal digits is needed for 64b numbers, but toss in extras in case I miscounted. */
	char buffer[22] = { };
	int len = _convert_decimal(buffer, val);

	/* Zero padding is ignored if left justification and/or precision is also specified. */
	bool zero_padded = spec->flag_zero_pad && !spec->flag_left_justified && spec->precision < 0;

	if (spec->precision < 0)
		spec->precision = 1;

	int sign_len = negative || spec->flag_sign || spec->flag_space_sign;
	int zeroext_len = (spec->precision < len) ? 0 : (spec->precision - len);
	int pad_len = spec->field_width - sign_len - len - zeroext_len;

	int n = 0;

	/* Print the various parts of the format in correct order. */

	/* Space padding for right justified output. */
	if (!zero_padded && !spec->flag_left_justified)
		n += _print_padding(stream, pad_len, _spaces, _spaces_len());

	/* Le sign. */
	n += _print_sign(stream, spec, negative);

	/*
	 * Zero padding for '0' flag.
	 * In contrast to space padding, must be after the sign.
	 */
	if (zero_padded)
		n += _print_padding(stream, pad_len, _zeroes, _zeroes_len());

	/* Zero extension from precision. */
	n += _print_padding(stream, zeroext_len, _zeroes, _zeroes_len());

	/* The actual number. */
	n += _fwrite(stream, buffer, len);

	/* Space padding for left justified output. */
	if (spec->flag_left_justified)
		n += _print_padding(stream, pad_len, _spaces, _spaces_len());

	return n;
}

static int _print_str(FILE *stream, struct specifier *spec, const char *str)
{
	unimplemented();
}

static int _print_wstr(FILE *stream, struct specifier *spec, const wchar_t *str)
{
	unimplemented();
}

static int _wprint_str(FILE *stream, struct specifier *spec, const char *str)
{
	unimplemented();
}

static int _wprint_wstr(FILE *stream, struct specifier *spec, const wchar_t *str)
{
	unimplemented();
}

static int _print_bin(FILE *stream, struct specifier *spec, uint64_t val, int bits_per_digit, int mark, const char *digits)
{
	unimplemented();
}

/*
 * Numeric output is unaffected by differences between byte-oriented
 * and wide-oriented output.
 */
static int _print_numeric(FILE *stream, struct specifier *spec, va_list arg)
{
	switch (spec->format) {
	case 'd':
	case 'i':
		int64_t val = _get_signed(arg, spec->length);
		if (val >= 0) {
			return _print_decimal(stream, spec, val, false);
		} else {
			/* Directly negating INT64_MIN is undefined behavior, so cast
			 * first, negate second. Two's complement representation ensures
			 * this works.
			 */
			return _print_decimal(stream, spec, -(uint64_t) val, true);
		}
	case 'u':
		return _print_decimal(stream, spec, _get_unsigned(arg, spec->length), false);
	case 'b':
		return _print_bin(stream, spec, _get_unsigned(arg, spec->length), 1, 'b', _digits_lowercase);
	case 'B':
		return _print_bin(stream, spec, _get_unsigned(arg, spec->length), 1, 'B', _digits_lowercase);
	case 'o':
		return _print_bin(stream, spec, _get_unsigned(arg, spec->length), 3, '0', _digits_lowercase);
	case 'x':
		return _print_bin(stream, spec, _get_unsigned(arg, spec->length), 4, 'x', _digits_lowercase);
	case 'X':
		return _print_bin(stream, spec, _get_unsigned(arg, spec->length), 4, 'X', _digits_uppercase);
	case 'p':
		// TODO
		unimplemented();
		return 0;
	}

	return 0;
}

static int _print_float(FILE *stream, struct specifier *spec, va_list arg)
{
	unimplemented();
}

static int _print_char(FILE *stream, struct specifier *spec, int c)
{
	unimplemented();
}

static int _print_wchar(FILE *stream, struct specifier *spec, wchar_t c)
{
	unimplemented();
}

static int _wprint_char(FILE *stream, struct specifier *spec, int c)
{
	unimplemented();
}

static int _wprint_wchar(FILE *stream, struct specifier *spec, wchar_t c)
{
	unimplemented();
}

static void _store_n(int n, struct specifier *spec, va_list arg)
{
	int len = spec->length;
	if (len == 0)
		len = sizeof(int);

	switch (len) {
	case 1:
		*va_arg(arg, int8_t *) = n;
		break;
	case 2:
		*va_arg(arg, int16_t *) = n;
		break;
	case 4:
		*va_arg(arg, int32_t *) = n;
		break;
	case 8:
		*va_arg(arg, int64_t *) = n;
		break;
	}
}

static int _current_byte(void *p)
{
	const char **s = p;
	return **s;
}

static int _popchar_byte(void *p)
{
	const char **s = p;
	return *((*s)++);
}

static int _vfprintf(FILE *stream, const char *format, va_list arg)
{
	size_t n = 0;

	/* Iterate until format is empty. */
	while (true) {
		/* Find the first format specifier. */
		const char *p = strchr(format, '%');
		if (p == NULL) {
			/* No percent sign, print the rest. */
			n += _fwrite(stream, format, strlen(format));
			return n;
		}

		/* Print everything before the specifier. */
		n += _fwrite(stream, format, p - format);
		format = p + 1;

		struct specifier spec = _parse_specifier(&format, _popchar_byte, _current_byte, arg);

		switch (spec.format) {
		case 'n':
			_store_n(n, &spec, arg);
			break;
		case 'd':
		case 'i':
		case 'u':
		case 'b':
		case 'B':
		case 'o':
		case 'x':
		case 'X':
		case 'p':
			n += _print_numeric(stream, &spec, arg);
			break;
		case 'f':
		case 'F':
		case 'g':
		case 'G':
		case 'e':
		case 'E':
		case 'a':
		case 'A':
			n += _print_float(stream, &spec, arg);
			break;
		case '%':
			n += _print_char(stream, &spec, '%');
			break;
		case 'c':
			if (spec.length == sizeof(long))
				n += _print_wchar(stream, &spec, va_arg(arg, wint_t));
			else
				n += _print_char(stream, &spec, va_arg(arg, int));
			break;
		case 's':
			if (spec.length == sizeof(long))
				n += _print_wstr(stream, &spec, va_arg(arg, const wchar_t *));
			else
				n += _print_str(stream, &spec, va_arg(arg, const char *));
			break;
		}
	}

}

static int _popchar_wchar(void *p)
{
	const wchar_t **ws = p;
	return *((*ws)++);
}

static int _current_wchar(void *p)
{
	const wchar_t **ws = p;
	return **ws;
}

static int _vfwprintf(FILE *stream, const wchar_t *ws, va_list arg)
{
	int n = 0;
	wchar_t c;

	while ((c = *(ws++))) {
		if (c != L'%') {
			_fputwc(stream, c);
			n++;
		}

		struct specifier spec = _parse_specifier(&ws, _popchar_wchar, _current_wchar, arg);

		switch (spec.format) {
		case 'n':
			_store_n(n, &spec, arg);
			break;
		case 'd':
		case 'i':
		case 'u':
		case 'b':
		case 'B':
		case 'o':
		case 'x':
		case 'X':
		case 'p':
			/* When printing numbers, bytes and characters are the same thing. */
			n += _print_numeric(stream, &spec, arg);
			break;
		case 'f':
		case 'F':
		case 'g':
		case 'G':
		case 'e':
		case 'E':
		case 'a':
		case 'A':
			n += _print_float(stream, &spec, arg);
			break;
		case '%':
			n += _wprint_char(stream, &spec, '%');
			break;
		case 'c':
			if (spec.length == sizeof(long))
				n += _wprint_wchar(stream, &spec, va_arg(arg, wint_t));
			else
				n += _wprint_char(stream, &spec, va_arg(arg, int));
			break;
		case 's':
			if (spec.length == sizeof(long))
				n += _wprint_wstr(stream, &spec, va_arg(arg, const wchar_t *));
			else
				n += _wprint_str(stream, &spec, va_arg(arg, const char *));
			break;
		}
	}

	return n;
}

int vfwprintf(FILE *stream, const wchar_t *ws, va_list arg)
{
	flockfile(stream);
	_flush_before_write(stream);
	int ret = _vfwprintf(stream, ws, arg);
	_flush_after_write(stream);
	funlockfile(stream);
	return ret;
}

int vfprintf(FILE *stream, const char *s, va_list arg)
{
	flockfile(stream);
	_flush_before_write(stream);
	int ret = _vfprintf(stream, s, arg);
	_flush_after_write(stream);
	funlockfile(stream);
	return ret;
}

=======
void setbuf(FILE *stream, char *buf)
{
	if (buf == NULL) {
		(void) setvbuf(stream, NULL, _IONBF, 0);
	} else {
		(void) setvbuf(stream, buf, _IOFBF, BUFSIZ);
	}
}

int setvbuf(FILE *stream, char *buf, int mode, size_t size)
{
	if (mode != _IOFBF && mode != _IOLBF && mode != _IONBF) {
		_set_errno(EINVAL);
		return -1;
	}

	bool allocated_buffer = (buf == NULL) && (size > 0);

	if (allocated_buffer) {
		buf = malloc(size);
		if (!buf) {
			_set_errno(ENOMEM);
			return -1;
		}
	}

	/*
	 * We allow _IONBF with non-zero length buffer.
	 * The buffer will be used to collect data, but flushed at the end of each
	 * user-level write (e.g. a printf() might only write to the backend
	 * once, even when it's made of many individual format specifiers, but it
	 * will leave the buffer empty afterwards).
	 * Reads use buffer the same regardless of mode.
	 */
	stream->btype = mode;

	if (stream->allocated_buffer)
		free(stream->buffer);

	uint8_t *b = (uint8_t *) buf;

	stream->buffer = b;
	stream->buffer_end = b + size;
	stream->buffer_head = b;
	stream->buffer_tail = b;
	stream->allocated_buffer = allocated_buffer;
	return 0;
}
>>>>>>> stdio_tmp
