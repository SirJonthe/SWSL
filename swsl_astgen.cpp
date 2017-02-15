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

#define to_str(X) #X

swsl::Token_Err::Token_Err(const swsl::Token *p_parent) :
	Token(p_parent, TOKEN_ERR)
{}

swsl::SyntaxTree::SyntaxTree( void ) :
	Token(NULL, TOKEN_ROOT), file(NULL), errs(0)
{}
swsl::SyntaxTree::~SyntaxTree( void )
{
	delete file;
}

swsl::Token_Alias::Token_Alias(const Token *p_parent) :
	Token(p_parent, TOKEN_ALIAS)
{}

swsl::Token_DeclType::Token_DeclType(const Token *p_parent) :
	Token(p_parent, TOKEN_DECL_TYPE), arr_size(NULL)
{}
swsl::Token_DeclType::~Token_DeclType( void )
{
	delete arr_size;
}

swsl::Token_DeclVar::Token_DeclVar(const swsl::Token *p_parent) :
	Token(p_parent, TOKEN_DECL_VAR), decl_type(NULL), arr_size(NULL)
{}
swsl::Token_DeclVar::~Token_DeclVar( void )
{
	delete decl_type;
	delete arr_size;
}

swsl::Token_DeclFn::Token_DeclFn(const swsl::Token *p_parent) :
	Token(p_parent, TOKEN_DECL_FN), decl_type(NULL)
{}
swsl::Token_DeclFn::~Token_DeclFn( void )
{
	delete decl_type;
	mtlItem<Token*> *i = params.GetFirst();
	while (i != NULL) {
		delete i->GetItem();
		i = i->GetNext();
	}
}

swsl::Token_DefFn::Token_DefFn(const swsl::Token *p_parent) :
	Token(p_parent, TOKEN_DEF_FN), head(NULL), body(NULL)
{}
swsl::Token_DefFn::~Token_DefFn( void )
{
	delete head;
	delete body;
}

swsl::Token_DefVar::Token_DefVar(const swsl::Token *p_parent) :
	Token(p_parent, TOKEN_DEF_VAR), body(NULL)
{}
swsl::Token_DefVar::~Token_DefVar( void )
{
	delete body;
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

swsl::Token_Expr::Token_Expr(const swsl::Token *p_parent) :
	Token(p_parent, TOKEN_EXPR), lhs(NULL), rhs(NULL)
{}
swsl::Token_Expr::~Token_Expr( void )
{
	delete lhs;
	delete rhs;
}

swsl::Token_ReadFn::Token_ReadFn(const swsl::Token *p_parent) :
	Token(p_parent, TOKEN_READ_FN)
{}
swsl::Token_ReadFn::~Token_ReadFn( void )
{
	mtlItem<Token*> *i = input.GetFirst();
	while (i != NULL) {
		delete i->GetItem();
		i = i->GetNext();
	}
}

swsl::Token_ReadVar::Token_ReadVar(const swsl::Token *p_parent) :
	Token(p_parent, TOKEN_READ_VAR), decl_type(NULL), idx(NULL), mem(NULL)
{}

swsl::Token_ReadVar::~Token_ReadVar( void )
{
	delete decl_type;
	delete idx;
	delete mem;
}

swsl::Token_ReadLit::Token_ReadLit(const swsl::Token *p_parent) :
	Token(p_parent, TOKEN_READ_LIT)
{}

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
	return tok != NULL && name.Compare(tok->fn_name, true);
}

bool swsl::SyntaxTreeGenerator::CmpFnDefName(const mtlChars &name, const swsl::Token_DefFn *tok)
{
	if (tok != NULL) {
		try {
			return CmpFnDeclName(name, dynamic_cast<Token_DeclFn*>(tok->head));
		} catch (...) {
			return false;
		}
	}
	return false;
}

bool swsl::SyntaxTreeGenerator::CmpVarDefName(const mtlChars &name, const swsl::Token_DefVar *tok)
{
	return tok != NULL && name.Compare(tok->var_name, true);
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
			case Token::TOKEN_DEF_VAR:
				if (CmpVarDefName(name, dynamic_cast<const Token_DefVar*>(t))) { return false; }
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

swsl::Token *swsl::SyntaxTreeGenerator::ProcessError(const mtlChars &msg, mtlChars err, const swsl::Token *parent)
{
	Token_Err *token = new Token_Err(parent);

	mtlSyntaxParser p;
	p.SetBuffer(err);
	p.EnableCaseSensitivity();
	mtlArray<mtlChars> m;
	while (!p.IsEnd()) {
		if (p.Match("%s{%s} %| %s; %| %s") > -1) {
			err = m[0];
			p.SetBuffer(err);
		}
	}
	++root->errs;
	token->err = err;
	token->msg = msg;
	return token;
}

/*swsl::Token *swsl::SyntaxTreeGenerator::ProcessFindVar(const mtlChars &name, const swsl::Token *parent)
{
	if (FindVar(name, parent)) {
		Token_Word *token = new Token_Word(parent);

		token->word = name;
		return token;
	}
	return ProcessError("Undeclared(" to_str(ProcessFindVar) ")", name, parent);
}

swsl::Token *swsl::SyntaxTreeGenerator::ProcessFindType(const mtlChars &name, const swsl::Token *parent)
{
	if (FindType(name, parent)) {
		Token_Word *token = new Token_Word(parent);

		token->word = name;
		return token;
	}
	return ProcessError("Undefined(" to_str(ProcessFindType) ")", name, parent);
}*/

swsl::Token *swsl::SyntaxTreeGenerator::ProcessDeclType(const mtlChars &rw, const mtlChars &type_name, const mtlChars &ref, const swsl::Token *parent)
{
	Token_DeclType *token = new Token_DeclType(parent);

	token->is_const = rw.Compare("mutable", true) ? false : true;
	token->type_name = type_name;
	token->is_ref = (ref.GetSize() == 1 && ref[0] == '&');
	return token;
}

swsl::Token *swsl::SyntaxTreeGenerator::ProcessDeclVar(const mtlChars &rw, const mtlChars &type_name, const mtlChars &ref, const mtlChars &var_name, const swsl::Token *parent)
{
	Token_DeclVar *token = new Token_DeclVar(parent);

	token->decl_type = ProcessDeclType(rw, type_name, ref, token);
	token->var_name  = var_name;
	return token;
}

swsl::Token *swsl::SyntaxTreeGenerator::ProcessFuncCall(const mtlChars &fn_name, const mtlChars &params, const swsl::Token *parent)
{
	Token_ReadFn *token = new Token_ReadFn(parent);

	token->fn_name = fn_name;
	mtlArray<mtlChars> m;
	mtlSyntaxParser p;
	p.SetBuffer(params);
	p.EnableCaseSensitivity();
	while (!p.IsEnd()) {
		switch (p.Match("%S, %| %s", m)) {
		case 0:
		case 1:
			token->input.AddLast(ProcessExpression(m[0], parent));
			break;
		}
	}
	return token;
}

swsl::Token *swsl::SyntaxTreeGenerator::ProcessLiteral(const mtlChars &lit, const swsl::Token *parent)
{
	Token_ReadLit *token = new Token_ReadLit(parent);

	token->lit = lit;
	if (lit.IsInt()) {
		token->lit_type = Token_ReadLit::TYPE_INT;
	} else if (lit.IsFloat()) {
		token->lit_type = Token_ReadLit::TYPE_FLOAT;
	} else {
		token->lit_type = Token_ReadLit::TYPE_BOOL;
	}
	return token;
}

swsl::Token *swsl::SyntaxTreeGenerator::ProcessVariable(mtlSyntaxParser &var, const swsl::Token *parent)
{
	Token_ReadVar *token = new Token_ReadVar(parent);

	mtlArray<mtlChars> m;
	switch (var.Match("%w%0 %| %w. %| %w[%S]%0 %| %w[%S]. %| %s", m)) {
	case 1:
		token->mem = ProcessVariable(var, token);
	case 0:
		token->var_name = m[0];
		break;

	case 3:
		token->mem = ProcessVariable(var, token);
	case 2:
		token->var_name = m[0];
		token->idx = ProcessExpression(m[0], token);
		break;

	default:
		delete token;
		return ProcessError("Operand(" to_str(ProcessVariable) ")", m[0], parent);
	}
	return token;
}

swsl::Token *swsl::SyntaxTreeGenerator::ProcessOperand(const mtlChars &val, const swsl::Token *parent)
{
	Token *token = NULL;

	mtlArray<mtlChars> m;
	mtlSyntaxParser p;
	p.SetBuffer(val);
	p.EnableCaseSensitivity();
	if (p.Match("%i%0 %| %r%0 %| true%0 %| false%0", m) > -1) {
		token = ProcessLiteral(val, parent);
	} else {
		mtlSyntaxParser p2;
		p2.SetBuffer(val);
		p2.EnableCaseSensitivity();
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

	token->fn_name = fn_name;
	token->decl_type = ProcessDeclType(rw, type_name, ref, token);
	mtlArray<mtlChars> m;
	mtlSyntaxParser p;
	p.SetBuffer(params);
	p.EnableCaseSensitivity();
	while (!p.IsEnd()) {
		switch (p.Match(decl_str " %| %s", m)) {
		case 0:
			token->params.AddLast(ProcessDeclVar(m[decl_qlf], m[decl_typ], m[decl_ref], m[decl_nam], token));
			break;
		default:
			token->params.AddLast(ProcessError("Syntax(" to_str(ProcessFuncDecl) ")", m[0], token));
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
	p.EnableCaseSensitivity();
	switch (p.Match("%s+%S %| %s-%S %| %S*%S %| %S/%S %| %S==%S %| %S!=%S %| %S<=%S %| %S<%S %| %S>=%S %| %S>%S %| (%S)%0 %| %w(%s)%0 %| %s", m)) {
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
		token = ProcessError("Syntax(" to_str(ProcessExpression) ")", p.GetBuffer(), token);
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
	p.EnableCaseSensitivity();
	while (!p.IsEnd()) {
		switch (p.Match("{%s} %| if(%S){%s} %| while(%S){%s} %| return %s; %| " decl_str "=%S; %| " decl_str "; %| %w=%S; %| %s", m)) {
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
			token->tokens.AddLast(ProcessDeclVar(m[decl_qlf], m[decl_typ], m[decl_ref], m[decl_nam], token));
			token->tokens.AddLast(ProcessSet(m[decl_nam], m[decl_nam + 1], token));
			break;

		case 5:
			token->tokens.AddLast(ProcessDeclVar(m[decl_qlf], m[decl_typ], m[decl_ref], m[decl_nam], token));
			break;

		case 6:
			token->tokens.AddLast(ProcessSet(m[0], m[1], token));
			break;

		default:
			token->tokens.AddLast(ProcessError("Syntax(" to_str(ProcessBody) ")", m[0], token));
			break;
		}
	}
	return token;
}

swsl::Token *swsl::SyntaxTreeGenerator::ProcessFuncDef(const mtlChars &rw, const mtlChars &type_name, const mtlChars &ref, const mtlChars &fn_name, const mtlChars &params, const mtlChars &body, const swsl::Token *parent)
{
	Token_DefFn *token = new Token_DefFn(parent);

	token->head = ProcessFuncDecl(rw, type_name, ref, fn_name, params, token);
	token->body = ProcessBody(body, token);
	return token;
}

swsl::Token *swsl::SyntaxTreeGenerator::ProcessVarMemDecl(const mtlChars &decls)
{
	Token_Body *token = new Token_Body(NULL);

	mtlArray<mtlChars> m;
	mtlSyntaxParser p;
	p.SetBuffer(decls);
	p.EnableCaseSensitivity();
	while (!p.IsEnd()) {
		switch (p.Match(decl_str "; %| %s", m)) {
		case 0:
			token->tokens.AddLast(ProcessDeclVar(m[decl_qlf], m[decl_typ], m[decl_ref], m[decl_nam], token));
			break;

		default:
			token->tokens.AddLast(ProcessError("Syntax(" to_str(ProcessVarMemDecl) ")", m[0], token));
			break;
		}
	}
	return token;
}

swsl::Token *swsl::SyntaxTreeGenerator::ProcessVarDef(const mtlChars &struct_name, const mtlChars &decls, const swsl::Token *parent)
{
	if (VerifyName(struct_name) && NewName(struct_name, parent)) {
		Token_DefVar *token = new Token_DefVar(parent);

		token->var_name = struct_name;
		token->body = ProcessVarMemDecl(decls);
		return token;
	}
	return ProcessError("Collision(" to_str(ProcessVarDef) ")", struct_name, parent);
}

swsl::Token *swsl::SyntaxTreeGenerator::ProcessFile(const mtlChars &contents, const swsl::Token *parent)
{
	Token_Body *token = new Token_Body(parent);

	mtlArray<mtlChars> m;
	mtlSyntaxParser p;
	p.SetBuffer(contents);
	p.EnableCaseSensitivity();
	while (!p.IsEnd()) {
		switch (p.Match(decl_str "(%s); %| " decl_str "(%s){%s} %| struct %w{%s}; %| import\"%S\" %| %s", m)) {
		case 0:
			token->tokens.AddLast(ProcessFuncDecl(m[decl_qlf], m[decl_typ], m[decl_ref], m[decl_nam], m[4], token));
			break;

		case 1:
			token->tokens.AddLast(ProcessFuncDef(m[decl_qlf], m[decl_typ], m[decl_ref], m[decl_nam], m[4], m[5], token));
			break;

		case 2:
			token->tokens.AddLast(ProcessVarDef(m[0], m[1], token));
			break;

		case 3:
			token->tokens.AddLast(LoadFile(m[0], token));
			break;

		default:
			token->tokens.AddLast(ProcessError("Syntax(" to_str(ProcessFile) ")", m[0], token));
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
		return ProcessError("File not found(" to_str(LoadFile) ")", file_name, parent);
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
