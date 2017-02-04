#include "swsl_astgen.h"

// NOTE:
// I've removed arrays for now
// Arrays should always be declared
// Type[5] var_name := { a, b, c, d, e };

#define decl_str "%?(const %| mutable) %w%?(&)%w"
#define decl_qlf 0
#define decl_typ 1
#define decl_ref 2
#define decl_nam 3

#define built_in_n 10
#define keywords_n 8
static const mtlChars built_in_types[built_in_n] = { "void", "bool", "int", "int2", "int3", "int4", "float", "float2", "float3", "float4" };
static const mtlChars keywords[keywords_n] = { "const", "mutable", "if", "else", "while", "return", "true", "false" };

swsl::SyntaxTree::SyntaxTree( void ) :
	Token(NULL, TOKEN_START), file(NULL), errs(0)
{}
swsl::SyntaxTree::~SyntaxTree( void )
{
	delete file;
}

swsl::Token_Err::Token_Err(const swsl::Token *p_parent) :
	Token(p_parent, TOKEN_ERR)
{}

swsl::Token_DeclVar::Token_DeclVar(const swsl::Token *p_parent) :
	Token(p_parent, TOKEN_DECL_VAR), R_type_def(NULL), arr_size(NULL)
{}
swsl::Token_DeclVar::~Token_DeclVar( void )
{
	delete arr_size;
}

swsl::Token_DeclFn::Token_DeclFn(const swsl::Token *p_parent) :
	Token(p_parent, TOKEN_DECL_FN), ret(NULL)
{}
swsl::Token_DeclFn::~Token_DeclFn( void )
{
	delete ret;
	mtlItem<Token*> *i = params.GetFirst();
	while (i != NULL) {
		delete i->GetItem();
		i = i->GetNext();
	}
}

swsl::Token_DefFn::Token_DefFn(const swsl::Token *p_parent) :
	Token(p_parent, TOKEN_DEF_FN), sig(NULL), body(NULL)
{}
swsl::Token_DefFn::~Token_DefFn( void )
{
	delete sig;
	delete body;
}

swsl::Token_DefStruct::Token_DefStruct(const swsl::Token *p_parent) :
	Token(p_parent, TOKEN_DEF_STRUCT), struct_name(NULL), struct_body(NULL)
{}
swsl::Token_DefStruct::~Token_DefStruct( void )
{
	delete struct_body;
}

swsl::Token_File::Token_File(const swsl::Token *p_parent) :
	Token(p_parent, TOKEN_FILE)
{}
swsl::Token_File::~Token_File( void )
{
	delete body;
}

swsl::Token_Body::Token_Body(const swsl::Token *p_parent) :
	Token(p_parent, TOKEN_BODY)
{}
swsl::Token_Body::~Token_Body( void )
{
	mtlItem<Token*> *i = tokens.GetFirst();
	while (i != NULL) {
		delete i->GetItem();
		i = i->GetNext();
	}
}

swsl::Token_Set::Token_Set(const swsl::Token *p_parent) :
	Token(p_parent, TOKEN_SET), lhs(NULL), rhs(NULL)
{}
swsl::Token_Set::~Token_Set( void )
{
	delete lhs;
	delete rhs;
}

swsl::Token_CallFn::Token_CallFn(const swsl::Token *p_parent) :
	Token(p_parent, TOKEN_CALL_FN), fn_name(NULL)
{}
swsl::Token_CallFn::~Token_CallFn( void )
{
	mtlItem<Token*> *i = input.GetFirst();
	while (i != NULL) {
		delete i->GetItem();
		i = i->GetNext();
	}
}

swsl::Token_Var::Token_Var(const swsl::Token *p_parent) :
	Token(p_parent, TOKEN_VAR), R_var_decl(NULL), var_name(NULL), idx(NULL), mem(NULL)
{}

swsl::Token_Var::~Token_Var( void )
{
	delete idx;
	delete mem;
}

swsl::Token_Lit::Token_Lit(const swsl::Token *p_parent) :
	Token(p_parent, TOKEN_LIT), lit(NULL)
{}
swsl::Token_Lit::~Token_Lit( void )
{
	delete lit;
}

swsl::Token_Expr::Token_Expr(const swsl::Token *p_parent) :
	Token(p_parent, TOKEN_EXPR), lhs(NULL), rhs(NULL)
{}
swsl::Token_Expr::~Token_Expr( void )
{
	delete lhs;
	delete rhs;
}

swsl::Token_If::Token_If(const swsl::Token *p_parent) :
	Token(p_parent, TOKEN_IF), cond(NULL), if_body(NULL), el_body(NULL)
{}
swsl::Token_If::~Token_If( void )
{
	delete cond;
	delete if_body;
	delete el_body;
}

swsl::Token_While::Token_While(const swsl::Token *p_parent) :
	Token(p_parent, TOKEN_WHILE), cond(NULL), body(NULL)
{}
swsl::Token_While::~Token_While( void )
{
	delete cond;
	delete body;
}

swsl::Token_Ret::Token_Ret(const swsl::Token *p_parent) :
	Token(p_parent, TOKEN_RET), expr(NULL)
{}
swsl::Token_Ret::~Token_Ret( void )
{
	delete expr;
}

bool swsl::SyntaxTreeGenerator::IsReserved(const mtlChars &name)
{
	for (int i = 0; i < built_in_n; ++i) {
		if (name.Compare(built_in_types[i], true)) { return true; }
	}
	for (int i = 0; i < keywords_n; ++i) {
		if (name.Compare(keywords[i], true)) { return true; }
	}
	return false;
}

bool swsl::SyntaxTreeGenerator::VerifyName(const mtlChars &name)
{
	if (name.GetSize() > 0 && !IsReserved(name)) {
		for (int i = 1; i < name.GetSize(); ++i) {
			if (!mtlChars::IsAlphanumeric(name[i]) && name[i] != '_') { return false; }
		}
		return mtlChars::IsAlpha(name[0]) || name[0] == '_';
	}
	return false;
}

bool swsl::SyntaxTreeGenerator::CmpVarDeclName(const mtlChars &name, const swsl::Token_DeclVar *tok)
{
	return tok != NULL && name.Compare(tok->var_name, true);
}

bool swsl::SyntaxTreeGenerator::CmpFnDeclName(const mtlChars &name, const swsl::Token_DeclFn *tok)
{
	return tok != NULL && CmpVarDeclName(name, dynamic_cast<Token_DeclVar*>(tok->ret));
}

bool swsl::SyntaxTreeGenerator::CmpFnDefName(const mtlChars &name, const swsl::Token_DefFn *tok)
{
	return tok != NULL && CmpFnDeclName(name, dynamic_cast<Token_DeclFn*>(tok->sig));
}

bool swsl::SyntaxTreeGenerator::CmpStructDefName(const mtlChars &name, const swsl::Token_DefStruct *tok)
{
	return tok != NULL && name.Compare(tok->struct_name, true);
}

bool swsl::SyntaxTreeGenerator::NewName(const mtlChars &name, const swsl::Token *parent)
{
	while (parent != NULL && parent->type != Token::TOKEN_BODY) {
		parent = parent->parent;
	}

	if (parent != NULL && parent->type == Token::TOKEN_BODY) {
		const mtlItem<Token*> *i = dynamic_cast<const Token_Body*>(parent)->tokens.GetFirst();
		while (i != NULL) {
			const Token *t = i->GetItem();
			switch (t->type) {
			case Token::TOKEN_DECL_FN:
				if (CmpFnDeclName(name, dynamic_cast<const Token_DeclFn*>(t))) { return false; }
				break;
			case Token::TOKEN_DECL_VAR:
				if (CmpVarDeclName(name, dynamic_cast<const Token_DeclVar*>(t))) { return false; }
				break;
			case Token::TOKEN_DEF_FN:
				if (CmpFnDefName(name, dynamic_cast<const Token_DefFn*>(t))) { return false; }
				break;
			case Token::TOKEN_DEF_STRUCT:
				if (CmpStructDefName(name, dynamic_cast<const Token_DefStruct*>(t))) { return false; }
				break;
			default: break;
			}
			i = i->GetNext();
		}
	}
	return true;
}

bool swsl::SyntaxTreeGenerator::FindVar(const mtlChars &name, const swsl::Token *parent)
{
	if (parent != NULL) {
		switch (parent->type) {
		case Token::TOKEN_BODY:
			{
				const Token_Body *body = dynamic_cast<const Token_Body*>(parent);
				const mtlItem<Token*> *decl_iter = body->tokens.GetFirst();
				while (decl_iter != NULL) {
					if (decl_iter->GetItem()->type == Token::TOKEN_DECL_VAR) {
						Token_DeclVar *decl = dynamic_cast<Token_DeclVar*>(decl_iter->GetItem());
						if (decl != NULL && name.Compare(decl->var_name, true)) { return true; }
					}
					decl_iter = decl_iter->GetNext();
				}
			}
			break;
		default:
			return FindVar(name, parent->parent);
		}
	}
	return false;
}

bool swsl::SyntaxTreeGenerator::FindType(const mtlChars &name, const swsl::Token *parent)
{
	if (parent != NULL) {
	}

}

swsl::Token *swsl::SyntaxTreeGenerator::ProcessError(const mtlChars &msg, const mtlChars &err, const swsl::Token *parent)
{
	Token_Err *token = new Token_Err(parent);

	++root->errs;
	token->err = err;
	token->msg = msg;
	return token;
}

swsl::Token *swsl::SyntaxTreeGenerator::ProcessFindVar(const mtlChars &name, const swsl::Token *parent)
{
	if (FindVar(name, parent)) {
		Token_Word *token = new Token_Word(parent);

		token->word = name;
		return token;
	}
	return ProcessError("Undeclared", name, parent);
}

swsl::Token *swsl::SyntaxTreeGenerator::ProcessNewName(const mtlChars &name, const swsl::Token *parent)
{
	if (VerifyName(name) && NewName(name, parent)) {
		Token_Word *token = new Token_Word(parent);

		token->word = name;
		return token;
	}
	return ProcessError("Naming", name, parent);
}

swsl::Token *swsl::SyntaxTreeGenerator::ProcessFindType(const mtlChars &name, const swsl::Token *parent)
{
	if (FindType(name, parent)) {
		Token_Word *token = new Token_Word(parent);

		token->word = name;
		return token;
	}
	return ProcessError("Undefined", name, parent);
}

swsl::Token *swsl::SyntaxTreeGenerator::ProcessDecl(const mtlChars &rw, const mtlChars &type_name, const mtlChars &ref, const mtlChars &fn_name, const swsl::Token *parent)
{
	Token_DeclVar *token = new Token_DeclVar(parent);

	token->is_const = rw.Compare("mutable", true) ? false : true;
	token->type_name = ProcessFindType(type_name, token);
	token->is_ref = (ref.GetSize() == 1 && ref[0] == '&');
	token->var_name = ProcessNewName(fn_name, token);
	return token;
}

swsl::Token *swsl::SyntaxTreeGenerator::ProcessFuncCall(const mtlChars &fn_name, const mtlChars &params, const swsl::Token *parent)
{
	Token_CallFn *token = new Token_CallFn(parent);

	token->fn_name = ProcessNewName(fn_name, parent);
	mtlArray<mtlChars> m;
	mtlSyntaxParser p;
	p.SetBuffer(params);
	while (!p.IsEnd()) {
		switch (p.Match("%S, %| %s", m)) {
		case 0:
		case 1:
		default:
			token->input.AddLast(ProcessExpression(m[0], parent));
			break;
		}
	}
	return token;
}

swsl::Token *swsl::SyntaxTreeGenerator::ProcessLiteral(const mtlChars &lit, const swsl::Token *parent)
{
	Token_Lit *token = new Token_Lit(parent);

	token->lit = lit;
	if (lit.IsInt()) {
		token->lit = lit;
		token->lit_type = Token_Lit::TYPE_INT;
	} else if (lit.IsFloat()) {
		token->lit_type = Token_Lit::TYPE_FLOAT;
	} else if (lit.Compare("true", true) || lit.Compare("false", true)) {
		token->lit_type = Token_Lit::TYPE_BOOL;
	} else {
		token->lit_type = Token_Lit::TYPE_ERR;
	}
	return token;
}

swsl::Token *swsl::SyntaxTreeGenerator::ProcessVariable(mtlSyntaxParser &var, const swsl::Token *parent)
{
	Token_Var *token = new Token_Var(parent);

	mtlArray<mtlChars> m;
	switch (var.Match("%w%0 %| %w. %| %w[%S]%0 %| %w[%S]. %| %s", m)) {
	case 1:
		token->mem = ProcessVariable(var, token);
	case 0:
		token->var_name = ProcessFindVar(m[0], token);
		break;

	case 3:
		token->mem = ProcessVariable(var, token);
	case 2:
		token->var_name = ProcessFindVar(m[0], token);
		token->idx = ProcessExpression(m[0], token);
		break;

	default:
		delete token;
		return ProcessError("Operand", m[0], parent);
	}
	return token;
}

swsl::Token *swsl::SyntaxTreeGenerator::ProcessOperand(const mtlChars &val, const swsl::Token *parent)
{
	Token *token = NULL;

	mtlArray<mtlChars> m;
	mtlSyntaxParser p;
	p.SetBuffer(val);

	if (p.Match("%i%0 %| %r%0 %| true%0 %| false%0", m) > -1) {
		token = ProcessLiteral(val, parent);
	} else {
		mtlSyntaxParser p2;
		p2.SetBuffer(val);
		token = ProcessVariable(p2, parent);
	}

	return token;
}

swsl::Token *swsl::SyntaxTreeGenerator::ProcessSet(const mtlChars &lhs, const mtlChars &rhs, const swsl::Token *parent)
{
	Token_Set *token = new Token_Set(parent);

	token->lhs = ProcessOperand(lhs, token);
	token->rhs = ProcessExpression(rhs, token);
	return token;
}

swsl::Token *swsl::SyntaxTreeGenerator::ProcessFuncDecl(const mtlChars &rw, const mtlChars &type_name, const mtlChars &ref, const mtlChars &fn_name, const mtlChars &params, const swsl::Token *parent)
{
	Token_DeclFn *token = new Token_DeclFn(parent);

	token->ret = ProcessDecl(rw, type_name, ref, fn_name, token);
	mtlArray<mtlChars> m;
	mtlSyntaxParser p;
	p.SetBuffer(params);
	while (!p.IsEnd()) {
		switch (p.Match(decl_str "%| %s", m)) {
		case 0:
			token->params.AddLast(ProcessDecl(m[decl_qlf], m[decl_typ], m[decl_ref], m[decl_nam], token));
			break;
		default:
			token->params.AddLast(ProcessError("Syntax(3)", m[0], token));
			return token;
		}
		p.Match(",");
	}
	return token;
}

swsl::Token *swsl::SyntaxTreeGenerator::ProcessOperation(const mtlChars &lhs, const mtlChars &op, const mtlChars &rhs, const swsl::Token *parent)
{
	Token_Expr *token = new Token_Expr(parent);

	token->op = op;
	token->lhs = lhs.GetSize() > 0 ? ProcessExpression(lhs, token) : NULL;
	token->rhs = ProcessExpression(rhs, token);
	return token;
}

swsl::Token *swsl::SyntaxTreeGenerator::ProcessExpression(const mtlChars &expr, const swsl::Token *parent)
{
	Token *token = NULL;

	mtlArray<mtlChars> m;
	mtlSyntaxParser p;
	p.SetBuffer(expr);
	switch (p.Match("%s+%S %| %s-%S %| %S*%S %| %S/%S %| %S==%S %| %S!=%S %| %S<=%S %| %S<%S %| %S>=%S %| %S>%S %| (%S)%0 %| %w(%s)%0 %| %S", m)) {
	case 0:
		token = ProcessOperation(m[0], "+", m[1], parent);
		break;

	case 1:
		token = ProcessOperation(m[0], "-", m[1], parent);
		break;

	case 2:
		token = ProcessOperation(m[0], "*", m[1], parent);
		break;

	case 3:
		token = ProcessOperation(m[0], "/", m[1], parent);
		break;

	case 4:
		token = ProcessOperation(m[0], "==", m[1], parent);
		break;

	case 5:
		token = ProcessOperation(m[0], "!=", m[1], parent);
		break;

	case 6:
		token = ProcessOperation(m[0], "<=", m[1], parent);
		break;

	case 7:
		token = ProcessOperation(m[0], "<", m[1], parent);
		break;

	case 8:
		token = ProcessOperation(m[0], ">=", m[1], parent);
		break;

	case 9:
		token = ProcessOperation(m[0], ">", m[1], parent);
		break;

	case 10:
		token = ProcessExpression(m[0], parent);
		break;

	case 11:
		token = ProcessFuncCall(m[0], m[1], parent);
		break;

	case 12:
		token = ProcessOperand(m[0], parent);
		break;

	default:
		token = ProcessError("Syntax(5)", p.GetBuffer(), token);
		break;
	}
	return token;
}

swsl::Token *swsl::SyntaxTreeGenerator::ProcessIf(const mtlChars &cond, const mtlChars &body, const swsl::Token *parent, mtlSyntaxParser &p)
{
	Token_If *token = new Token_If(parent);

	token->cond = ProcessExpression(cond, token);
	token->if_body = ProcessBody(body, token);
	mtlArray<mtlChars> m;
	switch (p.Match("else if(%S){%s} %| else{%s}", m)) {
	case 0:
		token->el_body = ProcessIf(m[0], m[1], token, p);
		break;

	case 1:
		token->el_body = ProcessBody(m[0], token);
		break;

	default: break; // Not an error
	}
	return token;
}

swsl::Token *swsl::SyntaxTreeGenerator::ProcessWhile(const mtlChars &cond, const mtlChars &body, const swsl::Token *parent)
{
	Token_While *token = new Token_While(parent);

	token->cond = ProcessExpression(cond, token);
	token->body = ProcessBody(body, token);
	return token;
}

swsl::Token *swsl::SyntaxTreeGenerator::ProcessReturn(const mtlChars &expr, const swsl::Token *parent)
{
	Token_Ret *token = new Token_Ret(parent);

	token->expr = ProcessExpression(expr, token);
	return token;
}

swsl::Token *swsl::SyntaxTreeGenerator::ProcessBody(const mtlChars &body, const swsl::Token *parent)
{
	Token_Body *token = new Token_Body(parent);

	mtlArray<mtlChars> m;
	mtlSyntaxParser p;
	p.SetBuffer(body);
	while (!p.IsEnd()) {
		switch (p.Match("{%s} %| if(%S){%s} %| while(%S){%s} %| return %s; %| " decl_str ":=%S; %| " decl_str "; %| %w:=%S; %| %s", m)) {
		case 0:
			token->tokens.AddLast(ProcessBody(m[0], token));
			break;

		case 1:
			token->tokens.AddLast(ProcessIf(m[0], m[1], token, p));
			break;

		case 2:
			token->tokens.AddLast(ProcessWhile(m[0], m[1], token));
			break;

		case 3:
			token->tokens.AddLast(ProcessReturn(m[0], token));
			break;

		case 4:
			token->tokens.AddLast(ProcessDecl(m[decl_qlf], m[decl_typ], m[decl_ref], m[decl_nam], token));
			token->tokens.AddLast(ProcessSet(m[decl_nam], m[decl_nam + 1], token));
			break;

		case 5:
			token->tokens.AddLast(ProcessDecl(m[decl_qlf], m[decl_typ], m[decl_ref], m[decl_nam], token));
			break;

		case 6:
			token->tokens.AddLast(ProcessSet(m[0], m[1], token));
			break;

		default:
			token->tokens.AddLast(ProcessError("Syntax(2)", m[0], token));
			break;
		}
	}
	return token;
}

swsl::Token *swsl::SyntaxTreeGenerator::ProcessFuncDef(const mtlChars &rw, const mtlChars &type_name, const mtlChars &ref, const mtlChars &fn_name, const mtlChars &params, const mtlChars &body, const swsl::Token *parent)
{
	Token_DefFn *token = new Token_DefFn(parent);

	token->sig = ProcessFuncDecl(rw, type_name, ref, fn_name, params, token);
	token->body = ProcessBody(body, token);
	return token;
}

swsl::Token *swsl::SyntaxTreeGenerator::ProcessStructMem(const mtlChars &decls)
{
	Token_Body *token = new Token_Body(NULL);

	mtlArray<mtlChars> m;
	mtlSyntaxParser p;
	p.SetBuffer(decls);
	while (!p.IsEnd()) {
		switch (p.Match(decl_str "; %| %s", m)) {
		case 0:
			token->tokens.AddLast(ProcessDecl(m[decl_qlf], m[decl_typ], m[decl_ref], m[decl_nam], token));
			break;

		default:
			token->tokens.AddLast(ProcessError("Syntax(3)", m[0], token));
			break;
		}
	}
	return token;
}

swsl::Token *swsl::SyntaxTreeGenerator::ProcessStructDef(const mtlChars &struct_name, const mtlChars &decls, const swsl::Token *parent)
{
	Token_DefStruct *token = new Token_DefStruct(parent);

	token->struct_name = ProcessNewName(struct_name, parent);
	token->struct_body = ProcessStructMem(decls);
	return token;
}

swsl::Token *swsl::SyntaxTreeGenerator::ProcessFile(const mtlChars &contents, const swsl::Token *parent)
{
	Token_Body *token = new Token_Body(parent);

	mtlArray<mtlChars> m;
	mtlSyntaxParser p;
	p.SetBuffer(contents);
	while (!p.IsEnd()) {
		switch (p.Match(decl_str "(%s); %| " decl_str "(%s){%s} %| struct %w{%s}; %| import\"%S\" %| %s", m)) {
		case 0:
			token->tokens.AddLast(ProcessFuncDecl(m[decl_qlf], m[decl_typ], m[decl_ref], m[decl_nam], m[4], token));
			break;

		case 1:
			token->tokens.AddLast(ProcessFuncDef(m[decl_qlf], m[decl_typ], m[decl_ref], m[decl_nam], m[4], m[5], token));
			break;

		case 2:
			token->tokens.AddLast(ProcessStructDef(m[0], m[1], token));
			break;

		case 3:
			token->tokens.AddLast(LoadFile(m[0], token));
			break;

		default:
			token->tokens.AddLast(ProcessError("Syntax(4)", m[0], token));
			break;
		}
	}
	return token;
}

swsl::Token *swsl::SyntaxTreeGenerator::LoadFile(const mtlChars &file_name, const swsl::Token *parent)
{
	Token_File *token = new Token_File(parent);

	if (!mtlSyntaxParser::BufferFile(file_name, token->content)) {
		delete token;
		return ProcessError("File not found", file_name, parent);
	}
	token->file_name = file_name;
	token->body = ProcessFile(token->content, token);
	return token;
}

swsl::SyntaxTree *swsl::SyntaxTreeGenerator::Generate(const mtlChars &entry_file)
{
	root = new SyntaxTree();

	root->file = LoadFile(entry_file, root);
	return root;
}
