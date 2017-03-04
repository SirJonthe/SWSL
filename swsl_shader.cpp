#include "swsl_shader.h"
#include "swsl_instr.h"

#include "MiniLib/MTL/mtlMemory.h"

// Points to an instruction in the instruction cache.
#define read_1_instr(reg) reg = &program[iptr++]
#define read_2_instr(reg_a, reg_b) read_1_instr(reg_a); read_1_instr(reg_b)

// Convert an instruction pointer to a value, or dereference the pointer to a value stored in the data cache.
#define to_wide_float(reg)       (mpl::wide_float(*(float*)(reg)))

void swsl::Shader::Delete( void )
{
	m_program.Free();
	m_errors.RemoveAll();
	m_warnings.RemoveAll();
}

bool swsl::Shader::IsValid( void ) const
{
	return m_program.GetSize() > 0 && m_errors.GetSize() == 0 && (m_inputs->constant.count + m_inputs->varying.count + m_inputs->fragments.count == m_program[gMetaData_InputIndex].u_addr);
}

int swsl::Shader::GetErrorCount( void ) const
{
	return m_errors.GetSize();
}

int swsl::Shader::GetWarningCount( void ) const
{
	return m_warnings.GetSize();
}

void swsl::Shader::SetInputArrays(InputArrays &inputs)
{
	m_inputs = &inputs;
}

const mtlItem<swsl::CompilerMessage> *swsl::Shader::GetErrors( void ) const
{
	return m_errors.GetFirst();
}

const mtlItem<swsl::CompilerMessage> *swsl::Shader::GetWarnings( void ) const
{
	return m_warnings.GetFirst();
}

#include <iostream>
void print_fl(const mpl::wide_float &f)
{
	float fs[MPL_WIDTH];
	f.to_scalar(fs);
	std::cout << "[";
	for (int i = 0; i < MPL_WIDTH; ++i) {
		std::cout << fs[i] << " ";
	}
	std::cout << "]";
}

bool swsl::Shader::Run(const mpl::wide_bool &frag_mask) const
{
	mpl::wide_float stack[STACK_SIZE];

	swsl::addr_t   iptr = m_program[gMetaData_EntryIndex].u_addr; // instruction pointer
	swsl::addr_t   sptr = 0;                                      // stack pointer
	mpl::wide_bool mask_reg = true;                               // current conditional mask (state is pushed and popped from stack)
	mpl::wide_bool test_reg;                                      // current test register

	const swsl::Instruction *program      = (const Instruction*)(&m_program[0]);
	const swsl::addr_t       program_size = (addr_t)m_program.GetSize();

	void       *reg_a;
	const void *reg_b;

	// Transfer all data to local memory addressable by VM
	int frag_offset = m_inputs->varying.count + m_inputs->constant.count;
	mtlCopy(stack, m_inputs->constant.data, m_inputs->constant.count);
	mtlCopy(stack + m_inputs->constant.count, m_inputs->varying.data, m_inputs->varying.count);
	mtlCopy(stack + frag_offset, m_inputs->fragments.data, m_inputs->fragments.count);

	while (iptr < program_size) {

		switch (program[iptr++].instr) {

		case swsl::NOP:
			continue;

		case swsl::END: {
			// Sync up fragment data to output
			const mpl::wide_float *fragment_data = stack + frag_offset;
			for (int i = 0; i < m_inputs->fragments.count; ++i) {
				// NOTE: changed how CMOV works, cond_mask is now inverted
				m_inputs->fragments.data[i] = mpl::wide_float::mov_if_true(fragment_data[i], m_inputs->fragments.data[i], frag_mask);
			}
			return true;
		}

		case swsl::RETURN:
			iptr = *((swsl::addr_t*)(&stack[sptr--]));
			break;

		case swsl::TST_PUSH:
			stack[sptr++] = *(mpl::wide_float*)(&mask_reg);
			break;

		case swsl::TST_POP:
			mask_reg = *(mpl::wide_bool*)(&stack[sptr--]);
			break;

		case swsl::TST_AND:
			mask_reg = mask_reg & test_reg;
			break;

		case swsl::TST_OR:
			mask_reg = mask_reg | test_reg;
			break;

		case swsl::TST_INV:
			mask_reg = !mask_reg;
			break;

		case swsl::FLT_PUSH_M:
			stack[sptr] = stack[sptr - program[iptr++].u_addr];
			++sptr;
			break;

		case swsl::FLT_PUSH_I:
			stack[sptr++] = to_wide_float(program + iptr);
			++iptr;
			break;

		case swsl::UNS_PUSH_I:
			sptr += program[iptr++].u_addr;
			break;

		case swsl::FLT_POP_M: {
			swsl::addr_t addr = sptr - program[iptr++].u_addr;
			stack[addr] = stack[--sptr];
			break;
		}

		case swsl::UNS_POP_I:
			sptr -= program[iptr++].u_addr;
			break;

		case swsl::UNS_JMP_I:
			iptr = program[iptr].u_addr;
			++iptr;
			break;

		case swsl::FLT_MSET_MM:
			reg_a = stack + sptr - program[iptr++].u_addr;
			reg_b = stack + sptr - program[iptr++].u_addr;
			*(mpl::wide_float*)(reg_a) = *(mpl::wide_float*)(reg_b) & *(mpl::wide_float*)(&mask_reg);
			break;

		case swsl::FLT_MSET_MI:
			reg_a = stack + sptr - program[iptr++].u_addr;
			reg_b = &program[iptr++];
			*(mpl::wide_float*)(reg_a) = *(mpl::wide_float*)(reg_b) & *(mpl::wide_float*)(&mask_reg);
			break;

		case swsl::FLT_SET_MM:
			reg_a = stack + sptr - program[iptr++].u_addr;
			reg_b = stack + sptr - program[iptr++].u_addr;
			*(mpl::wide_float*)(reg_a) = *(mpl::wide_float*)(reg_b);
			break;

		case swsl::FLT_SET_MI:
			reg_a = stack + sptr - program[iptr++].u_addr;
			reg_b = &program[iptr++];
			*(mpl::wide_float*)(reg_a) = to_wide_float(reg_b);
			break;

		case swsl::FLT_ADD_MM:
			reg_a = stack + sptr - program[iptr++].u_addr;
			reg_b = stack + sptr - program[iptr++].u_addr;
			*(mpl::wide_float*)(reg_a) += *(mpl::wide_float*)(reg_b);
			break;

		case swsl::FLT_ADD_MI:
			reg_a = stack + sptr - program[iptr++].u_addr;
			reg_b = &program[iptr++];
			*(mpl::wide_float*)(reg_a) += to_wide_float(reg_b);
			break;

		case swsl::FLT_SUB_MM:
			reg_a = stack + sptr - program[iptr++].u_addr;
			reg_b = stack + sptr - program[iptr++].u_addr;
			*(mpl::wide_float*)(reg_a) -= *(mpl::wide_float*)(reg_b);
			break;

		case swsl::FLT_SUB_MI:
			reg_a = stack + sptr - program[iptr++].u_addr;
			reg_b = &program[iptr++];
			*(mpl::wide_float*)(reg_a) -= to_wide_float(reg_b);
			break;

		case swsl::FLT_MUL_MM:
			reg_a = stack + sptr - program[iptr++].u_addr;
			reg_b = stack + sptr - program[iptr++].u_addr;
			*(mpl::wide_float*)(reg_a) *= *(mpl::wide_float*)(reg_b);
			break;

		case swsl::FLT_MUL_MI:
			reg_a = stack + sptr - program[iptr++].u_addr;
			reg_b = &program[iptr++];
			*(mpl::wide_float*)(reg_a) *= to_wide_float(reg_b);
			break;

		case swsl::FLT_DIV_MM:
			reg_a = stack + sptr - program[iptr++].u_addr;
			reg_b = stack + sptr - program[iptr++].u_addr;
			*(mpl::wide_float*)(reg_a) /= *(mpl::wide_float*)(reg_b);
			break;

		case swsl::FLT_DIV_MI:
			reg_a = stack + sptr - program[iptr++].u_addr;
			reg_b = &program[iptr++];
			*(mpl::wide_float*)(reg_a) /= to_wide_float(reg_b);
			break;

		case swsl::FLT_EQ_MM:
			reg_a = stack + sptr - program[iptr++].u_addr;
			reg_b = stack + sptr - program[iptr++].u_addr;
			test_reg = *(mpl::wide_float*)(reg_a) == *(mpl::wide_float*)(reg_b);
			break;

		case swsl::FLT_EQ_MI:
			reg_a = stack + sptr - program[iptr++].u_addr;
			reg_b = &program[iptr++];
			test_reg = *(mpl::wide_float*)(reg_a) == to_wide_float(reg_b);
			break;

		case swsl::FLT_NEQ_MM:
			reg_a = stack + sptr - program[iptr++].u_addr;
			reg_b = stack + sptr - program[iptr++].u_addr;
			test_reg = *(mpl::wide_float*)(reg_a) != *(mpl::wide_float*)(reg_b);
			break;

		case swsl::FLT_NEQ_MI:
			reg_a = stack + sptr - program[iptr++].u_addr;
			reg_b = &program[iptr++];
			test_reg = *(mpl::wide_float*)(reg_a) != to_wide_float(reg_b);
			break;

		case swsl::FLT_LT_MM:
			reg_a = stack + sptr - program[iptr++].u_addr;
			reg_b = stack + sptr - program[iptr++].u_addr;
			test_reg = *(mpl::wide_float*)(reg_a) < *(mpl::wide_float*)(reg_b);
			break;

		case swsl::FLT_LT_MI:
			reg_a = stack + sptr - program[iptr++].u_addr;
			reg_b = &program[iptr++];
			test_reg = *(mpl::wide_float*)(reg_a) < to_wide_float(reg_b);
			break;

		case swsl::FLT_LTE_MM:
			reg_a = stack + sptr - program[iptr++].u_addr;
			reg_b = stack + sptr - program[iptr++].u_addr;
			test_reg = *(mpl::wide_float*)(reg_a) <= *(mpl::wide_float*)(reg_b);
			break;

		case swsl::FLT_LTE_MI:
			reg_a = stack + sptr - program[iptr++].u_addr;
			reg_b = &program[iptr++];
			test_reg = *(mpl::wide_float*)(reg_a) <= to_wide_float(reg_b);
			break;

		case swsl::FLT_GT_MM:
			reg_a = stack + sptr - program[iptr++].u_addr;
			reg_b = stack + sptr - program[iptr++].u_addr;
			test_reg = *(mpl::wide_float*)(reg_a) > *(mpl::wide_float*)(reg_b);
			break;

		case swsl::FLT_GT_MI:
			reg_a = stack + sptr - program[iptr++].u_addr;
			reg_b = &program[iptr++];
			test_reg = *(mpl::wide_float*)(reg_a) > to_wide_float(reg_b);
			break;

		case swsl::FLT_GTE_MM:
			reg_a = stack + sptr - program[iptr++].u_addr;
			reg_b = stack + sptr - program[iptr++].u_addr;
			test_reg = *(mpl::wide_float*)(reg_a) >= *(mpl::wide_float*)(reg_b);
			break;

		case swsl::FLT_GTE_MI:
			reg_a = stack + sptr - program[iptr++].u_addr;
			reg_b = &program[iptr++];
			test_reg = *(mpl::wide_float*)(reg_a) >= to_wide_float(reg_b);
			break;

		default: return false;
		}
	}

	return false;
}
