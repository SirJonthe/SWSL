#include "compiler.h"

#define BLOCK 2048

void CppCompiler::InitializeCompilerState( void )
{
	Compiler::InitializeCompilerState();
	m_indent = 0;
	m_buffer.Free();
	m_buffer.poolMemory = true;
	m_buffer.SetCapacity(BLOCK);
}

void CppCompiler::PushScope( void )
{
	PrintIndent();
	PrintNL("{");
	++m_indent;
}

void CppCompiler::PopScope( void )
{
	--m_indent;
	PrintIndent();
	PrintNL("}");
}

void CppCompiler::EmitElse( void )
{
	PrintIndent();
	PrintNL("else");
}

void CppCompiler::EmitIf(const mtlChars &condition)
{
	PrintIndent();
	Print("if (");
	Print(condition);
	PrintNL(")");
}

void CppCompiler::EmitStatement(const mtlChars &statement)
{
	PrintIndent();
	Print(statement);
	PrintNL(";");
}

void CppCompiler::EmitFunctionSignature(const mtlChars &ret_type, const mtlChars &func_name, const mtlChars &params)
{
	PrintIndent();
	Print(ret_type);
	Print(" ");
	Print(func_name);
	Print("(");
	Print(params);
	PrintNL(")");
}

void CppCompiler::ProgramErrorCheck( void ) {}

void CppCompiler::ConvertToOutput(swsl::Binary &output)
{
	output.SetSize(m_buffer.GetSize());
	for (int i = 0; i < output.GetSize(); ++i) {
		output[i] = m_buffer[i];
	}
}

void CppCompiler::Print(const mtlChars &ch)
{
	while (m_buffer.GetSize() + ch.GetSize() > m_buffer.GetCapacity()) {
		m_buffer.SetCapacity(m_buffer.GetCapacity() + BLOCK);
	}
	for (int i = 0; i < ch.GetSize(); ++i) {
		m_buffer.Add(ch[i]);
	}
}

void CppCompiler::PrintNL(const mtlChars &ch)
{
	Print(ch);
	Print("\n");
}

void CppCompiler::PrintIndent( void )
{
	for (int i = 0; i < m_indent; ++i) {
		Print("\t");
	}
}
