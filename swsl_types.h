#ifndef SWSL_TYPES_H
#define SWSL_TYPES_H

#include "MiniLib/MPL/mplWide.h"

namespace swsl
{

template < typename wide_t, int n >
class wide_vec
{
private:
	wide_t e[n];

public:
	wide_vec( void ) {}
	wide_vec(const wide_t &x) { for (int i; i < n; ++i) { e[i] = x; } }
	wide_vec(const wide_t &x, const mpl::wide_bool&) : wide_vec(x) {}
	wide_vec(const wide_t &x, const wide_t &y) { e[0] = x; e[1] = y; }
	wide_vec(const wide_t &x, const wide_t &y, const mpl::wide_bool&) : wide_vec(x, y) {}
	wide_vec(const wide_t &x, const wide_t &y, const wide_t &z) { e[0] = x; e[1] = y; e[2] = z; }
	wide_vec(const wide_t &x, const wide_t &y, const wide_t &z, const mpl::wide_bool&) : wide_vec(x, y, z) {}
	wide_vec(const wide_t &x, const wide_t &y, const wide_t &z, const wide_t &w) { e[0] = x; e[1] = y; e[2] = z; e[3] = w; }
	wide_vec(const wide_t &x, const wide_t &y, const wide_t &z, const wide_t &w, const mpl::wide_bool&) : wide_vec(x, y, z, w) {}

	wide_vec<wide_t, n> &operator=(const wide_t &r);
	wide_vec<wide_t, n> &operator=(const wide_vec<wide_t, n> &v);

	wide_vec<wide_t, n> operator+(const wide_vec<wide_t, n> &r) const;
	wide_vec<wide_t, n> operator-(const wide_vec<wide_t, n> &r) const;
	wide_vec<wide_t, n> operator*(const wide_vec<wide_t, n> &r) const;
	wide_vec<wide_t, n> operator/(const wide_vec<wide_t, n> &r) const;
	wide_vec<wide_t, n> operator&(const wide_vec<wide_t, n> &r) const;
	wide_vec<wide_t, n> operator|(const wide_vec<wide_t, n> &r) const;

	void set(int a, const wide_vec<wide_t, 1> &x);
	void set(int a, int b, wide_vec<wide_t, 1> &x);
	void set(int a, int b, const wide_vec<wide_t, 2> &x);
	void set(int a, int b, int c, const wide_vec<wide_t, 1> &x);
	void set(int a, int b, int c, const wide_vec<wide_t, 3> &x);
	void set(int a, int b, int c, int d, const wide_vec<wide_t, 1> &x);
	void set(int a, int b, int c, int d, const wide_vec<wide_t, 4> &x);

	wide_t              &operator[](int a);
	const wide_t        &operator[](int a) const;
	wide_vec<wide_t, 2>  operator[](int a, int b) const;
	wide_vec<wide_t, 3>  operator[](int a, int b, int c) const;
	wide_vec<wide_t, 4>  operator[](int a, int b, int c, int d) const;
};

template < typename wide_t, int n >
inline void set(wide_vec<wide_t, n> &l, const wide_vec<wide_t, n> &r, const mpl::wide_bool &lmask)
{
	for (int i = 0; i < n; ++i) {
		l = wide_t::merge(l[i], r[i], lmask);
	}
}

typedef wide_vec<mpl::wide_int, 1>       wide_int1;
typedef wide_vec<mpl::wide_int, 2>       wide_int2;
typedef wide_vec<mpl::wide_int, 3>       wide_int3;
typedef wide_vec<mpl::wide_int, 4>       wide_int4;
typedef wide_vec<mpl::wide_fixed<16>, 1> wide_fixed1;
typedef wide_vec<mpl::wide_fixed<16>, 2> wide_fixed2;
typedef wide_vec<mpl::wide_fixed<16>, 3> wide_fixed3;
typedef wide_vec<mpl::wide_fixed<16>, 4> wide_fixed4;
typedef wide_vec<mpl::wide_float, 1>     wide_float1;
typedef wide_vec<mpl::wide_float, 2>     wide_float2;
typedef wide_vec<mpl::wide_float, 3>     wide_float3;
typedef wide_vec<mpl::wide_float, 4>     wide_float4;

template < typename wide_t, int n >
swsl::wide_vec<wide_t, n> wide_max(const swsl::wide_vec<wide_t, n> &a, const swsl::wide_vec<wide_t, n> &b, const mpl::wide_bool&)
{
	swsl::wide_vec<wide_t, n> o;
	for (int i = 0; i < n; ++i) {
		o[i] = wide_t::max(a[i], b[i]);
	}
	return o;
}

template < typename wide_t, int n >
swsl::wide_vec<wide_t, n> wide_min(const swsl::wide_vec<wide_t, n> &a, const swsl::wide_vec<wide_t, n> &b, const mpl::wide_bool&)
{
	swsl::wide_vec<wide_t, n> o;
	for (int i = 0; i < n; ++i) {
		o[i] = wide_t::min(a[i], b[i]);
	}
	return o;
}

template < typename wide_t, int n >
swsl::wide_vec<wide_t, n> wide_abs(const swsl::wide_vec<wide_t, n> &a, const mpl::wide_bool&)
{
	swsl::wide_vec<wide_t, n> o;
	for (int i = 0; i < n; ++i) {
		o[i] = wide_t::abs(a[i]);
	}
	return o;
}

}

template < typename wide_t, int n >
swsl::wide_vec<wide_t, n> &swsl::wide_vec<wide_t, n>::operator=(const wide_t &r)
{
	for (int i = 0; i < n; ++i) {
		e[i] = r;
	}
	return *this;
}

template < typename wide_t, int n >
swsl::wide_vec<wide_t, n> &swsl::wide_vec<wide_t, n>::operator=(const swsl::wide_vec<wide_t, n> &v)
{
	if (this != &v) {
		for (int i = 0; i < n; ++i) {
			e[i] = v.e[i];
		}
	}
	return *this;
}

template < typename wide_t, int n >
swsl::wide_vec<wide_t, n> swsl::wide_vec<wide_t, n>::operator+(const swsl::wide_vec<wide_t, n> &r)
{
	swsl::wide_vec<wide_t, n> o;
	for (int i = 0; i < n; ++i) {
		o.e[i] = e[i] + r.e[i];
	}
	return o;
}

template < typename wide_t, int n >
swsl::wide_vec<wide_t, n> swsl::wide_vec<wide_t, n>::operator-(const swsl::wide_vec<wide_t, n> &r)
{
	swsl::wide_vec<wide_t, n> o;
	for (int i = 0; i < n; ++i) {
		o.e[i] = e[i] - r.e[i];
	}
	return o;
}

template < typename wide_t, int n >
swsl::wide_vec<wide_t, n> swsl::wide_vec<wide_t, n>::operator*(const swsl::wide_vec<wide_t, n> &r)
{
	swsl::wide_vec<wide_t, n> o;
	for (int i = 0; i < n; ++i) {
		o.e[i] = e[i] * r.e[i];
	}
	return o;
}

template < typename wide_t, int n >
swsl::wide_vec<wide_t, n> swsl::wide_vec<wide_t, n>::operator/(const swsl::wide_vec<wide_t, n> &r)
{
	swsl::wide_vec<wide_t, n> o;
	for (int i = 0; i < n; ++i) {
		o.e[i] = e[i] / r.e[i];
	}
	return o;
}

template < typename wide_t, int n >
swsl::wide_vec<wide_t, n> swsl::wide_vec<wide_t, n>::operator&(const swsl::wide_vec<wide_t, n> &r)
{
	swsl::wide_vec<wide_t, n> o;
	for (int i = 0; i < n; ++i) {
		o.e[i] = e[i] & r.e[i];
	}
	return o;
}

template < typename wide_t, int n >
void swsl::wide_vec<wide_t, n>::set(int a, const wide_vec<wide_t, 1> &x)
{
	e[a] = x.e[0];
}

template < typename wide_t, int n >
void swsl::wide_vec<wide_t, n>::set(int a, int b, const wide_vec<wide_t, 1> &x)
{
	e[a] = x.e[0];
	e[b] = x.e[0];
}

template < typename wide_t, int n >
void swsl::wide_vec<wide_t, n>::set(int a, int b, const wide_vec<wide_t, 2> &x)
{
	e[a] = x.e[0];
	e[b] = x.e[1];
}

template < typename wide_t, int n >
void swsl::wide_vec<wide_t, n>::set(int a, int b, int c, const wide_vec<wide_t, 1> &x)
{
	e[a] = x.e[0];
	e[b] = x.e[0];
	e[c] = x.e[0];
}

template < typename wide_t, int n >
void swsl::wide_vec<wide_t, n>::set(int a, int b, int c, const wide_vec<wide_t, 3> &x)
{
	e[a] = x.e[0];
	e[b] = x.e[1];
	e[c] = x.e[2];
}

template < typename wide_t, int n >
void swsl::wide_vec<wide_t, n>::set(int a, int b, int c, int d, const wide_vec<wide_t, 1> &x)
{
	e[a] = x.e[0];
	e[b] = x.e[0];
	e[c] = x.e[0];
	e[d] = x.e[0];
}

template < typename wide_t, int n >
void swsl::wide_vec<wide_t, n>::set(int a, int b, int c, int d, const wide_vec<wide_t, 4> &x)
{
	e[a] = x.e[0];
	e[b] = x.e[1];
	e[c] = x.e[2];
	e[d] = x.e[3];
}

template < typename wide_t, int n >
swsl::wide_vec<wide_t, n> swsl::wide_vec<wide_t, n>::operator|(const swsl::wide_vec<wide_t, n> &r)
{
	swsl::wide_vec<wide_t, n> o;
	for (int i = 0; i < n; ++i) {
		o.e[i] = e[i] | r.e[i];
	}
	return o;
}

template < typename wide_t, int n >
swsl::wide_t &swsl::wide_vec<wide_t, n>::operator[](int a) const
{
	return e[a];
}

template < typename wide_t, int n >
const swsl::wide_t &swsl::wide_vec<wide_t, n>::operator[](int a) const
{
	return e[a];
}

template < typename wide_t, int n >
swsl::wide_vec<wide_t, 2> swsl::wide_vec<wide_t, n>::operator[](int a, int b) const
{
	return swsl::wide_vec<wide_t, 2>(e[a], e[b]);
}

template < typename wide_t, int n >
swsl::wide_vec<wide_t, 3> swsl::wide_vec<wide_t, n>::operator[](int a, int b, int c) const
{
	return swsl::wide_vec<wide_t, 3>(e[a], e[b], e[c]);
}

template < typename wide_t, int n >
swsl::wide_vec<wide_t, 4> swsl::wide_vec<wide_t, n>::operator[](int a, int b, int c, int d) const
{
	return swsl::wide_vec<wide_t, 4>(e[a], e[b], e[c], e[d]);
}

template < typename wide_t, int n >
class wide_array
{
private:
	wide_t e[n];

public:
	wide_t &operator[](int i) { return e[i]; }
	const wide_t &operator[](int i) const { return e[i]; }
	// TODO
	// wide_setter<wide_t> &operator[](const mpl::wide_int &i);
	// const wide_t operator[](const mpl::wide_int &i) const;
};

#endif // SWSL_TYPES_H
