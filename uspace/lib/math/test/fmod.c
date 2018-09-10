
#include <pcut/pcut.h>
#include <math.h>
#include <inttypes.h>

PCUT_INIT;

PCUT_TEST_SUITE(fmod);

#include <math.h>

#include "testables.h"

PCUT_TEST(_frexpi64)
{
	// TODO: more tests
	double pi2 = 1.57079632679489661923132169163975144209858469968755;

	int e;
	uint64_t i = __testable_frexpi64(pi2, &e);

	PCUT_ASSERT_UINT64_EQUALS(0x1921fb54442d18, i);
	PCUT_ASSERT_INT_EQUALS(-52, e);
}

PCUT_TEST(_ldexpi64)
{
	// TODO: more tests
	double pi2 = 1.57079632679489661923132169163975144209858469968755;
	double f = __testable_ldexpi64(0x1921fb54442d18, -52);
	PCUT_ASSERT_DOUBLE_IDENTICAL(pi2, f);

	uint64_t max = UINT64_C(0x1fffffffffffff);
	int emax = 1023 - 52;
	double max_double = 0x1.fffffffffffffp1023;
	PCUT_ASSERT_DOUBLE_IDENTICAL(max_double, __testable_ldexpi64(max, emax));

	PCUT_ASSERT_DOUBLE_IDENTICAL(1.0, __testable_ldexpi64(1, 0));
	PCUT_ASSERT_DOUBLE_IDENTICAL(1.0, __testable_ldexpi64(16, -4));
	PCUT_ASSERT_DOUBLE_IDENTICAL(16.0, __testable_ldexpi64(1, 4));
}

PCUT_TEST(_fmod_split)
{
	uint64_t result;
	uint64_t result2;
	int eresult;
	int eresult2;
	__testable_fmod_split(1, 0, 1, 0, &result, &eresult);
	PCUT_ASSERT_UINT64_EQUALS(0, result);

	__testable_fmod_split(1, 10, 1, 5, &result, &eresult);
	PCUT_ASSERT_UINT64_EQUALS(0, result);

	uint64_t max = UINT64_C(0x1fffffffffffff);
	int emax = /*1023*/52 - 52;

	/* lower bound for PI/2 in double (53-bit) precision */
	uint64_t arg1 = UINT64_C(0x1921fb54442d18);
	int earg1 = -52;

	/* upper bound for PI/2 in double (53-bit) precision */
	uint64_t arg2 = UINT64_C(0x1921fb54442d19);
	int earg2 = -52;

	/* lower bound for PI/2 in 63-bit precision */
	uint64_t arg3 = UINT64_C(0x6487ed5110b4611a);
	int earg3 = -62;

	/* upper bound for PI/2 in 63-bit precision */
	uint64_t arg4 = UINT64_C(0x6487ed5110b4611b);
	int earg4 = -62;

	double f1 = __testable_ldexpi64(arg1, earg1);
	double f2 = __testable_ldexpi64(arg3, earg3);
	double f3 = __testable_ldexpi64(arg4, earg4);
	PCUT_ASSERT_DOUBLE_IDENTICAL(f1, f2);
	PCUT_ASSERT_DOUBLE_IDENTICAL(f1, f3);

	__testable_fmod_split(max, emax, arg1, earg1, &result, &eresult);
	PCUT_ASSERT_UINT64_EQUALS(0xbae9ea49cb3a, result);
	PCUT_ASSERT_INT_EQUALS(-49, eresult);

	__testable_fmod_split(max, emax, arg2, earg2, &result, &eresult);
	PCUT_ASSERT_UINT64_EQUALS(0xa9a1a38c8be67, result);
	PCUT_ASSERT_INT_EQUALS(-52, eresult);

	__testable_fmod_split(max, emax, arg3, earg3, &result, &eresult);
	//PCUT_ASSERT_UINT64_EQUALS(0x7630f824903066, result);
	//PCUT_ASSERT_INT_EQUALS(-49, eresult);

	__testable_fmod_split(max, emax, arg4, earg4, &result2, &eresult2);
	PCUT_ASSERT_UINT64_EQUALS(result2, result);
	PCUT_ASSERT_INT_EQUALS(eresult2, eresult);
}

PCUT_TEST(fmod)
{
	//double max_double = 0x1.fffffffffffffp1023;

}

PCUT_EXPORT(fmod);
