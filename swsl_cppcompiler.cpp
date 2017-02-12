#include "swsl_cppcompiler.h"

void swsl::CppCompiler::PrintTabs( void )
{
	for (int i = 0; i < m_depth - 1; ++i) {
		m_buffer.Add('\t');
	}
}

void swsl::CppCompiler::Print(const mtlChars &ch)
{
	for (int i = 0; i < ch.GetSize(); ++i) {
		m_buffer.Add(ch[i]);
	}
}

void swsl::CppCompiler::PrintNewline( void )
{
	m_buffer.Add('\n');
}

void swsl::CppCompiler::PrintMask( void )
{
	m_buffer.Add('m');
	mtlString num;
	num.FromInt(m_cond_depth);
	Print(num);
}

void swsl::CppCompiler::OutputBinary(swsl::Binary &bin)
{
	if (m_errs == 0) {
		bin.SetSize(m_buffer.GetSize());
		for (int i = 0; i < bin.GetSize(); ++i) {
			bin[i] = m_buffer[i];
		}
	} else {
		bin.Free();
	}
}

void swsl::CppCompiler::PrintType(const mtlChars &type)
{
	if (type.Compare("void", true)) {
		Print("void");
	} else if (type.Compare("bool", true)) {
		Print("mpl::wide_bool");
	} else if (type.Compare("int", true)) {
		Print("swsl::wide_int1");
	} else if (type.Compare("int2", true)) {
		Print("swsl::wide_int2");
	} else if (type.Compare("int3", true)) {
		Print("swsl::wide_int3");
	} else if (type.Compare("int4", true)) {
		Print("swsl::wide_int4");
	} else if (type.Compare("float", true)) {
		Print("swsl::wide_float1");
	} else if (type.Compare("float2", true)) {
		Print("swsl::wide_float2");
	} else if (type.Compare("float3", true)) {
		Print("swsl::wide_float3");
	} else if (type.Compare("float4", true)) {
		Print("swsl::wide_float4");
	} else {
		Print(m_bin_name);
		Print("_");
		Print(type);
	}
}

void swsl::CppCompiler::DispatchBody(const Token_Body *t)
{
	if (m_depth > 0) {
		PrintTabs();
		Print("{");
		PrintNewline();
	}
	++m_depth;

	Dispatch(t->tokens);

	--m_depth;
	if (m_depth > 0) {
		PrintTabs();
		Print("}");
	}
	PrintNewline();
}

void swsl::CppCompiler::DispatchCallFn(const Token_CallFn *t)
{
	PrintTabs();
	Print(t->fn_name);
	Print("(");
	const mtlItem<swsl::Token*> *i = t->input.GetFirst();
	while (i != NULL) {
		Dispatch(i->GetItem());
		Print(", ");
		i = i->GetNext();
	}
	PrintMask();
	Print(")");
}

void swsl::CppCompiler::DispatchDeclFn(const Token_DeclFn *t)
{
	Dispatch(t->ret);
	Print("(");
	Dispatch(t->params);
	Print("const ");
	PrintType("bool");
	Print(" &");
	PrintMask();
	Print(")");
	if (t->parent != NULL && t->parent->type != swsl::Token::TOKEN_DEF_FN) {
		Print(";");
	}
	PrintNewline();
	PrintNewline();
}

void swsl::CppCompiler::DispatchDeclVar(const Token_DeclVar *t)
{
	PrintTabs();
	if (t->is_const) {
		Print("const ");
	} else {
		Print("mutable ");
	}
	PrintType(t->type_name);
	Print(" ");
	if (t->is_ref) {
		Print("&");
	}
	if (t->arr_size != NULL) {
		Print("[");
		Dispatch(t->arr_size);
		Print("]");
	}
	Print(";");
	PrintNewline();
}

void swsl::CppCompiler::DispatchDefFn(const Token_DefFn *t)
{
	Dispatch(t->head);
	Dispatch(t->body);
	PrintNewline();
}

void swsl::CppCompiler::DispatchDefVar(const Token_DefVar *t)
{
	Print("struct ");
	Print(t->var_name);
	Print("{");

	++m_depth;

	Dispatch(t->body);

	--m_depth;

	Print("}");
}

void swsl::CppCompiler::DispatchRoot(const SyntaxTree *t)
{
	Print("#include \"swsl_types.h\"");
	PrintNewline();
	PrintNewline();
	Dispatch(t->file);
}

#include <iostream>
void swsl::CppCompiler::DispatchErr(const Token_Err *t)
{
	++m_errs;
	std::cout << " > ";
	for (int i = 0; i < t->msg.GetSize(); ++i) {
		std::cout << t->msg[i];
	}
	std::cout << ": ";
	for (int i = 0; i < t->err.GetSize(); ++i) {
		if (!mtlChars::IsNewline(t->err[i]) && t->err[i] != '\t') { std::cout << t->err[i]; }
	}
	std::cout << std::endl;
}

void swsl::CppCompiler::DispatchExpr(const Token_Expr *t)
{
	Print("(");
	Dispatch(t->lhs);
	Print(t->op);
	Dispatch(t->rhs);
	Print(")");
}

void swsl::CppCompiler::DispatchFile(const Token_File *t)
{
	Dispatch(t->body);
}

void swsl::CppCompiler::DispatchIf(const Token_If *t)
{
	++m_cond_depth;

	PrintTabs();
	Print("{");
	PrintNewline();
	++m_depth;

	PrintTabs();
	PrintType("bool");
	Print(" ");
	PrintMask();
	Print(" = ");
	Dispatch(t->cond);
	Print(";");
	PrintNewline();

	PrintTabs();
	Print("if ( !(");
	Print(".all_fail() )");
	PrintNewline();
	Dispatch(t->if_body);

	if (t->el_body != NULL) {
		PrintMask();
		Print(" = !");
		PrintMask();

		PrintTabs();
		Print("if ( !(");
		Print(".all_fail() )");
		PrintNewline();
		Dispatch(t->el_body);
	}

	--m_depth;
	PrintTabs();
	Print("}");
	PrintNewline();

	--m_cond_depth;
}

void swsl::CppCompiler::DispatchRet(const Token_Ret *t)
{
	Print("return ");
	Dispatch(t->expr);
	Print(";");
}

void swsl::CppCompiler::DispatchSet(const Token_Set *t)
{
	PrintTabs();
	Dispatch(t->lhs);
	Print(" = ");
	Dispatch(t->rhs);
	Print(";");
	PrintNewline();
}

void swsl::CppCompiler::DispatchVar(const Token_Var *t)
{
	Print(t->var_name);
	if (t->idx != NULL) {
		Print("[");
		Dispatch(t->idx);
		Print("]");
	}
	if (t->mem != NULL) {
		Print(".");
		Dispatch(t->mem);
	}
}

void swsl::CppCompiler::DispatchLit(const Token_Lit *t)
{
	if (t->lit_type == Token_Lit::TYPE_BOOL) {
		if (t->lit.Compare("false", true)) {
			Print("MPL_FALSE");
		} else {
			Print("MPL_TRUE");
		}
	} else {
		Print(t->lit);
		if (t->lit_type == Token_Lit::TYPE_FLOAT) {
			Print("f");
		}
	}
}

void swsl::CppCompiler::DispatchWhile(const Token_While *t)
{
	++m_cond_depth;

	PrintTabs();
	Print("while (");
	Dispatch(t->cond);
	Print(")");
	PrintNewline();
	Dispatch(t->body);

	--m_cond_depth;
}

bool swsl::CppCompiler::Compile(swsl::SyntaxTree *t, const mtlChars &bin_name, swsl::Binary &out_bin)
{
	m_cond_depth = 0;
	m_depth = 0;
	m_errs = 0;
	m_buffer.Free();
	m_buffer.SetCapacity(4096);
	m_buffer.poolMemory = true;
	m_bin_name = bin_name;
	Dispatch(t);
	OutputBinary(out_bin);
	return m_errs == 0;
}
