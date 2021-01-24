#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define MIN(a, b) ((a)<(b)?(a):(b))
#define MAX(a, b) ((a)>(b)?(a):(b))
#define lenof(a) (sizeof(a)/sizeof(*(a)))
#define endof(a) (a + lenof(a))

typedef struct { int32_t x, y; } Vec;

static const Vec vec_zero = {0, 0};

/* returns integer (floored) square root of x, capped at 2^bits
 * fast, gcc tends to unroll with constant bits at -O3
 */
static inline int32_t isqrt(int64_t x, unsigned bits) {
	uint32_t y = 0;

	while (bits-- > 0) {
		uint64_t a = (y + (1 << bits)) << bits;

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
static inline int64_t vec_normsq(Vec v) {
	return (int64_t)v.x * v.x + (int64_t)v.y * v.y;
}

/* returns the norm (length) of vector v */
static inline int32_t vec_norm(Vec v) {
	return isqrt(vec_normsq(v), CHAR_BIT*sizeof(v.x));
}

/* returns the sum of vectors a and b, overflow-unsafe */
static inline Vec vec_add(Vec a, Vec b) {
	return (Vec){a.x + b.x, a.y + b.y};
}

/* scales vector v by the fraction num/denom
 * overflow-unsafe except when |denom|>=|num|
 */
static inline Vec vec_scale(Vec v, int32_t num, int64_t denom) {
	return (Vec){
		(int32_t)(((int64_t)v.x * num) / denom),
		(int32_t)(((int64_t)v.y * num) / denom)
	};
}

/* negates vector v */
static inline Vec vec_neg(Vec v) {
	return vec_scale(v, -1, 1);
}

/* returns a vector with same angle as v, and specified magnitude */
static inline Vec vec_normalize(Vec v, int32_t magnitude) {
	return vec_scale(v, magnitude, vec_norm(v));
}

/* returns a random vector inside a rectangle using rand() */
/*static inline Vec vec_rand(Vec min, Vec max) {
	return (Vec){
		rand() % ((int64_t)max.x + 1) + min.x,
		rand() % ((int64_t)max.y + 1) + min.y
	};
}*/

static inline Vec vec_min(Vec a, Vec b) {
	return (Vec){a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y};
}

static inline Vec vec_max(Vec a, Vec b) {
	return (Vec){a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y};
}

static inline Vec vec_mul(Vec a, Vec b) {
    return (Vec){a.x * b.x, a.y * b.y};
}

static inline Vec vec_div(Vec a, Vec b) {
    return (Vec){a.x / b.x, a.y / b.y};
}

static inline Vec vec_mod(Vec a, Vec b) {
    return (Vec){a.x % b.x, a.y % b.y};
}

/* returns true if vector v is inside the rectangle with tl and br as top-left
 * and bottom-right corners
 */
static inline bool vec_isinside(Vec v, Vec tl, Vec br) {
	return v.x >= tl.x && v.x <= br.x && v.y >= tl.y && v.y <= br.y;
}

static inline long vec_grad(Vec a){
    return (((long) a.y)<<16)/((((long) a.x)<<16)+1);
}
