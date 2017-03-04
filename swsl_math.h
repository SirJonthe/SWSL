#ifndef SWSL_MATH_H_INCLUDED__
#define SWSL_MATH_H_INCLUDED__

namespace swsl
{

template < typename wide_t >
void mov_if_true(wide_t &a, const wide_t &b, const mpl::wide_bool &cond_mask)
{
	a = wide_t::mov_if_true(a, b, cond_mask);
}

template < typename wide_t >
wide_t max(const wide_t &a, const wide_t &b, const mpl::wide_bool&)
{
	return wide_t::max(a, b);
}

template < typename wide_t >
wide_t min(const wide_t &a, const wide_t &b, const mpl::wide_bool&)
{
	return wide_t::min(a, b);
}

template < typename wide_t >
wide_t sqrt(const wide_t &a, const mpl::wide_bool&)
{
	return wide_t::sqrt(a);
}

}

#endif // SWSL_MATH_H_INCLUDED__
