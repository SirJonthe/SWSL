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
		PrintNewline();
	}
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
}

void swsl::CppCompiler::DispatchDeclVar(const Token_DeclVar *t)
{
}

void swsl::CppCompiler::DispatchDefFn(const Token_DefFn *t)
{


	PrintNewline();
	PrintNewline();
}

void swsl::CppCompiler::DispatchDefStruct(const Token_DefStruct *t)
{
}

void swsl::CppCompiler::DispatchEntry(const SyntaxTree *t)
{
	Print("#include \"swsl_types.h\"");
	PrintNewline();
	PrintNewline();
	Dispatch(t->file);
}

void swsl::CppCompiler::DispatchErr(const Token_Err *t)
{
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
	Print("mpl::wide_bool ");
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

void swsl::CppCompiler::DispatchNull( void )
{
}

void swsl::CppCompiler::DispatchRet(const Token_Ret *t)
{
}

void swsl::CppCompiler::DispatchSet(const Token_Set *t)
{
	PrintTabs();
	Dispatch(t->lhs);
	Print(" = ");
	Dispatch(t->rhs);
	Print(";");
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
	Print(t->lit);
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

void swsl::CppCompiler::Compile(swsl::SyntaxTree *t, swsl::Binary &out_bin)
{
	m_cond_depth = 0;
	m_depth = 0;
	m_buffer.Free();
	m_buffer.SetCapacity(4096);
	m_buffer.poolMemory = true;
	Dispatch(t);
}
