/*
 * General floating point GCD routine and associated code for Mathomatic.
 *
 * Copyright (C) 1987-2010 George Gesslein II.
 
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public 
    License as published by the Free Software Foundation; either 
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of 
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.
 
    You should have received a copy of the GNU Lesser General Public 
    License along with this library; if not, write to the Free Software 
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 
 */

#include "includes.h"

/*
 * Return the Greatest Common Divisor (GCD) of doubles d1 and d2,
 * by using the Euclidean GCD algorithm.
 *
 * The GCD is defined as the largest positive number that evenly divides both d1 and d2.
 * This should always works perfectly and exactly with two integers up to MAX_K_INTEGER.
 * Will usually work with non-integers, but there may be some floating point error.
 *
 * Returns 0 on failure, otherwise returns the positive GCD.
 */
double
gcd(d1, d2)
double	d1, d2;
{
	int	count;
	double	larger, divisor, remainder1, lower_limit;

	if (!isfinite(d1) || !isfinite(d2)) {
		return 0.0;	/* operands must be finite */
	}
	d1 = fabs(d1);
	d2 = fabs(d2);
#if	1	/* true for standard gcd(), otherwise returns 0 (failure) if either parameter is 0 */
	if (d1 == 0)
		return d2;
	if (d2 == 0)
		return d1;
#endif
	if (d1 > d2) {
		larger = d1;
		divisor = d2;
	} else {
		larger = d2;
		divisor = d1;
	}
	lower_limit = larger * epsilon;
	if (divisor <= lower_limit || larger >= MAX_K_INTEGER) {
		return 0.0;	/* out of range, result would be too inaccurate */
	}
	for (count = 1; count < 50; count++) {
		remainder1 = fabs(fmod(larger, divisor));
		if (remainder1 <= lower_limit || fabs(divisor - remainder1) <= lower_limit) {
			if (remainder1 != 0.0 && divisor <= (100.0 * lower_limit))
				return 0.0;
			return divisor;
		}
		larger = divisor;
		divisor = remainder1;
	}
	return 0.0;
}

/*
 * Return the verified exact Greatest Common Divisor (GCD) of doubles d1 and d2.
 *
 * Returns 0 on failure or inexactness, otherwise returns the verified positive GCD result.
 * Result is not necessarily integer unless both d1 and d2 are integer.
 */
double
gcd_verified(d1, d2)
double	d1, d2;
{
	double	divisor, d3, d4;

	divisor = gcd(d1, d2);
	if (divisor != 0.0) {
		d3 = d1 / divisor;
		d4 = d2 / divisor;
		if (fmod(d3, 1.0) != 0.0 || fmod(d4, 1.0) != 0.0)
			return 0.0;
		if (gcd(d3, d4) != 1.0)
			return 0.0;
	}
	return divisor;
}

/*
 * Return a floating point double rounded to the nearest integer.
 */
double
my_round(d1)
double	d1;	/* value to round */
{
	if (d1 >= 0.0) {
		modf(d1 + 0.5, &d1);
	} else {
		modf(d1 - 0.5, &d1);
	}
	return d1;
}

/*
 * Convert the passed double d to an equivalent fully reduced fraction.
 * This done by the following simple algorithm:
 *
 * divisor = gcd(d, 1.0)
 * numerator = d / divisor
 * denominator = 1.0 / divisor
 *
 * Return true with integers in numerator and denominator
 * if conversion to a fraction was successful.
 * Otherwise return false with numerator = d and denominator = 1.0
 *
 * True return indicates d is rational and finite, otherwise d is probably irrational.
 */
int
f_to_fraction(d, numeratorp, denominatorp)
double	d;		/* floating point number to convert */
double	*numeratorp;	/* returned numerator */
double	*denominatorp;	/* returned denominator */
{
	double	divisor;
	double	numerator, denominator;
	double	k3, k4;

	*numeratorp = d;
	*denominatorp = 1.0;
	if (!isfinite(d)) {
		return false;
	}
/* see if "d" is an integer, or very close to an integer: */
	if (fmod(d, 1.0) == 0.0) {
		return true;
	}
	k3 = fabs(d) * small_epsilon;
	k4 = my_round(d);
	if (k4 != 0.0 && fabs(k4 - d) <= k3) {
		*numeratorp = k4;
		return true;
	}
/* try to convert non-integer floating point value in "d" to a fraction: */
	if ((divisor = gcd(1.0, d)) > epsilon) {
		numerator = my_round(d / divisor);
		denominator = my_round(1.0 / divisor);
/* don't allow more than 11 digits in the numerator or denominator: */
		if (fabs(numerator) >= 1.0e12)
			return false;
		if (denominator >= 1.0e12 || denominator < 2.0)
			return false;
/* make sure the result is a fully reduced fraction: */
		divisor = gcd(numerator, denominator);
		if (divisor > 1.0) {	/* shouldn't happen, but just in case */
			numerator /= divisor;
			denominator /= divisor;
		}
		k3 = (numerator / denominator);
		if (fabs(k3 - d) > (small_epsilon * fabs(k3))) {
			return false;	/* result is too inaccurate */
		}
/* numerator and denominator are integral */
		*numeratorp = numerator;
		*denominatorp = denominator;
		return true;
	}
	return false;
}

/*
 * Convert non-integer constants in an equation side to fractions,
 * when exactly equal to simple fractions.
 * Uses f_to_fraction() above.
 *
 * Return true if any fractions were created.
 */
int
make_fractions(equation, np)
token_type	*equation;	/* equation side pointer */
int		*np;		/* pointer to length of equation side */
{
	int	i, j, k;
	int	level;
	double	numerator, denominator;
	int	inc_level, modified = false;

	for (i = 0; i < *np; i += 2) {
		if (equation[i].kind == CONSTANT) {
			level = equation[i].level;
			if (i > 0 && equation[i-1].level == level && equation[i-1].token.operatr == DIVIDE)
				continue;
			if (!f_to_fraction(equation[i].token.constant, &numerator, &denominator))
				continue;
			if (denominator == 1.0) {
				equation[i].token.constant = numerator;
				continue;
			}
			if ((*np + 2) > n_tokens) {
				error_huge();
			}
			modified = true;
			inc_level = (*np > 1);
			if ((i + 1) < *np && equation[i+1].level == level) {
				switch (equation[i+1].token.operatr) {
				case TIMES:
					for (j = i + 3; j < *np && equation[j].level >= level; j += 2) {
						if (equation[j].level == level && equation[j].token.operatr == DIVIDE) {
							break;
						}
					}
					if (numerator == 1.0) {
						blt(&equation[i], &equation[i+2], (j - (i + 2)) * sizeof(token_type));
						j -= 2;
					} else {
						equation[i].token.constant = numerator;
						blt(&equation[j+2], &equation[j], (*np - j) * sizeof(token_type));
						*np += 2;
					}
					equation[j].level = level;
					equation[j].kind = OPERATOR;
					equation[j].token.operatr = DIVIDE;
					j++;
					equation[j].level = level;
					equation[j].kind = CONSTANT;
					equation[j].token.constant = denominator;
					if (numerator == 1.0) {
						i -= 2;
					}
					continue;
				case DIVIDE:
					inc_level = false;
					break;
				}
			}
			j = i;
			blt(&equation[i+3], &equation[i+1], (*np - (i + 1)) * sizeof(token_type));
			*np += 2;
			equation[j].token.constant = numerator;
			j++;
			equation[j].level = level;
			equation[j].kind = OPERATOR;
			equation[j].token.operatr = DIVIDE;
			j++;
			equation[j].level = level;
			equation[j].kind = CONSTANT;
			equation[j].token.constant = denominator;
			if (inc_level) {
				for (k = i; k <= j; k++)
					equation[k].level++;
			}
		}
	}
	return modified;
}
