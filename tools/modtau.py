#!/usr/bin/env python3

from fractions import Fraction

# Using the following series to compute Tau to the required number of bits:
# Tau = 2 * sum(k=0..inf) (-1/4)^k (1/(1 + 2 k) + 2/(1 + 4 k) + 1/(3 + 4 k))
#
# Thanks to the alternating sign, we alternately get a lower bound for Tau
# and an upper bound for Tau. We use this to determine when to stop iterating.
#

last_lbound = Fraction(218, 35)
last_ubound = Fraction(20, 3)
k = 2
q = Fraction(1, 16)
quarter = Fraction(1, 4)
two = Fraction(2, 1)

# Scale up until we know the correct integral multiplier.

dividend = Fraction(1, 1)
i = 0

while i < 1024:
    #print("dividend:", dividend)
    while ((dividend / last_lbound).__floor__() != (dividend / last_ubound).__floor__()):
        last_ubound = last_lbound + two * q * (Fraction(1, 1 + 2*k) + Fraction(2, 1 + 4*k) + Fraction(1, 3 + 4*k))
        q *= quarter
        k += 1
        last_lbound = last_ubound - two * q * (Fraction(1, 1 + 2*k) + Fraction(2, 1 + 4*k) + Fraction(1, 3 + 4*k))
        q *= quarter
        k += 1
        #print(float(last_lbound), "< Tau <", float(last_ubound))
        #print((dividend / last_lbound).__floor__() - (dividend / last_ubound).__floor__())

    mult = (dividend / last_lbound).__floor__()

    # mult * Tau <= dividend < (mult + 1) * Tau

    #print("Multiplier is", mult)

    # Iterate further, this time until we know enough of the remainder for
    # the closest possible float.

    while True:
        rem1 = dividend - mult * last_lbound
        rem2 = dividend - mult * last_ubound
        #print(float(rem1), "< rem <", float(rem2))
        if (float(rem1) == float(rem2)):
            break

        last_ubound = last_lbound + two * q * (Fraction(1, 1 + 2*k) + Fraction(2, 1 + 4*k) + Fraction(1, 3 + 4*k))
        q *= quarter
        k += 1
        last_lbound = last_ubound - two * q * (Fraction(1, 1 + 2*k) + Fraction(2, 1 + 4*k) + Fraction(1, 3 + 4*k))
        q *= quarter
        k += 1
        #print(float(last_lbound), "< Tau <", float(last_ubound))

    print(float(rem1).hex(), ", /* 2 ^", i, "mod Tau =", float(rem1), "*/")

    i += 1
    dividend *= 2