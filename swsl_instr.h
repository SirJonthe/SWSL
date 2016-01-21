#ifndef SWSL_INSTR_H
#define SWSL_INSTR_H

namespace swsl
{

enum InstructionSet
{
	// 0 arguments

	NOP,
	END,

	// 1 argument

	POP,
	PUSH,
	JMP,

	// 2 arguments

	// Various arithmetic instructions
	MOV_FF,
	MOV_FC,
	ADD_FF,
	ADD_FC,
	SUB_FF,
	SUB_FC,
	MUL_FF,
	MUL_FC,
	DIV_FF,
	DIV_FC,
	MAX_FF,
	MAX_FC,
	MIN_FF,
	MIN_FC,
	SQRT_FF,
	SQRT_FC,

	// Comparisons generate a comparison mask (and an inverse comparison mask).
	// All variables that originate from outside the statement are copied inside the statement if they are modified there.
	// On "endif" copied variables that have been modified inside the statements are merged (MRG) with the original variable using the comparison masks.
	EQ_FF,
	EQ_FC,
	EQ_CF,
	NEQ_FF,
	NEQ_FC,
	NEQ_CF,
	LT_FF,
	LT_FC,
	LT_CF,
	LE_FF,
	LE_FC,
	LE_CF,
	GT_FF,
	GT_FC,
	GT_CF,
	GE_FF,
	GE_FC,
	GE_CF,
	// NOTE: One of the branches resulting from a comparison between two constants can be optimized away.

	MRG, // branch_a, branch_b (uses value at top of stack as mask)

	INSTR_COUNT
};

typedef unsigned int instr_t;

}

#endif // SWSL_INSTR_H
