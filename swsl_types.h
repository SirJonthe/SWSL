#ifndef SWSL_TYPES_H_INCLUDED__
#define SWSL_TYPES_H_INCLUDED__

#include "MiniLib/MPL/mplWide.h"

namespace swsl
{

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

}

#endif // SWSL_TYPES_H_INCLUDED__
