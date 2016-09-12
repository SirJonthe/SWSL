#ifndef SWSL_AUX_H_INCLUDED__
#define SWSL_AUX_H_INCLUDED__

#include <iostream>
#include "MiniLib/MTL/mtlString.h"

namespace swsl
{

	inline void print_ch(const mtlChars &c)
	{
		for (int i = 0; i < c.GetSize(); ++i) {
			std::cout << c[i];
		}
	}

	inline void print_line(const mtlChars &c)
	{
		print_ch(c);
		std::cout << std::endl;
	}

}

#endif // SWSL_AUX_H_INCLUDED__
