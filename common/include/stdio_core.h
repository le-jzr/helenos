
#ifndef COMMON_STDIO_CORE_H_
#define COMMON_STDIO_CORE_H_

#include <stdarg.h>
#include <stddef.h>
#include <errno.h>

typedef struct _IO_FILE FILE;

enum __buffer_type {
	/** No buffering */
	_IONBF,
	/** Line buffering */
	_IOLBF,
	/** Full buffering */
	_IOFBF
};

#define EOF (-1)

void flockfile(FILE *);
int ftrylockfile(FILE *);
void funlockfile(FILE *);
int fflush(FILE *);
size_t fread(void *dest, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(const void *src, size_t size, size_t nmemb, FILE *stream);


int vfwprintf(FILE *stream, const wchar_t *ws, va_list arg);
int vfprintf(FILE *stream, const char *s, va_list arg);

#endif /* COMMON_STDIO_CORE_H_ */
