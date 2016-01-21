#ifndef SWSL_VMACH_H
#define SWSL_VMACH_H

#include "swsl_wide.h"
#include "swsl_compiler.h"
#include "swsl_instr.h"

#include "MiniLib/MTL/mtlArray.h"

namespace swsl
{

struct ShaderRegister
{
	swsl::wide_float *data;
	int               count;
};

struct ShaderInput
{
	ShaderRegister     constant;
	ShaderRegister     varying;
	ShaderRegister     fragments;
	//textures
};

class ShaderProgram
{
private:
	typedef unsigned char byte_t;
	static const byte_t DATA_CACHE_SIZE_MASK = (byte_t)(-1);
	static const int    DATA_CACHE_SIZE      = (int)DATA_CACHE_SIZE_MASK + 1;

private:
	swsl::wide_float         m_data_cache[DATA_CACHE_SIZE];
	mtlArray<swsl::instr_t>  m_instr_cache;
	swsl::ShaderInput       *m_input;
	int                      m_expected_input_count;

public:
	ShaderProgram( void );

	bool LoadProgram(const mtlArray<char> &program);
	void SetInputRegisters(swsl::ShaderInput &input);
	//void LoadTextures();

	bool Verify( void ) const;

	bool ExecProgram(const swsl::wide_cmpmask &fragment_mask);

	void UnloadProgram( void );
};

}

#endif // SWSL_VMACH_H
