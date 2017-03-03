#ifndef wide_included__
#define wide_included__

#include "swsl_types.h"

inline const mpl::wide_int wide_max(const mpl::wide_int& _a, const mpl::wide_int& _b, mpl::wide_bool m0)
{
	mpl::wide_int ret;
	{
		const mpl::wide_bool m1 = (_a < _b) & m0;
		if ( !(m1.all_fail()) )
		{
			swsl::wide_assign(ret, _b, m1);
			m0 = m0 & (!m1);
			if (m0.all_fail()) { return ret; }
		}
	}
	swsl::wide_assign(ret, _a, m0);
	return ret;
}

#endif
