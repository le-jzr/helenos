
#ifndef COMMON_INCLUDE_STRING_H_
#define COMMON_INCLUDE_STRING_H_

// TODO: generic
extern void *strchr(const void *s, int c);
extern void *memchr(const void *s, int c, size_t n);
extern void *memrchr(const void *s, int c, size_t n);

extern void *memcpy(void *restrict s1, const void *restrict s2, size_t n);
extern size_t strlen(const char *s);

#endif /* COMMON_INCLUDE_STRING_H_ */
