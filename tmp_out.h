#ifndef wide_included__
#define wide_included__

#include "swsl_types.h"

inline const mpl::wide_int wide_main(const mpl::wide_float _pz, const mpl::wide_float _pr, const mpl::wide_float _pg, const mpl::wide_float _pb, mpl::wide_float& _r, mpl::wide_float& _g, mpl::wide_float& _b, mpl::wide_float& _z, mpl::wide_bool m0)
{
	mpl::wide_int ret;
	const mpl::wide_bool _bl = mpl::wide_bool(true);
	{
		mpl::wide_bool m1 = (_pz < _z) & m0;
		if ( !(m1.all_fail()) )
		{
			swsl::wide_assign(_z, _pz, m1);
			swsl::wide_assign(_r, _pr, m1);
			swsl::wide_assign(_g, _pg, m1);
			swsl::wide_assign(_b, _pb, m1);
			{
				const mpl::wide_bool m2 = (_z == mpl::wide_float(0.0f)) & m1;
				if ( !(m2.all_fail()) )
				{
					swsl::wide_assign(ret, mpl::wide_int(0), m2);
					m0 = m0 & (!m2);
					if (m0.all_fail()) { return ret; }
					m1 = m1 & m0;
				}
			}
		}
		m1 = (!m1) & m0;
		if ( !(m1.all_fail()) )
		{
			mpl::wide_bool m2 = (_pz == _z) & m1;
			if ( !(m2.all_fail()) )
			{
				swsl::wide_assign(_r, mpl::wide_float(0.0f), m2);
				swsl::wide_assign(_g, mpl::wide_float(0.0f), m2);
				swsl::wide_assign(_b, mpl::wide_float(0.0f), m2);
			}
			m2 = (!m2) & m1;
			if ( !(m2.all_fail()) )
			{
				swsl::wide_assign(_r, mpl::wide_float(1.0f), m2);
				swsl::wide_assign(_g, mpl::wide_float(0.0f), m2);
				swsl::wide_assign(_b, mpl::wide_float(1.0f), m2);
			}
		}
	}
	mpl::wide_int _a = mpl::wide_int(0);
	{
		const mpl::wide_bool m1 = (_a < mpl::wide_int(10)) & m0;
		while ( !(m1.all_fail()) )
		{
			mpl::wide_int _b = mpl::wide_int(1);
			const mpl::wide_int _c = mpl::wide_int(2);
			_b = (_b + _c);
			swsl::wide_assign(_a, (_a + mpl::wide_int(1)), m1);
		}
	}
	swsl::wide_assign(ret, _a, m0);
	return ret;
}

inline const mpl::wide_int wide_main(void* inout, mpl::wide_bool m0)
{
	return wide_main(
		*( (mpl::wide_float*)(((char*)inout)) ),
		*( (mpl::wide_float*)(((char*)inout) + sizeof(mpl::wide_float)) ),
		*( (mpl::wide_float*)(((char*)inout) + sizeof(mpl::wide_float) + sizeof(mpl::wide_float)) ),
		*( (mpl::wide_float*)(((char*)inout) + sizeof(mpl::wide_float) + sizeof(mpl::wide_float) + sizeof(mpl::wide_float)) ),
		*( (mpl::wide_float*)(((char*)inout) + sizeof(mpl::wide_float) + sizeof(mpl::wide_float) + sizeof(mpl::wide_float) + sizeof(mpl::wide_float)) ),
		*( (mpl::wide_float*)(((char*)inout) + sizeof(mpl::wide_float) + sizeof(mpl::wide_float) + sizeof(mpl::wide_float) + sizeof(mpl::wide_float) + sizeof(mpl::wide_float)) ),
		*( (mpl::wide_float*)(((char*)inout) + sizeof(mpl::wide_float) + sizeof(mpl::wide_float) + sizeof(mpl::wide_float) + sizeof(mpl::wide_float) + sizeof(mpl::wide_float) + sizeof(mpl::wide_float)) ),
		*( (mpl::wide_float*)(((char*)inout) + sizeof(mpl::wide_float) + sizeof(mpl::wide_float) + sizeof(mpl::wide_float) + sizeof(mpl::wide_float) + sizeof(mpl::wide_float) + sizeof(mpl::wide_float) + sizeof(mpl::wide_float)) ),
		m0
	);
}

#endif
