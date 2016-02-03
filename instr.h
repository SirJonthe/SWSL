#ifndef INSTR_H
#define INSTR_H

#include "MiniLib/MTL/mtlString.h"

enum InstructionSet
{
	NOP,
	END,

	FPUSH_M,
	FPUSH_I,
	UPUSH_I,

	FPOP_M,
	UPOP_I,

	UJMP_I,

	FSET_MM,
	FSET_MI,

	FADD_MM,
	FADD_MI,

	FSUB_MM,
	FSUB_MI,

	FMUL_MM,
	FMUL_MI,

	FDIV_MM,
	FDIV_MI,

	INSTR_COUNT
};

struct InstructionInfo
{
	mtlChars       name;
	InstructionSet instr;
	int            params;
	bool           const_float_src;
};

const InstructionInfo gInstr[INSTR_COUNT] = {
	{ mtlChars("nop"),     NOP,      0, false },
	{ mtlChars("end"),     END,      0, false },
	{ mtlChars("fpush_m"), FPUSH_M,  1, false },
	{ mtlChars("fpush_i"), FPUSH_I,  1, true  },
	{ mtlChars("upush_i"), UPUSH_I,  1, false },
	{ mtlChars("fpop_m"),  FPOP_M,   1, false },
	{ mtlChars("upop_i"),  UPOP_I,   1, false },
	{ mtlChars("ujmp_i"),  UJMP_I,   1, false },
	{ mtlChars("fset_mm"), FSET_MM,  2, false },
	{ mtlChars("fset_mi"), FSET_MI,  2, true  },
	{ mtlChars("fadd_mm"), FADD_MM,  2, false },
	{ mtlChars("fadd_mi"), FADD_MI,  2, true  },
	{ mtlChars("fsub_mm"), FSUB_MM,  2, false },
	{ mtlChars("fsub_mi"), FSUB_MI,  2, true  },
	{ mtlChars("fmul_mm"), FMUL_MM,  2, false },
	{ mtlChars("fmul_mi"), FMUL_MI,  2, true  },
	{ mtlChars("fdiv_mm"), FDIV_MM,  2, false },
	{ mtlChars("fdiv_mi"), FDIV_MI,  2, true  }
};

typedef unsigned short addr_t;

union Instruction
{
	InstructionSet instr;
	float          fl_imm;
	addr_t         u_addr;
};

const int gMetaData_InputIndex = 0;
const int gMetaData_EntryIndex = 1;

#endif // INSTR_H
