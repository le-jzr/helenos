
#include <wchar.h>
#include <uchar.h>

wint_t btowc(int c)
{
	return (c < 0x80) ? c : WEOF;
}

int wctob(wint_t c)
{
	return c;
}

int mbsinit(const mbstate_t *ps)
{
	return ps == NULL || ps->continuation == 0;
}

size_t mbrlen(const char *s, size_t n, mbstate_t *ps)
{
	static mbstate_t global_state;
	return mbrtowc(NULL, s, n, ps != NULL ? ps: &global_state);
}

_Static_assert(sizeof(wchar_t) == sizeof(char16_t) || sizeof(wchar_t) == sizeof(char32_t));

size_t mbrtowc(wchar_t *pwc, const char *s, size_t n, mbstate_t *ps)
{
	static mbstate_t global_state;
	if (!ps)
		ps = &global_state;

	if (sizeof(wchar_t) == sizeof(char16_t))
		return mbrtoc16((char16_t *) pwc, s, n, ps);
	else
		return mbrtoc32((char32_t *) pwc, s, n, ps);
}

size_t wcrtomb(char *s, wchar_t wc, mbstate_t * ps)
{
	static mbstate_t global_state;
	if (!ps)
		ps = &global_state;

	if (sizeof(wchar_t) == sizeof(char16_t))
		return c16rtomb(s, (char16_t) wc, ps);
	else
		return c32rtomb(s, (char32_t) wc, ps);
}
