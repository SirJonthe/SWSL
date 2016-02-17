#include "shader.h"
#include "instr.h"

#include "swsl_wide.h"

#include "MiniLib/MTL/mtlMemory.h"

// Points to an instruction in the instruction cache.
#define read_1_instr(reg) reg = &program[iptr++]
#define read_2_instr(reg_a, reg_b) read_1_instr(reg_a); read_1_instr(reg_b)

// Convert an instruction pointer to a value, or dereference the pointer to a value stored in the data cache.
#define to_addr(reg)             (*(addr_t*)(reg))
#define to_wide_float(reg)       (swsl::wide_float(*(float*)(reg)))
#define to_stack_float(reg)      (stack[to_addr(reg)])
#define to_stack_float_x(reg, x) (stack[(to_addr(reg) + (x))])

#define topstack_to_cmpmask     (*(swsl::wide_cmpmask*)&stack[sptr-1])
#define pushstack_cmpmask(mask) (*(swsl::wide_cmpmask*)&stack[sptr++] = (mask))

#define stack_to_mask(reg) (*(swsl::wide_cmpmask*)(reg))

void Shader::Delete( void )
{
	m_program.Free();
	m_errors.RemoveAll();
	m_warnings.RemoveAll();
}

bool Shader::IsValid( void ) const
{
	return m_program.GetSize() > 0 && m_errors.GetSize() == 0 && (m_inputs->constant.count + m_inputs->varying.count + m_inputs->fragments.count == m_program[gMetaData_InputIndex].u_addr);
}

int Shader::GetErrorCount( void ) const
{
	return m_errors.GetSize();
}

int Shader::GetWarningCount( void ) const
{
	return m_warnings.GetSize();
}

void Shader::SetInputArrays(InputArrays &inputs)
{
	m_inputs = &inputs;
}

const mtlItem<CompilerMessage> *Shader::GetErrors( void ) const
{
	return m_errors.GetFirst();
}

const mtlItem<CompilerMessage> *Shader::GetWarnings( void ) const
{
	return m_warnings.GetFirst();
}
#include <iostream>
bool Shader::Run(const swsl::wide_cmpmask &frag_mask) const
{
	swsl::wide_float stack[STACK_SIZE];

	addr_t iptr = m_program[gMetaData_EntryIndex].u_addr; // instruction pointer
	addr_t sptr = 0;                                      // stack pointer
	addr_t mptr = 0;                                      // mask pointer

	const Instruction *program      = (const Instruction*)(&m_program[0]);
	const addr_t       program_size = (addr_t)m_program.GetSize();

	void       *reg_a;
	const void *reg_b;

	// Transfer all data to local memory addressable by VM
	int frag_offset = m_inputs->varying.count + m_inputs->constant.count;
	mtlCopy(stack, m_inputs->constant.data, m_inputs->constant.count);
	mtlCopy(stack + m_inputs->constant.count, m_inputs->varying.data, m_inputs->varying.count);
	mtlCopy(stack + frag_offset, m_inputs->fragments.data, m_inputs->fragments.count);

	while (iptr < program_size) {

		switch (program[iptr++].instr) {

		case NOP:
			continue;

		case END: {
			// Sync up fragment data to output
			swsl::wide_float *fragment_data = stack + frag_offset;
			for (int i = 0; i < m_inputs->fragments.count; ++i) {
				m_inputs->fragments.data[i] = swsl::wide_float::merge(fragment_data[i], m_inputs->fragments.data[i], frag_mask);
			}
			return true;
		}

		case FPUSH_M:
			stack[sptr] = stack[sptr - program[iptr++].u_addr];
			++sptr;
			break;

		case FPUSH_I:
			stack[sptr++] = to_wide_float(program + iptr);
			++iptr;
			break;

		case UPUSH_I:
			sptr += program[iptr++].u_addr;
			break;

		case FPOP_M: {
			addr_t addr = sptr - program[iptr++].u_addr;
			stack[addr] = stack[--sptr];
			break;
		}

		case UPOP_I:
			sptr -= program[iptr++].u_addr;
			break;

		case UJMP_I:
			iptr = program[iptr].u_addr;
			++iptr;
			break;

		case FSET_MM:
			reg_a = stack + sptr - program[iptr++].u_addr;
			reg_b = stack + sptr - program[iptr++].u_addr;
			*(swsl::wide_float*)(reg_a) = swsl::wide_float::merge((*(swsl::wide_float*)(reg_b)), *(swsl::wide_float*)(reg_a), stack_to_mask(stack + mptr));
			break;

		case FSET_MI:
			reg_a = stack + sptr - program[iptr++].u_addr;
			reg_b = &program[iptr++];
			*(swsl::wide_float*)(reg_a) = swsl::wide_float::merge(to_wide_float(reg_b), *(swsl::wide_float*)(reg_a), stack_to_mask(stack + mptr));
			break;

		case FADD_MM:
			reg_a = stack + sptr - program[iptr++].u_addr;
			reg_b = stack + sptr - program[iptr++].u_addr;
			*(swsl::wide_float*)(reg_a) += *(swsl::wide_float*)(reg_b);
			break;

		case FADD_MI:
			reg_a = stack + sptr - program[iptr++].u_addr;
			reg_b = &program[iptr++];
			*(swsl::wide_float*)(reg_a) += to_wide_float(reg_b);
			break;

		case FSUB_MM:
			reg_a = stack + sptr - program[iptr++].u_addr;
			reg_b = stack + sptr - program[iptr++].u_addr;
			*(swsl::wide_float*)(reg_a) -= *(swsl::wide_float*)(reg_b);
			break;

		case FSUB_MI:
			reg_a = stack + sptr - program[iptr++].u_addr;
			reg_b = &program[iptr++];
			*(swsl::wide_float*)(reg_a) -= to_wide_float(reg_b);
			break;

		case FMUL_MM:
			reg_a = stack + sptr - program[iptr++].u_addr;
			reg_b = stack + sptr - program[iptr++].u_addr;
			*(swsl::wide_float*)(reg_a) *= *(swsl::wide_float*)(reg_b);
			break;

		case FMUL_MI:
			reg_a = stack + sptr - program[iptr++].u_addr;
			reg_b = &program[iptr++];
			*(swsl::wide_float*)(reg_a) *= to_wide_float(reg_b);
			break;

		case FDIV_MM:
			reg_a = stack + sptr - program[iptr++].u_addr;
			reg_b = stack + sptr - program[iptr++].u_addr;
			*(swsl::wide_float*)(reg_a) /= *(swsl::wide_float*)(reg_b);
			break;

		case FDIV_MI:
			reg_a = stack + sptr - program[iptr++].u_addr;
			reg_b = &program[iptr++];
			*(swsl::wide_float*)(reg_a) /= to_wide_float(reg_b);
			break;

		/*case MIN_FF:
			read_2_instr(reg_a, reg_b);
			to_ref_float(reg_a) = swsl::wide_float::min(to_ref_float(reg_a), to_ref_float(reg_b));
			break;

		case MIN_FC:
			read_2_instr(reg_a, reg_b);
			to_ref_float(reg_a) = swsl::wide_float::min(to_ref_float(reg_a), to_const_float(reg_b));
			break;

		case MAX_FF:
			read_2_instr(reg_a, reg_b);
			to_ref_float(reg_a) = swsl::wide_float::max(to_ref_float(reg_a), to_ref_float(reg_b));
			break;

		case MAX_FC:
			read_2_instr(reg_a, reg_b);
			to_ref_float(reg_a) = swsl::wide_float::max(to_ref_float(reg_a), to_const_float(reg_b));
			break;

		case SQRT_FF:
			read_2_instr(reg_a, reg_b);
			to_ref_float(reg_a) = swsl::wide_float::sqrt(to_ref_float(reg_b));
			break;

		case SQRT_FC:
			read_2_instr(reg_a, reg_b);
			to_ref_float(reg_a) = swsl::wide_float::sqrt(to_const_float(reg_b));
			break;

		case EQ_FF:
			read_2_instr(reg_a, reg_b);
			pushstack_cmpmask(to_ref_float(reg_a) == to_ref_float(reg_b));
			break;

		case EQ_FC:
			read_2_instr(reg_a, reg_b);
			pushstack_cmpmask(to_ref_float(reg_a) == to_const_float(reg_b));
			break;

		case EQ_CF:
			read_2_instr(reg_a, reg_b);
			pushstack_cmpmask(to_const_float(reg_a) == to_ref_float(reg_b));
			break;

		case NEQ_FF:
			read_2_instr(reg_a, reg_b);
			pushstack_cmpmask(to_ref_float(reg_a) != to_ref_float(reg_b));
			break;

		case NEQ_FC:
			read_2_instr(reg_a, reg_b);
			pushstack_cmpmask(to_ref_float(reg_a) != to_const_float(reg_b));
			break;

		case NEQ_CF:
			read_2_instr(reg_a, reg_b);
			pushstack_cmpmask(to_const_float(reg_a) != to_ref_float(reg_b));
			break;

		case LT_FF:
			read_2_instr(reg_a, reg_b);
			pushstack_cmpmask(to_ref_float(reg_a) < to_ref_float(reg_b));
			break;

		case LT_FC:
			read_2_instr(reg_a, reg_b);
			pushstack_cmpmask(to_ref_float(reg_a) < to_const_float(reg_b));
			break;

		case LT_CF:
			read_2_instr(reg_a, reg_b);
			pushstack_cmpmask(to_const_float(reg_a) < to_ref_float(reg_b));
			break;

		case LE_FF:
			read_2_instr(reg_a, reg_b);
			pushstack_cmpmask(to_ref_float(reg_a) <= to_ref_float(reg_b));
			break;

		case LE_FC:
			read_2_instr(reg_a, reg_b);
			pushstack_cmpmask(to_ref_float(reg_a) <= to_const_float(reg_b));
			break;

		case LE_CF:
			read_2_instr(reg_a, reg_b);
			pushstack_cmpmask(to_const_float(reg_a) <= to_ref_float(reg_b));
			break;

		case GT_FF:
			read_2_instr(reg_a, reg_b);
			pushstack_cmpmask(to_ref_float(reg_a) > to_ref_float(reg_b));
			break;

		case GT_FC:
			read_2_instr(reg_a, reg_b);
			pushstack_cmpmask(to_ref_float(reg_a) > to_const_float(reg_b));
			break;

		case GT_CF:
			read_2_instr(reg_a, reg_b);
			pushstack_cmpmask(to_const_float(reg_a) > to_ref_float(reg_b));
			break;

		case GE_FF:
			read_2_instr(reg_a, reg_b);
			pushstack_cmpmask(to_ref_float(reg_a) >= to_ref_float(reg_b));
			break;

		case GE_FC:
			read_2_instr(reg_a, reg_b);
			pushstack_cmpmask(to_ref_float(reg_a) >= to_const_float(reg_b));
			break;

		case GE_CF:
			read_2_instr(reg_a, reg_b);
			pushstack_cmpmask(to_const_float(reg_a) >= to_ref_float(reg_b));
			break;

		case MRG:
			read_2_instr(reg_a, reg_b);
			to_ref_float(reg_a) = swsl::wide_float::merge(to_ref_float(reg_a), to_ref_float(reg_b), topstack_to_cmpmask);
			break;*/

		default: return false;
		}
	}

	return false;
}
