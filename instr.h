#ifndef INSTR_H
#define INSTR_H

#include "MiniLib/MTL/mtlString.h"

enum Instruction
{
	NOP,

	UINPUT_I,

	END,

	UENTRY_I,

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
	mtlChars    name;
	Instruction instr;
	int         params;
};

const InstructionInfo gInstr[INSTR_COUNT] = {
	{ mtlChars("nop"),     NOP,      0 },
	{ mtlChars("end"),     END,      0 },
	{ mtlChars("uinput_i"),UINPUT_I, 1 },
	{ mtlChars("uentry_i"),UENTRY_I, 1 },
	{ mtlChars("fpush_m"), FPUSH_M,  1 },
	{ mtlChars("fpush_i"), FPUSH_I,  1 },
	{ mtlChars("upush_i"), UPUSH_I,  1 },
	{ mtlChars("fpop_m"),  FPOP_M,   1 },
	{ mtlChars("upop_i"),  UPOP_I,   1 },
	{ mtlChars("ujmp_i"),  UJMP_I,   1 },
	{ mtlChars("fset_mm"), FSET_MM,  2 },
	{ mtlChars("fset_mi"), FSET_MI,  2 },
	{ mtlChars("fadd_mm"), FADD_MM,  2 },
	{ mtlChars("fadd_mi"), FADD_MI,  2 },
	{ mtlChars("fsub_mm"), FSUB_MM,  2 },
	{ mtlChars("fsub_mi"), FSUB_MI,  2 },
	{ mtlChars("fmul_mm"), FMUL_MM,  2 },
	{ mtlChars("fmul_mi"), FMUL_MI,  2 },
	{ mtlChars("fdiv_mm"), FDIV_MM,  2 },
	{ mtlChars("fdiv_mi"), FDIV_MI,  2 }
};

#endif // INSTR_H
