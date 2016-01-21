#include <cstdlib>

#include "swsl_vmach.h"

// Points to an instruction in the instruction cache.
#define read_1_instr(reg) reg = &m_instr_cache[instr_ptr++]
#define read_2_instr(reg_a, reg_b) read_1_instr(reg_a); read_1_instr(reg_b)
#define read_3_instr(reg_a, reg_b, reg_c) read_2_instr(reg_a, reg_b); read_1_instr(reg_c)

// Convert an instruction pointer to a value, or dereference the pointer to a value stored in the data cache.
#define to_const_instr(reg)    (*(swsl::instr_t*)(reg))
#define to_const_float(reg)    (swsl::wide_float(*(float*)&to_const_instr(reg)))
#define to_ref_float(reg)      (m_data_cache[to_const_instr(reg) & DATA_CACHE_SIZE_MASK])
#define to_ref_float_x(reg, x) (m_data_cache[(to_const_instr(reg) + (x)) & DATA_CACHE_SIZE_MASK])

#define topstack_to_cmpmask     (*(swsl::wide_cmpmask*)&m_data_cache[data_ptr-1])
#define pushstack_cmpmask(mask) (*(swsl::wide_cmpmask*)&m_data_cache[data_ptr++] = (mask))

#define word_size 1
#define vec2_size 2
#define vec3_size 3
#define vec4_size 4

swsl::ShaderProgram::ShaderProgram( void ) : m_instr_cache(), m_input(NULL), m_expected_input_count(0)
{}

bool swsl::ShaderProgram::LoadProgram(const mtlArray<char> &program)
{
	UnloadProgram();

	int data_size = program.GetSize() / sizeof(swsl::instr_t);
	if (data_size > 0) {
		swsl::instr_t *data = (swsl::instr_t*)&program[0];
		m_expected_input_count = (int)data[0]; // the number of wide_floats that are set as input parameters to main()
		data++;
		m_instr_cache.Create(data_size - 1);
		for (int i = 0; i < m_instr_cache.GetSize(); ++i) {
			m_instr_cache[i] = data[i];
		}
		return true;
	}
	return false;
}

void swsl::ShaderProgram::SetInputRegisters(swsl::ShaderInput &input)
{
	m_input = &input;
}

bool swsl::ShaderProgram::Verify( void ) const
{
	return (m_instr_cache.GetSize() > 0) && (m_input != NULL) && (m_input->constant.count + m_input->varying.count + m_input->fragments.count == m_expected_input_count);
}

bool swsl::ShaderProgram::ExecProgram(const swsl::wide_cmpmask &frag_mask)
{
	swsl::instr_t instr_ptr = 0;
	byte_t        data_ptr  = m_expected_input_count;

	void *reg_a;
	void *reg_b;

	// Transfer all data to local memory addressable by VM
	int frag_offset = m_input->varying.count + m_input->constant.count;
	mtlCopy(m_data_cache, m_input->constant.data, m_input->constant.count);
	mtlCopy(m_data_cache + m_input->constant.count, m_input->varying.data, m_input->varying.count);
	mtlCopy(m_data_cache + frag_offset, m_input->fragments.data, m_input->fragments.count);

	while (instr_ptr < (instr_t)m_instr_cache.GetSize()) {

		switch ((swsl::InstructionSet)m_instr_cache[instr_ptr++]) {

		case NOP:
			continue;

		case END: {
			// Sync up fragment data to output
			swsl::wide_float *fragment_data = m_data_cache + frag_offset;
			for (int i = 0; i < m_input->fragments.count; ++i) {
				m_input->fragments.data[i] = swsl::wide_float::merge(fragment_data[i], m_input->fragments.data[i], frag_mask);
			}
			return true;
		}

		case JMP:
			read_1_instr(reg_a);
			instr_ptr = to_const_instr(reg_a);
			break;

		case POP:
			read_1_instr(reg_a);
			data_ptr -= to_const_instr(reg_a);
			break;

		case PUSH:
			read_1_instr(reg_a);
			data_ptr += to_const_instr(reg_a);
			break;

		case MOV_FF:
			read_2_instr(reg_a, reg_b);
			to_ref_float(reg_a) = to_ref_float(reg_b);
			break;

		case MOV_FC:
			read_2_instr(reg_a, reg_b);
			to_ref_float(reg_a) = to_const_float(reg_b);
			break;

		case ADD_FF:
			read_2_instr(reg_a, reg_b);
			to_ref_float(reg_a) += to_ref_float(reg_b);
			break;

		case ADD_FC:
			read_2_instr(reg_a, reg_b);
			to_ref_float(reg_a) += to_const_float(reg_b);
			break;

		case SUB_FF:
			read_2_instr(reg_a, reg_b);
			to_ref_float(reg_a) -= to_ref_float(reg_b);
			break;

		case SUB_FC:
			read_2_instr(reg_a, reg_b);
			to_ref_float(reg_a) -= to_const_float(reg_b);
			break;

		case MUL_FF:
			read_2_instr(reg_a, reg_b);
			to_ref_float(reg_a) *= to_ref_float(reg_b);
			break;

		case MUL_FC:
			read_2_instr(reg_a, reg_b);
			to_ref_float(reg_a) *= to_const_float(reg_b);
			break;

		case DIV_FF:
			read_2_instr(reg_a, reg_b);
			to_ref_float(reg_a) /= to_ref_float(reg_b);
			break;

		case DIV_FC:
			read_2_instr(reg_a, reg_b);
			to_ref_float(reg_a) /= to_const_float(reg_b);
			break;

		case MIN_FF:
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
			break;

		default: return false;
		}
	}

	return false;
}

void swsl::ShaderProgram::UnloadProgram( void )
{
	m_instr_cache.Free();
	m_expected_input_count = 0;
}
