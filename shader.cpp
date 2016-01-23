#include "shader.h"
#include "instr.h"

#include "swsl_wide.h"

#include "MiniLib/MTL/mtlMemory.h"

// Points to an instruction in the instruction cache.
#define read_1_instr(reg) reg = &program[iptr++]
#define read_2_instr(reg_a, reg_b) read_1_instr(reg_a); read_1_instr(reg_b)
#define read_3_instr(reg_a, reg_b, reg_c) read_2_instr(reg_a, reg_b); read_1_instr(reg_c)

#define fread_m(a) a = &program[iptr++]
#define fread_i(a) a = &program[iptr]; iptr += sizeof(float);
#define fread_mm(a, b) fread_m(a); fread_m(b)
#define fread_mi(a, b) fread_m(a); fread_i(b)

// Convert an instruction pointer to a value, or dereference the pointer to a value stored in the data cache.
#define to_const_instr(reg)    (*(byte_t*)(reg))
#define to_const_float(reg)    (swsl::wide_float(*(float*)(reg)))
#define to_ref_float(reg)      (stack[to_const_instr(reg) & STACK_SIZE_MASK])
#define to_ref_float_x(reg, x) (stack[(to_const_instr(reg) + (x)) & STACK_SIZE_MASK])

#define topstack_to_cmpmask     (*(swsl::wide_cmpmask*)&stack[sptr-1])
#define pushstack_cmpmask(mask) (*(swsl::wide_cmpmask*)&stack[sptr++] = (mask))

#define word_size 1
#define vec2_size 2
#define vec3_size 3
#define vec4_size 4

void Shader::Delete( void )
{
	m_program.Free();
	m_errors.RemoveAll();
	m_warnings.RemoveAll();
}

bool Shader::IsValid( void ) const
{
	return m_program.GetSize() > 0 && m_errors.GetSize() == 0 && (m_inputs->constant.count + m_inputs->varying.count + m_inputs->fragments.count == (byte_t)m_program[UINPUT_I]);
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

bool Shader::Run(const swsl::wide_cmpmask &frag_mask) const
{
	swsl::wide_float stack[STACK_SIZE];

	byte_t iptr = 0;
	byte_t sptr  = (byte_t)m_program.GetChars()[UINPUT_I];

	const byte_t *program = (const byte_t*)m_program.GetChars();
	const byte_t program_size = (byte_t)m_program.GetSize();

	void       *reg_a;
	const void *reg_b;

	// Transfer all data to local memory addressable by VM
	int frag_offset = m_inputs->varying.count + m_inputs->constant.count;
	mtlCopy(stack, m_inputs->constant.data, m_inputs->constant.count);
	mtlCopy(stack + m_inputs->constant.count, m_inputs->varying.data, m_inputs->varying.count);
	mtlCopy(stack + frag_offset, m_inputs->fragments.data, m_inputs->fragments.count);

	while (iptr < program_size) {

		switch ((Instruction)program[iptr++]) {

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
			stack[sptr++] = stack[sptr - program[iptr++]];
			break;

		case FPUSH_I:
			stack[sptr++] = to_const_float(program[iptr]);
			iptr += sizeof(float);
			break;

		case UPUSH_I:
			sptr += to_const_instr(program[iptr++]);
			break;

		case FPOP_M:
		{
			byte_t addr = sptr - program[iptr++];
			stack[addr] = stack[--sptr];
			break;
		}

		case UPOP_I:
			sptr -= program[iptr++];
			break;

		case UJMP_I:
			iptr = (byte_t)program[iptr];
			++iptr;
			break;

		case FSET_MM:
			reg_a = stack + sptr - program[iptr++];
			reg_b = stack + sptr - program[iptr++];
			to_ref_float(reg_a) = to_ref_float(reg_b);
			break;

		case FSET_MI:
			reg_a = stack + sptr - program[iptr++];
			reg_b = &program[iptr];
			iptr += sizeof(float);
			to_ref_float(reg_a) = to_const_float(reg_b);
			break;

		case FADD_MM:
			reg_a = stack + sptr - program[iptr++];
			reg_b = stack + sptr - program[iptr++];
			to_ref_float(reg_a) += to_ref_float(reg_b);
			break;

		case FADD_MI:
			reg_a = stack + sptr - program[iptr++];
			reg_b = &program[iptr];
			iptr += sizeof(float);
			to_ref_float(reg_a) += to_const_float(reg_b);
			break;

		case FSUB_MM:
			reg_a = stack + sptr - program[iptr++];
			reg_b = stack + sptr - program[iptr++];
			to_ref_float(reg_a) -= to_ref_float(reg_b);
			break;

		case FSUB_MI:
			reg_a = stack + sptr - program[iptr++];
			reg_b = &program[iptr];
			iptr += sizeof(float);
			to_ref_float(reg_a) -= to_const_float(reg_b);
			break;

		case FMUL_MM:
			reg_a = stack + sptr - program[iptr++];
			reg_b = stack + sptr - program[iptr++];
			to_ref_float(reg_a) *= to_ref_float(reg_b);
			break;

		case FMUL_MI:
			reg_a = stack + sptr - program[iptr++];
			reg_b = &program[iptr];
			iptr += sizeof(float);
			to_ref_float(reg_a) *= to_const_float(reg_b);
			break;

		case FDIV_MM:
			reg_a = stack + sptr - program[iptr++];
			reg_b = stack + sptr - program[iptr++];
			to_ref_float(reg_a) /= to_ref_float(reg_b);
			break;

		case FDIV_MI:
			reg_a = stack + sptr - program[iptr++];
			reg_b = &program[iptr];
			iptr += sizeof(float);
			to_ref_float(reg_a) /= to_const_float(reg_b);
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
