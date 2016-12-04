#ifndef SWSL_SHADER_H
#define SWSL_SHADER_H

#include "MiniLib/MPL/mplWide.h"
#include "MiniLib/MTL/mtlList.h"
#include "MiniLib/MTL/mtlString.h"
#include "MiniLib/MTL/mtlArray.h"

#include "swsl_instr.h"

class Compiler;
class CppCompiler;
class ByteCodeCompiler;

namespace swsl
{

	typedef mtlString Binary;

	class Compiler;
	class Disassembler;

	struct CompilerMessage
	{
		mtlString msg;
		mtlString ref;
	};

	class Shader
	{
		friend class swsl::Compiler;
		friend class swsl::Disassembler;
		friend class ::Compiler;
		friend class ::CppCompiler;
		friend class ::ByteCodeCompiler;

	public:
		struct InputArray
		{
			mpl::wide_float *data;
			int              count;
		};

		struct InputArrays
		{
			InputArray constant, varying, fragments;
			// change to:
			// InputArray in, inout;
		};

	private:
		enum MetaData
		{
			NumInputs,
			EntryIndex
		};

	private:
		static const addr_t STACK_SIZE_MASK = (addr_t)(-1);
		static const int    STACK_SIZE      = ((int)STACK_SIZE_MASK) + 1;

	private:
		mtlArray<Instruction>     m_program;
		InputArrays              *m_inputs;
		mtlList<CompilerMessage>  m_errors;
		mtlList<CompilerMessage>  m_warnings;

	public:
		Shader( void ) : m_inputs(NULL) {}

		void                            Delete( void );
		bool                            IsValid( void ) const;
		int                             GetErrorCount( void ) const;
		int                             GetWarningCount( void ) const;
		void                            SetInputArrays(InputArrays &inputs);
		const mtlItem<CompilerMessage> *GetErrors( void ) const;
		const mtlItem<CompilerMessage> *GetWarnings( void ) const;
		bool                            Run(const mpl::wide_bool &frag_mask) const;
	};

}

#endif // SHADER_H
