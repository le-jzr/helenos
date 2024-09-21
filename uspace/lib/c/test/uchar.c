
#include <pcut/pcut.h>
#include <uchar.h>
#include <mem.h>
#include <stdio.h>

PCUT_INIT;

PCUT_TEST_SUITE(uchar);

PCUT_TEST(mbrtoc16)
{
	mbstate_t mbstate;
	memset(&mbstate, 0, sizeof(mbstate));

	size_t ret;
	const char *s = u8"a𐎣";
	char16_t out[] = u"AAAAA";
	const char16_t expected[] = u"a𐎣";

	ret = mbrtoc16(&out[0], s, MB_CUR_MAX, &mbstate);
	PCUT_ASSERT_INT_EQUALS(1, ret);
	PCUT_ASSERT_INT_EQUALS(expected[0], out[0]);

	s += ret;

	ret = mbrtoc16(&out[1], s, MB_CUR_MAX, &mbstate);
	PCUT_ASSERT_INT_EQUALS(4, ret);
	PCUT_ASSERT_INT_EQUALS(expected[1], out[1]);

	s += ret;

	ret = mbrtoc16(&out[2], s, MB_CUR_MAX, &mbstate);
	PCUT_ASSERT_INT_EQUALS(-3, ret);
	PCUT_ASSERT_INT_EQUALS(expected[2], out[2]);

	ret = mbrtoc16(&out[3], s, MB_CUR_MAX, &mbstate);
	PCUT_ASSERT_INT_EQUALS(0, ret);
	PCUT_ASSERT_INT_EQUALS(expected[3], out[3]);
}

PCUT_TEST(c16rtomb)
{
	mbstate_t mbstate;
	memset(&mbstate, 0, sizeof(mbstate));

	const char16_t in[] = u"aβℷ𐎣";
	char out[] = "AAAAAAAAAAAAAAAAA";

	char *s = out;

	for (size_t i = 0; i < sizeof(in) / sizeof(char16_t); i++) {
		s += c16rtomb(s, in[i], &mbstate);
	}

	const char expected[] = u8"aβℷ𐎣";
	PCUT_ASSERT_STR_EQUALS(expected, out);
	PCUT_ASSERT_INT_EQUALS(s - out, sizeof(expected));
}

PCUT_TEST(mbrtoc32)
{
	mbstate_t mbstate;
	memset(&mbstate, 0, sizeof(mbstate));

	size_t ret;
	char32_t c = 0;
	const char *s = u8"aβℷ𐎣";

	ret = mbrtoc32(&c, s, MB_CUR_MAX, &mbstate);
	PCUT_ASSERT_INT_EQUALS(ret, 1);
	PCUT_ASSERT_INT_EQUALS(c, U'a');

	s += ret;

	ret = mbrtoc32(&c, s, 1, &mbstate);
	PCUT_ASSERT_INT_EQUALS(ret, -2);
	PCUT_ASSERT_INT_EQUALS(c, U'a');

	s += 1;

	ret = mbrtoc32(&c, s, MB_CUR_MAX, &mbstate);
	PCUT_ASSERT_INT_EQUALS(ret, 1);
	PCUT_ASSERT_INT_EQUALS(c, U'β');

	s += ret;

	ret = mbrtoc32(&c, s, MB_CUR_MAX, &mbstate);
	PCUT_ASSERT_INT_EQUALS(ret, 3);
	PCUT_ASSERT_INT_EQUALS(c, U'ℷ');

	s += ret;

	ret = mbrtoc32(&c, s, 3, &mbstate);
	PCUT_ASSERT_INT_EQUALS(ret, -2);
	PCUT_ASSERT_INT_EQUALS(c, U'ℷ');

	s += 3;

	ret = mbrtoc32(&c, s, MB_CUR_MAX, &mbstate);
	PCUT_ASSERT_INT_EQUALS(ret, 1);
	PCUT_ASSERT_INT_EQUALS(c, U'𐎣');

	s += ret;

	ret = mbrtoc32(&c, s, MB_CUR_MAX, &mbstate);
	PCUT_ASSERT_INT_EQUALS(ret, 0);
	PCUT_ASSERT_INT_EQUALS(c, 0);
}

PCUT_TEST(c32rtomb)
{
	mbstate_t mbstate;
	memset(&mbstate, 0, sizeof(mbstate));

	char out[] = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
	char *s = out;

	_Static_assert(sizeof(out) > 5 * MB_CUR_MAX);

	s += c32rtomb(s, U'a', &mbstate);
	PCUT_ASSERT_INT_EQUALS(s - out, 1);

	s += c32rtomb(s, U'β', &mbstate);
	PCUT_ASSERT_INT_EQUALS(s - out, 3);

	s += c32rtomb(s, U'ℷ', &mbstate);
	PCUT_ASSERT_INT_EQUALS(s - out, 6);

	s += c32rtomb(s, U'𐎣', &mbstate);
	PCUT_ASSERT_INT_EQUALS(s - out, 10);

	s += c32rtomb(s, 0, &mbstate);
	PCUT_ASSERT_INT_EQUALS(s - out, 11);

	const char expected[] = u8"aβℷ𐎣";
	PCUT_ASSERT_INT_EQUALS(memcmp(out, expected, sizeof(expected)), 0);
}

PCUT_EXPORT(uchar);
