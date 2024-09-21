
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

#define BUFSIZ  4096

#ifndef SEEK_SET
#define SEEK_SET  0
#endif

#ifndef SEEK_CUR
#define SEEK_CUR  1
#endif

#ifndef SEEK_END
#define SEEK_END  2
#endif

void flockfile(FILE *);
int ftrylockfile(FILE *);
void funlockfile(FILE *);
int fflush(FILE *);
size_t fread(void *dest, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(const void *src, size_t size, size_t nmemb, FILE *stream);

int fgetc(FILE *stream);
char *fgets(char *s, int n, FILE *stream);

int vfwprintf(FILE *stream, const wchar_t *ws, va_list arg);
int vfprintf(FILE *stream, const char *s, va_list arg);

void setbuf(FILE *stream, char *buf);
int setvbuf(FILE *stream, char *buf, int mode, size_t size);

#endif /* COMMON_STDIO_CORE_H_ */
