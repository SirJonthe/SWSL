#include "compiler.h"
#include "swsl_aux.h"

#define BLOCK 2048

void CppCompiler::InitializeCompilerState(swsl::Binary &output)
{
	m_indent = 0;
	m_buffer.Free();
	m_buffer.poolMemory = true;
	m_buffer.SetCapacity(BLOCK);
	PrintNL("#include \"swsl_types.h\""); // TODO: make this path relative to output path
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
	Print(" = !");
	PrintCurMask();
	PrintNL(";");

	PrintIndent();
	Print("if ( !(");
	PrintCurMask();
	PrintNL(".all_fail()) )");
}

void CppCompiler::EmitIf(const mtlChars &condition)
{
	PrintIndent();
	EmitBaseType("bool");
	Print(" ");
	PrintCurMask();
	Print(" = ( ");
	EmitExpression(condition);
	Print(" ) & ");
	PrintPrevMask();
	PrintNL(";");

	PrintIndent();
	Print("if ( !(");
	PrintCurMask();
	PrintNL(".all_fail()) )");
}

void CppCompiler::EmitStatement(const mtlChars &statement)
{
	PrintIndent();
	mtlSyntaxParser p;
	mtlArray<mtlChars> m;
	p.SetBuffer(statement);
	switch (p.Match("%w=%S%0 %| %w%w=%S%0 %| %w%w%0 %| %S", m)) {
	case 0:
		// TODO: only requires merge if a mask has been declared after dst was declared
		Print("swsl::set(");
		EmitDst(m[0]);
		Print(", ");
		EmitExpression(m[1]);
		Print(", ");
		PrintCurMask();
		Print(")");
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

bool CppCompiler::IsReservedName(const mtlChars &name) const
{
	return name.Compare(Keywords[Token_Bool],   true) ||
		   name.Compare(Keywords[Token_Int],    true) ||
		   name.Compare(Keywords[Token_Int2],   true) ||
		   name.Compare(Keywords[Token_Int3],   true) ||
		   name.Compare(Keywords[Token_Int4],   true) ||
		   name.Compare(Keywords[Token_Fixed],  true) ||
		   name.Compare(Keywords[Token_Fixed2], true) ||
		   name.Compare(Keywords[Token_Fixed3], true) ||
		   name.Compare(Keywords[Token_Fixed4], true) ||
		   name.Compare(Keywords[Token_Float],  true) ||
		   name.Compare(Keywords[Token_Float2], true) ||
		   name.Compare(Keywords[Token_Float3], true) ||
		   name.Compare(Keywords[Token_Float4], true) ||
		   name.Compare("max",                  true) ||
		   name.Compare("min",                  true) ||
		   name.Compare("abs",                  true) ||
		   name.Compare("sqrt",                 true) ||
		   name.Compare("ceil",                 true) ||
		   name.Compare("floor",                true) ||
		   name.Compare("round",                true) ||
		   name.Compare("trunc",                true) ||
		   name.Compare("sin",                  true) ||
		   name.Compare("cos",                  true) ||
		   name.Compare("tan",                  true) ||
		   name.Compare("asin",                 true) ||
		   name.Compare("acos",                 true) ||
		   name.Compare("atan",                 true);
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
	// NOTE: no need to optimize constant parts of the expression as the C++ compiler will do this for us
	// functions: always add last mask as final argument in call

	mtlSyntaxParser p;
	p.SetBuffer(expr);
	mtlArray<mtlChars> m;
	mtlChars s;

	while (!p.IsEnd()) {
		switch (p.Match("(%S) %| + %| - %| * %| / %| && %| || %| == %| != %| < %| <= %| > %| >= %| %r %| . %| %w(%s) %| %w %| %s", m, &s)) {
		case 0:
			Print("(");
			EmitExpression(m[0]);
			Print(")");
			break;

		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
		case 11:
		case 12:
			Print(" ");
			Print(s);
			Print(" ");
			break;

		case 13:
			Print(s);
			if (s.FindFirstChar('.') == -1) {
				Print(".0");
			}
			Print("f");
			p.Match("f");
			break;

		case 14:
			if (p.Match("%w", m, &s) == 0) {
				const mtlChars mems = "xyzw";
				Print("[");
				for (int i = 0; i < m[0].GetSize(); ++i) {
					for (int j = 0; j < mems.GetSize(); ++j) {
						if (m[0][i] == mems[j]) {
							mtlString num;
							num.FromInt(j);
							Print(num);
						}
					}
					if (i < m[0].GetSize() - 1) {
						Print(", ");
					}
				}
				Print("]");
			} else {
				AddError("Unable to scan members", s);
				return;
			}
			break;

		case 15:
			EmitFunctionCall(m[0], m[1]);
			break;

		case 16:
			EmitName(m[0]);
			break;

		default:
			AddError("Unable to scan expression", s);
			return;
		}
	}
}

static const mtlChars swsl_type_decl_str = "%?(const %| mutable) %w%?(&)%w %| %s";

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
		switch (p.Match(swsl_type_decl_str, m)) {
		case 0:
			EmitDecl(m[1], m[3]);
			if (p.Match(",") == 0) { Print(", "); }
			break;

		default:
			AddError("Unknown parameter parsing error (1)", m[0]);
			return;
		}
	}
	Print(", const ");
	EmitBaseType("bool");
	Print(" &");
	PrintMask(0);
	PrintNL(")");
}

void CppCompiler::EmitCompatibilityMain(const mtlChars &ret_type, const mtlChars &func_name, const mtlChars &params)
{
	PrintIndent();
	Print("inline ");
	EmitType(ret_type);
	Print(" ");
	PrintFunctionName(func_name);
	Print("(void *params, const ");
	EmitBaseType("bool");
	Print(" &");
	PrintCurMask();
	PrintNL(")");

	EmitPushScope();
	PrintIndent();

	if (!ret_type.Compare(Keywords[Token_Void])) {
		Print("return ");
	}

	PrintFunctionName(func_name);
	PrintNL("(");

	mtlSyntaxParser p;
	mtlArray<mtlChars> m;
	p.SetBuffer(params);
	mtlString po_str;

	++m_indent;

	while (!p.IsEnd()) {

		PrintIndent();

		switch (p.Match(swsl_type_decl_str, m)) {
		case 0:
		{
			//( (type&)(((char*)params)[sum]) )
			Print("( (");
			EmitType(m[1]);
			Print("&)((char*)params)[");
			if (po_str.GetSize() > 0) {
				mtlSyntaxParser p0;
				mtlArray<mtlChars> m0;
				p0.SetBuffer(po_str);
				while (!p0.IsEnd() && p0.Match("%w", m0) == 0) {
					Print("sizeof(");
					EmitType(m0[0]);
					Print(")");
					if (!p0.IsEnd()) {
						Print(" + ");
					}
				}
			} else {
				Print("0");
			}
			Print("] )");
			po_str.Append(m[1]);
			po_str.Append(" ");
			break;
		}

		default:
			AddError("Unknown parameter parsing error (2)", m[0]);
			break;
		}
		p.Match(",");
		PrintNL(", ");
	}

	PrintIndent();
	PrintCurMask();
	PrintNL("");

	--m_indent;

	PrintIndent();
	PrintNL(");");

	EmitPopScope();
}

void CppCompiler::DeclareFunction(const mtlChars &ret_type, const mtlChars &func_name, const mtlChars &params)
{
	EmitFunctionSignature(ret_type, func_name, params);
	PrintNL(";");
}

void CppCompiler::EmitFunctionCall(const mtlChars &name, const mtlChars &params)
{
	mtlSyntaxParser p;
	p.SetBuffer(params);
	mtlArray<mtlChars> m;

	if (IsReservedName(name)) {
		Print("swsl::wide_");
		EmitInternalName(name);
	} else {
		PrintFunctionName(name);
	}
	Print("(");
	while (!p.IsEnd()) {
		switch (p.Match("%s, %| %s", m)) {
		case 0:
		case 1:
			EmitExpression(m[0]);
			break;
		}
		Print(", ");
	}
	PrintCurMask();
	Print(")");
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
