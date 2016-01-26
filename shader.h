#ifndef SHADER_H
#define SHADER_H

#include "MiniLib/MTL/mtlList.h"
#include "MiniLib/MTL/mtlString.h"

#include "swsl_wide.h"

class Compiler;
class Disassembler;

struct CompilerMessage
{
	mtlChars msg;
	mtlChars ref;

	CompilerMessage(const mtlChars &_msg, const mtlChars &_ref) : msg(_msg), ref(_ref) {}
};

class Shader
{
	friend class Compiler;
	friend class Disassembler;

public:
	struct InputArray
	{
		swsl::wide_float *data;
		int               count;
	};

	struct InputArrays
	{
		InputArray constant, varying, fragments;
	};

private:
	typedef unsigned char byte_t;
	static const byte_t STACK_SIZE_MASK = (byte_t)(-1);
	static const int    STACK_SIZE      = (int)STACK_SIZE_MASK + 1;

private:
	mtlString                 m_program;
	InputArrays              *m_inputs;
	mtlList<CompilerMessage>  m_errors;
	mtlList<CompilerMessage>  m_warnings;

public:
	Shader( void ) : m_inputs(NULL) {}

	void Delete( void );
	bool IsValid( void ) const;
	int GetErrorCount( void ) const;
	int GetWarningCount( void ) const;
	void SetInputArrays(InputArrays &inputs);
	const mtlItem<CompilerMessage> *GetErrors( void ) const;
	const mtlItem<CompilerMessage> *GetWarnings( void ) const;

	bool Run(const swsl::wide_cmpmask &frag_mask) const;
};

#endif // SHADER_H
