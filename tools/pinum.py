#!/usr/bin/env python3

from fractions import Fraction

base = Fraction(2^2001, 1)

last_lbound = Fraction(0, 1)
last_ubound = Fraction(0, 1)

k = 0
q = Fraction(1, 1)
quarter = Fraction(1, 4)
half = Fraction(1, 2)

# Scale up until the div is equal.

while True:
	print (float(last_lbound))
	print (float(last_ubound))
	last_ubound = last_lbound + half * q * (Fraction(1, 1 + 2*k) + Fraction(2, 1 + 4*k) + Fraction(1, 3 + 4*k))
	q *= quarter
	k += 1
	last_lbound = last_ubound - half * q * (Fraction(1, 1 + 2*k) + Fraction(2, 1 + 4*k) + Fraction(1, 3 + 4*k))
	q *= quarter
	k += 1
	print((base / last_lbound).__floor__() - (base / last_ubound).__floor__())
	if (base / last_lbound).__floor__() == (base / last_ubound).__floor__():
		break

div = (base / last_lbound).__floor__()

while True:
	print (float(last_lbound))
	print (float(last_ubound))
	last_ubound = last_lbound + half * q * (Fraction(1, 1 + 2*k) + Fraction(2, 1 + 4*k) + Fraction(1, 3 + 4*k))
	q *= quarter
	k += 1
	last_lbound = last_ubound - half * q * (Fraction(1, 1 + 2*k) + Fraction(2, 1 + 4*k) + Fraction(1, 3 + 4*k))
	q *= quarter
	k += 1

	rem1 = base - div * last_lbound
	rem2 = base - div * last_ubound
	print(float(rem1))
	print(float(rem2))
	if (float(rem1) == float(rem2)):
		break

(x +- ex) * (y +- ey) = x * y +- x*ey +- y * ex +- ex * ey



