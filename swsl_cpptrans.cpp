#include "swsl_cpptrans.h"

void swsl::CppTranslator::PrintTabs( void )
{
	for (int i = 0; i < m_depth - 1; ++i) {
		m_buffer.Add('\t');
	}
}

void swsl::CppTranslator::Print(const mtlChars &ch)
{
	const int BLOCK = 2048;
	while (m_buffer.GetSize() + ch.GetSize() > m_buffer.GetCapacity()) {
		m_buffer.SetCapacity(m_buffer.GetCapacity() + BLOCK);
	}
	for (int i = 0; i < ch.GetSize(); ++i) {
		m_buffer.Add(ch[i]);
	}
}

void swsl::CppTranslator::PrintNewline( void )
{
	m_buffer.Add('\n');
}

void swsl::CppTranslator::PrintMask(int mask)
{
	Print("m");
	mtlString num;
	num.FromInt(mask);
	Print(num);
}

void swsl::CppTranslator::PrintMask( void )
{
	PrintMask(m_mask_depth);
}

void swsl::CppTranslator::PrintPrevMask( void )
{
	PrintMask(m_mask_depth - 1);
}

void swsl::CppTranslator::PrintVarName(const mtlChars &name)
{
	Print("_");
	Print(name);
}

void swsl::CppTranslator::OutputBinary(swsl::Binary &bin)
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

bool swsl::CppTranslator::IsType(const Token *token, Token::TokenType type)
{
	return token != NULL && token->type == type;
}

bool swsl::CppTranslator::CompareMaskDepth(const Token *token) const
{
	if (token == NULL || token->type != Token::TOKEN_READ_VAR) { return false; }
	const Token *t = dynamic_cast<const Token_ReadVar*>(token)->decl_type;
	return m_mask_depth == ((t != NULL) ? t->CountAscend(Token::TOKEN_IF|Token::TOKEN_WHILE) : 0);
}

void swsl::CppTranslator::PrintReturnMerge( void )
{
	if (m_mask_depth > 1) {
		PrintTabs();
		PrintPrevMask();
		Print(" = ");
		PrintPrevMask();
		Print(" & m0;");
		PrintNewline();
	}
}

void swsl::CppTranslator::PrintType(const mtlChars &type)
{
	if (type.Compare("void", true)) {
		Print("void");
	} else if (type.Compare("bool", true)) {
		Print("mpl::wide_bool");
	} else if (type.Compare("int", true)) {
		Print("mpl::wide_int");
	} else if (type.Compare("float", true)) {
		Print("mpl::wide_float");
	} else {
		Print(m_bin_name);
		PrintVarName(type);
	}
}

void swsl::CppTranslator::DispatchAlias(const Token_Alias *t)
{
	if (t->scope == 0) {
		Print(m_bin_name);
	}
	PrintVarName(t->alias);
}

void swsl::CppTranslator::DispatchBody(const Token_Body *t)
{
	if (m_depth > 0) {
		PrintTabs();
		Print("{");
		PrintNewline();
	}
	++m_depth;

	if (IsType(t->parent, Token::TOKEN_DEF_FN)) {
		const Token_DefFn *def_fn = dynamic_cast<const Token_DefFn*>(t->parent);
		if (def_fn != NULL && def_fn->decl_type != NULL) {
			PrintTabs();
			DispatchTypeName(def_fn->decl_type);
			Print(" ret;");
			PrintNewline();
		}
	}

	Dispatch(t->tokens);

	--m_depth;
	if (m_depth > 0) {
		PrintTabs();
		Print("}");
	}
	PrintNewline();
}

void swsl::CppTranslator::DispatchDeclFn(const Token_DeclFn *t)
{
	Print("inline ");
	if (t->decl_type != NULL) {
		Dispatch(t->decl_type);
	} else {
		PrintType("void");
	}
	Print(" ");
	PrintType(t->fn_name);
	Print("(");
	Dispatch(t->params);
	PrintType("bool");
	Print(" ");
	PrintMask();
	Print(")");
	Print(";");
	PrintNewline();
	PrintNewline();
}

void swsl::CppTranslator::DispatchDeclType(const Token_DeclType *t)
{
	if (!t->type_name.Compare("void") && (t->permissions == Token_DeclType::ReadOnly || t->permissions == Token_DeclType::Constant) && t->parent != NULL && t->parent->parent != NULL && (t->parent->parent->type == Token::TOKEN_DECL_FN || t->parent->parent->type == Token::TOKEN_DEF_FN)) {
		Print("const ");
	}
	if (t->arr_size != NULL) {
		Print("swsl::wide_array<");
	}
	PrintType(t->type_name);
	if (t->arr_size != NULL) {
		Print(", ");
		Dispatch(t->arr_size);
		Print(">");
	}
	if (t->is_ref) {
		Print("&");
	}
}

void swsl::CppTranslator::DispatchDeclVar(const Token_DeclVar *t)
{
	const bool is_param = IsType(t->parent, Token::TOKEN_DECL_FN) || IsType(t->parent, Token::TOKEN_DEF_FN);

	if (!is_param) {
		PrintTabs();
	}
	Dispatch(t->decl_type);
	Print(" ");
	PrintVarName(t->var_name);
	if (t->expr != NULL) {
		Print(" = ");
		Dispatch(t->expr);
	}
	if (!is_param) {
		Print(";");
		PrintNewline();
	} else {
		Print(", ");
	}
}

void swsl::CppTranslator::DispatchDefFn(const Token_DefFn *t)
{
	Print("inline ");
	if (t->decl_type != NULL) {
		Dispatch(t->decl_type);
	} else {
		PrintType("void");
	}
	Print(" ");
	PrintType(t->fn_name);
	Print("(");
	Dispatch(t->params);
	PrintType("bool");
	Print(" ");
	PrintMask();
	Print(")");
	PrintNewline();
	Dispatch(t->body);
	PrintNewline();

	if (t->fn_name.Compare("main", true)) {
		DispatchCompatMain(t);
	}
}

void swsl::CppTranslator::DispatchDefType(const Token_DefType *t)
{
	Print("struct ");
	PrintType(t->type_name);
	PrintNewline();

	Dispatch(t->body);

	Print(";");
	PrintNewline();
	PrintNewline();
}

#include <iostream>
void swsl::CppTranslator::DispatchErr(const Token_Err *t)
{
	++m_errs;
	std::cout << " > ";
	for (int i = 0; i < t->msg.GetSize(); ++i) {
		std::cout << t->msg[i];
	}
	std::cout << ": ";
	for (int i = 0; i < t->err.GetSize(); ++i) {
		if (mtlChars::IsWhitespace(t->err[i])) {
			std::cout << " ";
			while (mtlChars::IsWhitespace(t->err[i])) {
				++i;
			}
			--i;
		} else {
			std::cout << t->err[i];
		}
	}
	std::cout << std::endl;
}

void swsl::CppTranslator::DispatchExpr(const Token_Expr *t)
{
	Print("(");
	Dispatch(t->lhs);
	Print(" ");
	Print(t->op);
	Print(" ");
	Dispatch(t->rhs);
	Print(")");
}

void swsl::CppTranslator::DispatchFile(const Token_File *t)
{
	Dispatch(t->body);
}

void swsl::CppTranslator::DispatchIf(const Token_If *t)
{
	++m_mask_depth;

	PrintTabs();
	Print("{");
	PrintNewline();
	++m_depth;

	PrintTabs();
	//if (t->el_body == NULL) {
	//	Print("const ");
	//}
	PrintType("bool");
	Print(" ");
	PrintMask();
	Print(" = ");
	Dispatch(t->cond);
	Print(" & ");
	PrintPrevMask();
	Print(";");
	PrintNewline();

	PrintTabs();
	Print("if ( !(");
	PrintMask();
	Print(".all_fail()) )");
	PrintNewline();
	Dispatch(t->if_body);

	if (t->el_body != NULL) {
		PrintTabs();
		PrintMask();
		Print(" = (!");
		PrintMask();
		Print(") & ");
		PrintPrevMask();
		Print(";");
		PrintNewline();

		PrintTabs();
		Print("if ( !(");
		PrintMask();
		Print(".all_fail()) )");
		PrintNewline();
		Dispatch(t->el_body);
	}

	--m_depth;
	PrintTabs();
	Print("}");
	PrintNewline();

	--m_mask_depth;

	PrintReturnMerge();
}

void swsl::CppTranslator::DispatchReadFn(const Token_ReadFn *t)
{
	PrintType(t->fn_name);
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

void swsl::CppTranslator::DispatchReadLit(const Token_ReadLit *t)
{
	if (t->lit_type == Token_ReadLit::TYPE_BOOL) {
		Print("mpl::wide_bool(");
		Print(t->lit);
		Print(")");
	} else if (t->lit_type == Token_ReadLit::TYPE_FLOAT) {
		Print("mpl::wide_float(");
		Print(t->lit);
		Print("f)");
	} else {
		Print("mpl::wide_int(");
		Print(t->lit);
		Print(")");
	}
}

void swsl::CppTranslator::DispatchReadVar(const Token_ReadVar *t)
{
	PrintVarName(t->var_name);
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

void swsl::CppTranslator::DispatchRet(const Token_Ret *t)
{
	const Token *p = t->parent;
	while (p != NULL && p->type != Token::TOKEN_DEF_FN) {
		p = p->parent;
	}

	const bool is_void = (p == NULL) || (dynamic_cast<const Token_DefFn*>(p)->decl_type == NULL);

	if (!is_void) {
		PrintTabs();
		Print("swsl::mov_if_true(ret, ");
		Dispatch(t->expr);
		Print(", ");
		PrintMask();
		Print(");");
		PrintNewline();
	}

	if (m_mask_depth > 0) {
		PrintTabs();
		Print("m0 = m0 & (!");
		PrintMask();
		Print(");");
		PrintNewline();

		PrintTabs();
		if (!is_void) { Print("if (m0.all_fail()) { return ret; }"); }
		else          { Print("if (m0.add_fail()) { return; }"); }
		PrintNewline();

		if (m_mask_depth > 1) {
			PrintTabs();
			PrintPrevMask();
			Print(" = ");
			PrintPrevMask();
			Print(" & m0;");
			PrintNewline();
		}
	} else {
		PrintTabs();
		if (!is_void) { Print("return ret;"); }
		else          { Print("return;"); }
		PrintNewline();
	}
}

void swsl::CppTranslator::DispatchRoot(const SyntaxTree *t)
{
	Print("#ifndef ");
	Print(m_bin_name);
	Print("_included__");
	PrintNewline();
	Print("#define ");
	Print(m_bin_name);
	Print("_included__");
	PrintNewline();
	PrintNewline();
	Print("#include \"swsl_types.h\"");
	PrintNewline();
	Print("#include \"swsl_math.h\"");
	PrintNewline();
	PrintNewline();
	Dispatch(t->file);
	Print("#endif");
	PrintNewline();
	PrintNewline();
}

void swsl::CppTranslator::DispatchSet(const Token_Set *t)
{
	// if lhs was defined at the same stack depth as current one
	// there is no need to apply a mask

	const bool needs_merge = !CompareMaskDepth(t->lhs);

	PrintTabs();
	if (needs_merge) {
		Print("swsl::mov_if_true(");
		Dispatch(t->lhs);
		Print(", ");
	} else {
		Dispatch(t->lhs);
		Print(" = ");
	}
	Dispatch(t->rhs);
	if (needs_merge) {
		Print(", ");
		PrintMask();
		Print(")");
	}
	Print(";");
	PrintNewline();
}

void swsl::CppTranslator::DispatchWhile(const Token_While *t)
{
	++m_mask_depth;

	PrintTabs();
	Print("{");
	PrintNewline();
	++m_depth;

	PrintTabs();
	PrintType("bool");
	Print(" ");
	PrintMask();
	Print(" = ");
	PrintPrevMask();
	Print(";");
	PrintNewline();

	PrintTabs();
	Print("while ( !(");

	Print("(");
	PrintMask();
	Print(" = ");
	Dispatch(t->cond);
	Print(" & ");
	PrintMask();
	Print(")");

	Print(".all_fail()) )");
	PrintNewline();
	Dispatch(t->body);

	--m_depth;
	PrintTabs();
	Print("}");
	PrintNewline();

	--m_mask_depth;

	PrintReturnMerge();
}

void swsl::CppTranslator::DispatchTypeName(const Token *t)
{
	const Token_DeclType *type = (t != NULL && t->type == Token::TOKEN_DECL_TYPE) ? dynamic_cast<const Token_DeclType*>(t) : NULL;
	if (type != NULL) {
		PrintType(type->type_name);
		if (type->arr_size != NULL) {
			Print("[");
			Dispatch(type->arr_size);
			Print("]");
		}
	}
}

void swsl::CppTranslator::DispatchCompatMain(const Token_DefFn *t)
{
	Print("inline ");
	if (t->decl_type != NULL) {
		Dispatch(t->decl_type);
	} else {
		PrintType("void");
	}
	Print(" ");
	PrintType(t->fn_name);
	Print("(void* inout, ");
	PrintType("bool");
	Print(" ");
	PrintMask();
	Print(")");
	PrintNewline();
	PrintTabs();
	Print("{");
	PrintNewline();
	++m_depth;

	PrintTabs();
	if (t->decl_type != NULL) {
		Print("return ");
	}
	PrintType(t->fn_name);
	Print("(");
	++m_depth;
	const mtlItem<Token*> *i = t->params.GetFirst();
	const mtlItem<Token*> *j = NULL;
	while (i != NULL && i->GetItem()->type == Token::TOKEN_DECL_VAR) {
		PrintNewline();
		PrintTabs();
		Print("*( (");
		DispatchTypeName(dynamic_cast<const Token_DeclVar*>(i->GetItem())->decl_type);
		Print("*)(((char*)inout)");

		if (j != NULL) {
			Print(" += sizeof(");
			DispatchTypeName(dynamic_cast<const Token_DeclVar*>(j->GetItem())->decl_type);
			Print(")");
		}
		Print(") ), ");
		j = i;
		i = i->GetNext();
	}
	PrintNewline();
	PrintTabs();
	PrintMask();
	--m_depth;
	PrintNewline();
	PrintTabs();
	Print(");");

	--m_depth;
	PrintNewline();
	PrintTabs();
	Print("}");
	PrintNewline();
}

void swsl::CppTranslator::SetBinaryName(const mtlChars &name)
{
	m_bin_name.Free();
	m_bin_name.Reserve(name.GetSize() + 1);
	if (name.GetSize() > 0 && mtlChars::IsNumeric(name[0])) {
		m_bin_name.Append('_');
	}
	for (int i = 0; i < name.GetSize(); ++i) {
		if (mtlChars::IsAlphanumeric(name[i])) {
			m_bin_name.Append(name[i]);
		} else if (m_bin_name.GetSize() > 1 && m_bin_name[m_bin_name.GetSize() - 1] != '_') {
			m_bin_name.Append('_');
		}
	}
}

bool swsl::CppTranslator::Compile(swsl::SyntaxTree *t, const mtlChars &bin_name, swsl::Binary &out_bin)
{
	m_mask_depth = 0;
	m_depth = 0;
	m_errs = 0;
	m_buffer.Free();
	m_buffer.SetCapacity(4096);
	m_buffer.poolMemory = true;
	SetBinaryName(bin_name);
	Dispatch(t);
	OutputBinary(out_bin);
	return m_errs == 0;
}
