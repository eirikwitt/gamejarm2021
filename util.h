#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#define MIN(a, b) ((a)<(b)?(a):(b))
#define MAX(a, b) ((a)>(b)?(a):(b))
#define lenof(a) (sizeof(a)/sizeof(*(a)))
#define endof(a) (a + lenof(a))

typedef struct { short x, y; } Vec;

static const Vec vec_zero = {0, 0};

/* returns integer square root of x, capped at 2^bits
 * fast, gcc tends to unroll with constant bits at -O3
 */
static inline unsigned short isqrt(unsigned long x, unsigned bits) {
	unsigned y = 0;

	while (bits-- > 0) {
		unsigned a = (y + (1 << bits)) << bits;

		if (x >= a) {
			x -= a;
			y |= 2 << bits;
		}
	}
	return y >> 1;
}

/* returns the norm (length) squared of vector v
 * this is faster than vec_norm because it avoids sqrt
 */
static inline unsigned long vec_normsq(Vec v) {
	return (unsigned long)v.x * v.x + (unsigned long)v.y * v.y;
}

/* returns the norm (length) of vector v */
static inline unsigned short vec_norm(Vec v) {
	return isqrt(vec_normsq(v), CHAR_BIT*sizeof(v.x));
}

/* returns the sum of vectors a and b, overflow-unsafe */
static inline Vec vec_add(Vec a, Vec b) {
	return (Vec){a.x + b.x, a.y + b.y};
}

/* scales vector v by the fraction num/denom
 * overflow-unsafe except when |denom|>=|num|
 */
static inline Vec vec_scale(Vec v, short num, long denom) {
	return (Vec){((long)v.x * num) / denom, ((long)v.y * num) / denom};
}

/* negates vector v */
static inline Vec vec_neg(Vec v) {
	return vec_scale(v, -1, 1);
}

/* returns a vector with same angle as v, and specified magnitude */
static inline Vec vec_normalize(Vec v, short magnitude) {
	return vec_scale(v, magnitude, vec_norm(v));
}

/* returns a random vector inside a rectangle using rand() */
static inline Vec vec_rand(Vec min, Vec max) {
	return (Vec){
		rand() % ((long)max.x + 1) + min.x,
		rand() % ((long)max.y + 1) + min.y
	};
}

static inline Vec vec_min(Vec a, Vec b) {
	return (Vec){a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y};
}

static inline Vec vec_max(Vec a, Vec b) {
	return (Vec){a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y};
}
