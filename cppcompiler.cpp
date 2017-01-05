#include "compiler.h"
#include "swsl_aux.h"

#define BLOCK 2048

void CppCompiler::InitializeCompilerState(swsl::Binary &output)
{
	m_indent = 0;
	m_buffer.Free();
	m_buffer.poolMemory = true;
	m_buffer.SetCapacity(BLOCK);
	PrintNL("#include \"swsl_types.h\"");
}

void CppCompiler::EmitPushScope( void )
{
	PrintIndent();
	PrintNL("{");
	++m_indent;
}

void CppCompiler::EmitPopScope( void )
{
	--m_indent;
	PrintIndent();
	PrintNL("}");
}

void CppCompiler::EmitElse( void )
{
	PrintIndent();
	PrintCurMask();
	Print("[0] = !");
	PrintCurMask();
	PrintNL("[0];");

	PrintIndent();
	Print("if ( !(");
	PrintCurMask();
	PrintNL("[0].all_fail()) )");
}

void CppCompiler::EmitIf(const mtlChars &condition)
{
	PrintIndent();
	EmitType("bool");
	Print(" ");
	PrintCurMask();
	PrintNL(";");

	PrintIndent();
	PrintCurMask();
	Print("[0] = ( ");
	Print(condition);
	Print(" ) & ");
	PrintPrevMask();
	PrintNL("[0];");

	PrintIndent();
	Print("if ( !(");
	PrintCurMask();
	PrintNL("[0].all_fail()) )");
}

void CppCompiler::EmitStatement(const mtlChars &statement)
{
	PrintIndent();
	mtlSyntaxParser p;
	mtlArray<mtlChars> m;
	p.SetBuffer(statement);
	switch (p.Match("%w=%S%0 %| %w%w=%S%0 %| %w%w%0 %| %S", m)) {
	case 0:
		// IMPROVEMENT: only requires merge if a mask has been declared after dst was declared
		Print("swsl::wide_vec_merge(");
		EmitDst(m[0]);
		Print(", ");
		EmitExpression(m[1]);
		Print(", ");
		PrintCurMask();
		Print("[0])");
		break;

	case 1:
		// Never requires a masked merge
		EmitDecl(m[0], m[1]);
		Print(" = ");
		EmitExpression(m[2]);
		break;

	case 2:
		EmitDecl(m[0], m[1]);
		break;

	case 3:
	default:
		Print(statement);
		break;
	}
	PrintNL(";");
}

void CppCompiler::EmitDst(const mtlChars &dst)
{
	EmitName(dst);
}

void CppCompiler::EmitType(const mtlChars &type)
{
	if (type.Compare(Keywords[Token_Bool], true)) {
		Print("swsl::wide_bool1");
	} else if (type.Compare(Keywords[Token_Int], true)) {
		Print("swsl::wide_int1");
	} else if (type.Compare(Keywords[Token_Int2], true)) {
		Print("swsl::wide_int2");
	} else if (type.Compare(Keywords[Token_Int3], true)) {
		Print("swsl::wide_int3");
	} else if (type.Compare(Keywords[Token_Int4], true)) {
		Print("swsl::wide_int4");
	} else if (type.Compare(Keywords[Token_Fixed], true)) {
		Print("swsl::wide_fixed1");
	} else if (type.Compare(Keywords[Token_Fixed2], true)) {
		Print("swsl::wide_fixed2");
	} else if (type.Compare(Keywords[Token_Fixed3], true)) {
		Print("swsl::wide_fixed3");
	} else if (type.Compare(Keywords[Token_Fixed4], true)) {
		Print("swsl::wide_fixed4");
	} else if (type.Compare(Keywords[Token_Float], true)) {
		Print("swsl::wide_float1");
	} else if (type.Compare(Keywords[Token_Float2], true)) {
		Print("swsl::wide_float2");
	} else if (type.Compare(Keywords[Token_Float3], true)) {
		Print("swsl::wide_float3");
	} else if (type.Compare(Keywords[Token_Float4], true)) {
		Print("swsl::wide_float4");
	} else {
		Print(type);
	}
}

void CppCompiler::EmitBaseType(const mtlChars &type)
{
	if (type.Compare(Keywords[Token_Bool], true)) {
		Print("mpl::wide_bool");
	} else if (type.Compare(Keywords[Token_Int], true)) {
		Print("mpl::wide_int");
	} else if (type.Compare(Keywords[Token_Int2], true)) {
		Print("mpl::wide_int");
	} else if (type.Compare(Keywords[Token_Int3], true)) {
		Print("mpl::wide_int");
	} else if (type.Compare(Keywords[Token_Int4], true)) {
		Print("mpl::wide_int");
	} else if (type.Compare(Keywords[Token_Fixed], true)) {
		Print("mpl::wide_fixed");
	} else if (type.Compare(Keywords[Token_Fixed2], true)) {
		Print("mpl::wide_fixed");
	} else if (type.Compare(Keywords[Token_Fixed3], true)) {
		Print("mpl::wide_fixed");
	} else if (type.Compare(Keywords[Token_Fixed4], true)) {
		Print("mpl::wide_fixed");
	} else if (type.Compare(Keywords[Token_Float], true)) {
		Print("mpl::wide_float");
	} else if (type.Compare(Keywords[Token_Float2], true)) {
		Print("mpl::wide_float");
	} else if (type.Compare(Keywords[Token_Float3], true)) {
		Print("mpl::wide_float");
	} else if (type.Compare(Keywords[Token_Float4], true)) {
		Print("mpl::wide_float");
	} else {
		Print(type);
	}
}

void CppCompiler::EmitName(const mtlChars &name)
{
	Print("_");
	Print(name);
}

void CppCompiler::EmitInternalName(const mtlChars &name)
{
	Print(name);
}

void CppCompiler::EmitDecl(const mtlChars &type, const mtlChars &name)
{
	EmitType(type);
	Print(" "); // add a character initially to avoid naming collisions with compiler-generated variables
	EmitName(name);
}

void CppCompiler::EmitExpression(const mtlChars &expr)
{
	Print(expr);
}

void CppCompiler::EmitFunctionSignature(const mtlChars &ret_type, const mtlChars &func_name, const mtlChars &params)
{
	PrintIndent();
	Print("inline ");
	EmitType(ret_type);
	Print(" ");
	PrintFunctionName(func_name);
	Print("(");
	mtlArray<mtlChars> m;
	mtlSyntaxParser p;
	p.SetBuffer(params);

	while (!p.IsEnd()) {
		switch (p.Match("%w%w, %| %w%w%0 %| %s", m)) {
		case 0:
			EmitDecl(m[0], m[1]);
			Print(", ");
			break;

		case 1:
			EmitDecl(m[0], m[1]);
			break;

		default:
			AddError("Unknown parameter parsing error", m[0]);
			break;
		}
	}
	Print(", ");
	EmitType("bool");
	Print(" ");
	PrintMask(0);
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

void CppCompiler::PrintMask(unsigned int mask_num)
{
	Print("m");
	mtlString num;
	num.FromInt(mask_num);
	Print(num);
}

void CppCompiler::PrintCurMask( void )
{
	PrintMask(GetMaskDepth());
}

void CppCompiler::PrintPrevMask( void )
{
	PrintMask(GetMaskDepth() - 1);
}

void CppCompiler::PrintFunctionName(const mtlChars &name)
{
	EmitInternalName(GetProgramName());
	EmitName(name);
}
