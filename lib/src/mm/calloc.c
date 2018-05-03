
#include <c.h>
#include <stdint.h>

void *calloc(size_t nmemb, size_t size)
{
	if (SIZE_MAX / 2 / nmemb < size)
		return NULL;

	void *buf = malloc(nmemb * size);
	if (buf == NULL)
		return NULL;

	memset(buf, 0, nmemb * size);

	return buf;
}
