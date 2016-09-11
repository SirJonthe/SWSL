#ifndef SWSL_INSTR_H
#define SWSL_INSTR_H

#include "MiniLib/MTL/mtlString.h"

namespace swsl {

	enum InstructionSet
	{
		// TST, bitwise operation
		// INT, int operation
		// FIX, fixed operation
		// FLT, float operation

		NOP,
		END,
		RETURN,
		TST_RETURN,


		TST_PUSH,
		TST_POP,
		TST_AND,
		TST_OR,
		TST_INV,


		FLT_PUSH_M,
		FLT_PUSH_I,
		UNS_PUSH_I,

		FLT_POP_M,
		UNS_POP_I,


		UNS_JMP_I,


		FLT_MSET_MM,
		FLT_MSET_MI,


		FLT_SET_MM,
		FLT_SET_MI,

		FLT_ADD_MM,
		FLT_ADD_MI,

		FLT_SUB_MM,
		FLT_SUB_MI,

		FLT_MUL_MM,
		FLT_MUL_MI,

		FLT_DIV_MM,
		FLT_DIV_MI,


		FLT_EQ_MM,
		FLT_EQ_MI,

		FLT_NEQ_MM,
		FLT_NEQ_MI,

		FLT_LT_MM,
		FLT_LT_MI,

		FLT_LTE_MM,
		FLT_LTE_MI,

		FLT_GT_MM,
		FLT_GT_MI,

		FLT_GTE_MM,
		FLT_GTE_MI,

		INSTR_COUNT
	};

	struct InstructionInfo
	{
		mtlChars       name;
		InstructionSet instr;
		int            params;
	};

	const InstructionInfo gInstr[INSTR_COUNT] = {
		{ mtlChars("nop"),         NOP,         0 },
		{ mtlChars("end"),         END,         0 },
		{ mtlChars("return"),      RETURN,      0 },
		{ mtlChars("tst_return"),  TST_RETURN,  0 },

		{ mtlChars("tst_push"),    TST_PUSH,    0 },
		{ mtlChars("tst_pop"),     TST_POP,     0 },
		{ mtlChars("tst_and"),     TST_AND,     0 },
		{ mtlChars("tst_or"),      TST_OR,      0 },
		{ mtlChars("tst_inv"),     TST_INV,     0 },

		{ mtlChars("flt_push_m"),  FLT_PUSH_M,  1 },
		{ mtlChars("flt_push_i"),  FLT_PUSH_I,  1 },
		{ mtlChars("uns_push_i"),  UNS_PUSH_I,  1 },
		{ mtlChars("flt_pop_m"),   FLT_POP_M,   1 },
		{ mtlChars("uns_pop_i"),   UNS_POP_I,   1 },

		{ mtlChars("uns_jmp_i"),   UNS_JMP_I,   1 },

		{ mtlChars("flt_mset_mm"), FLT_MSET_MM, 2 },
		{ mtlChars("flt_mset_mi"), FLT_MSET_MI, 2 },

		{ mtlChars("flt_set_mm"),  FLT_SET_MM,  2 },
		{ mtlChars("flt_set_mi"),  FLT_SET_MI,  2 },
		{ mtlChars("flt_add_mm"),  FLT_ADD_MM,  2 },
		{ mtlChars("flt_add_mi"),  FLT_ADD_MI,  2 },
		{ mtlChars("flt_sub_mm"),  FLT_SUB_MM,  2 },
		{ mtlChars("flt_sub_mi"),  FLT_SUB_MI,  2 },
		{ mtlChars("flt_mul_mm"),  FLT_MUL_MM,  2 },
		{ mtlChars("flt_mul_mi"),  FLT_MUL_MI,  2 },
		{ mtlChars("flt_div_mm"),  FLT_DIV_MM,  2 },
		{ mtlChars("flt_div_mi"),  FLT_DIV_MI,  2 },

		{ mtlChars("flt_eq_mm"),   FLT_EQ_MM,   2 },
		{ mtlChars("flt_eq_mi"),   FLT_EQ_MI,   2 },
		{ mtlChars("flt_neq_mm"),  FLT_NEQ_MM,  2 },
		{ mtlChars("flt_neq_mi"),  FLT_NEQ_MM,  2 },
		{ mtlChars("flt_lt_mm"),   FLT_LT_MM,   2 },
		{ mtlChars("flt_lt_mi"),   FLT_LT_MM,   2 },
		{ mtlChars("flt_lte_mm"),  FLT_LTE_MM,  2 },
		{ mtlChars("flt_lte_mi"),  FLT_LTE_MM,  2 },
		{ mtlChars("flt_gt_mm"),   FLT_GT_MM,   2 },
		{ mtlChars("flt_gt_mi"),   FLT_GT_MM,   2 },
		{ mtlChars("flt_gte_mm"),  FLT_GTE_MM,  2 },
		{ mtlChars("flt_gte_mi"),  FLT_GTE_MM,  2 }
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

}

#define SWSL_INSTR_IMM_PARAM2(X) ((X) >= swsl::FLT_SET_MM && (X & 1) != (swsl::FLT_SET_MM & 1))

#endif // INSTR_H
