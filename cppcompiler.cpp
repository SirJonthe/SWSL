#include "compiler.h"

void CppCompiler::PushScope( void )
{
	// {
	// ++indent;
}

void CppCompiler::PopScope( void )
{
	// }
	// --indent;
}

void CppCompiler::EmitElse( void )
{
	// for indent TAB
	// else
}

void CppCompiler::EmitIf(const mtlChars &condition)
{
	// for indent TAB
	// if (condition)
}

void CppCompiler::EmitStatement(const mtlChars &statement)
{
	// for indent TAB
	// statement;
}

void CppCompiler::EmitFunctionSignature(const mtlChars &ret_type, const mtlChars &func_name, const mtlChars &params)
{
	// ret_type func_type(params)
}

void CppCompiler::ProgramErrorCheck( void ) {}

void CppCompiler::ConvertToOutput(swsl::Binary &output)
{
	//output.Copy(internal_state);
}
