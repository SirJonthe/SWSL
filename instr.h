#ifndef INSTR_H
#define INSTR_H

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



#endif // INSTR_H
