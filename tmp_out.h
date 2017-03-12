#ifndef wide_included__
#define wide_included__

#include "swsl_types.h"
#include "swsl_math.h"

inline const mpl::wide_float wide_max(const mpl::wide_float& _a, const mpl::wide_float& _b, mpl::wide_bool m0)
{
	mpl::wide_float ret;
	{
		const mpl::wide_bool m1 = (_a < _b) & m0;
		if ( !(m1.all_fail()) )
		{
			swsl::mov_if_true(ret, _b, m1);
			m0 = m0 & (!m1);
			if (m0.all_fail()) { return ret; }
		}
	}
	swsl::mov_if_true(ret, _a, m0);
	return ret;
}

inline void wide_mat_mult(
		  mpl::wide_float &x,         mpl::wide_float &y,         mpl::wide_float &z,
	const mpl::wide_float &m00, const mpl::wide_float &m10, const mpl::wide_float &m20,
	const mpl::wide_float &m01, const mpl::wide_float &m11, const mpl::wide_float &m21,
	const mpl::wide_float &m02, const mpl::wide_float &m12, const mpl::wide_float &m22)
{
	mpl::wide_float a = x*m00 + y*m10 + z*m20;
	mpl::wide_float b = x*m01 + y*m11 + z*m21;
	mpl::wide_float c = x*m02 + y*m12 + z*m22;
	x = a;
	y = b;
	z = c;
}

#if MPL_SIMD == MPL_SIMD_SSE
inline void wide_mat_mult_raw(
		  __m128 &x,         __m128 &y,         __m128 &z,
	const __m128 &m00, const __m128 &m10, const __m128 &m20,
	const __m128 &m01, const __m128 &m11, const __m128 &m21,
	const __m128 &m02, const __m128 &m12, const __m128 &m22)
{
	__m128 a = _mm_add_ps(_mm_mul_ps(x, m00), _mm_add_ps(_mm_mul_ps(y, m10), _mm_mul_ps(z, m20)));
	__m128 b = _mm_add_ps(_mm_mul_ps(x, m01), _mm_add_ps(_mm_mul_ps(y, m11), _mm_mul_ps(z, m21)));
	__m128 c = _mm_add_ps(_mm_mul_ps(x, m02), _mm_add_ps(_mm_mul_ps(y, m12), _mm_mul_ps(z, m22)));
	x = a;
	y = b;
	z = c;
}
#endif

#endif

