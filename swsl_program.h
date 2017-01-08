#ifndef SWSL_PROGRAM_H_INCLUDED__
#define SWSL_PROGRAM_H_INCLUDED__

#include "MiniLib/MPL/mplWide.h"

#include "swsl_instr.h"

namespace swsl
{

class ProgramVM
{
private:


public:
	bool SetProgram(const swsl::Binary &bin);
	bool operator()(void *data, const mpl::wide_bool &m0);
};

}

#endif // SWSL_PROGRAM_H_INCLUDED__
