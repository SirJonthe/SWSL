#include "swsl_astgen.h"

// NOTE:
// I've removed arrays for now
// Arrays should always be declared
// Type[5] var_name := { a, b, c, d, e };

#define _decl_str "%?(const %| mutable) %w%?(&)%w"
#define _decl_qlf 0
#define _decl_typ 1
#define _decl_ref 2
#define _decl_nam 3

#define _built_in_n 4
#define _reserved_n 16
#define _keywords_n 9
static const mtlChars _built_in_types[_built_in_n] = { "void", "bool", "int", "float" };
static const mtlChars _reserved[_reserved_n] = { "min", "max", "abs", "round", "trunc", "floor", "ceil", "sin", "cos", "tan", "asin", "acos", "atan", "sqrt", "pow", "fixed" };
static const mtlChars _keywords[_keywords_n] = { "const", "mutable", "if", "else", "while", "return", "true", "false", "import" };

#define _to_str(X) #X

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
	Token(p_parent, TOKEN_DECL_VAR), decl_type(NULL), expr(NULL)
{}
swsl::Token_DeclVar::~Token_DeclVar( void )
{
	delete decl_type;
	delete expr;
}

swsl::Token_DeclFn::Token_DeclFn(const swsl::Token *p_parent) :
	Token(p_parent, TOKEN_DECL_FN), decl_type(NULL)
{}
swsl::Token_DeclFn::~Token_DeclFn( void )
{
	//delete decl_type;
	mtlItem<Token*> *i = params.GetFirst();
	while (i != NULL) {
		delete i->GetItem();
		i = i->GetNext();
	}
}

swsl::Token_DefFn::Token_DefFn(const swsl::Token *p_parent) :
	Token(p_parent, TOKEN_DEF_FN), decl_type(NULL), body(NULL)
{}
swsl::Token_DefFn::~Token_DefFn( void )
{
	delete decl_type;
	mtlItem<Token*> *i = params.GetFirst();
	while (i != NULL) {
		delete i->GetItem();
		i = i->GetNext();
	}
	delete body;
}

swsl::Token_DefType::Token_DefType(const swsl::Token *p_parent) :
	Token(p_parent, TOKEN_DEF_TYPE), body(NULL)
{}
swsl::Token_DefType::~Token_DefType( void )
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
	Token(p_parent, TOKEN_READ_FN), decl_type(NULL)
{}
swsl::Token_ReadFn::~Token_ReadFn( void )
{
	//delete decl_type;
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
	//delete decl_type;
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
	for (int i = 0; i < _built_in_n; ++i) {
		if (name.Compare(_built_in_types[i], true)) { return true; }
	}
	for (int i = 0; i < _keywords_n; ++i) {
		if (name.Compare(_keywords[i], true)) { return true; }
	}
	for (int i = 0; i < _reserved_n; ++i) {
		if (name.Compare(_reserved[i], true)) { return true; }
	}
	return false;
}

bool swsl::SyntaxTreeGenerator::IsBuiltInType(const mtlChars &name)
{
	for (int i = 0; i < _built_in_n; ++i) {
		if (name.Compare(_built_in_types[i], true)) { return true; }
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
	return tok != NULL && name.Compare(tok->fn_name, true);
}

bool swsl::SyntaxTreeGenerator::CmpVarDefName(const mtlChars &name, const swsl::Token_DefType *tok)
{
	return tok != NULL && name.Compare(tok->type_name, true);
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
			case Token::TOKEN_DEF_TYPE:
				if (CmpVarDefName(name, dynamic_cast<const Token_DefType*>(t))) { return false; }
				break;
			default: break;
			}
			i = i->GetNext();
		}
	}
	return true;
}

const swsl::Token *swsl::SyntaxTreeGenerator::FindName(const mtlChars &name, const swsl::Token *parent)
{
	if (parent != NULL) {
		switch (parent->type) {
		case Token::TOKEN_BODY:
			{
				const Token_Body *body = dynamic_cast<const Token_Body*>(parent);
				const mtlItem<Token*> *iter = body->tokens.GetFirst();
				while (iter != NULL) {
					switch (iter->GetItem()->type) {
					case Token::TOKEN_DECL_VAR:
					case Token::TOKEN_DEF_TYPE:
					case Token::TOKEN_DECL_FN:
					case Token::TOKEN_DEF_FN:
						{
							const Token *t = FindName(name, iter->GetItem());
							if (t != NULL) { return t; }
						}
						break;

					default: break;
					}
					iter = iter->GetNext();
				}
			}
			break;

		case Token::TOKEN_DECL_VAR:
			{
				const Token_DeclVar *decl = dynamic_cast<const Token_DeclVar*>(parent);
				return (decl != NULL && name.Compare(decl->var_name, true)) ? decl : NULL;
			}
			break;

		case Token::TOKEN_DEF_TYPE:
			{
				const Token_DefType *def = dynamic_cast<const Token_DefType*>(parent);
				return (def != NULL && name.Compare(def->type_name, true)) ? def : NULL;
			}
			break;

		case Token::TOKEN_DEF_FN:
			{
				const Token_DefFn *def = dynamic_cast<const Token_DefFn*>(parent);
				return (def != NULL && name.Compare(def->fn_name, true)) ? def : NULL;
			}
			break;

		case Token::TOKEN_DECL_FN:
			{
				const Token_DeclFn *decl = dynamic_cast<const Token_DeclFn*>(parent);
				if (parent->parent != NULL && parent->parent->type == Token::TOKEN_DEF_FN) {
					const mtlItem<Token*> *iter = decl->params.GetFirst();
					while (iter != NULL) {
						const Token *t = FindName(name, iter->GetItem());
						if (t != NULL) { return t; }
						iter = iter->GetNext();
					}
				}
				return (decl != NULL && name.Compare(decl->fn_name, true)) ? decl : NULL;
			}
			break;

		default: break;
		}
		return FindName(name, parent->parent);
	}
	return NULL;
}

const swsl::Token_DefType *swsl::SyntaxTreeGenerator::FindDefType(const mtlChars &name, const swsl::Token *parent)
{
	const Token *token = FindName(name, parent);
	return (token != NULL && token->type == Token::TOKEN_DEF_TYPE) ? dynamic_cast<const Token_DefType*>(token) : NULL;
}

const swsl::Token_DeclFn *swsl::SyntaxTreeGenerator::FindDeclFn(const mtlChars &name, const swsl::Token *parent)
{
	const Token *token = FindName(name, parent);
	return (token != NULL && token->type == Token::TOKEN_DECL_FN) ? dynamic_cast<const Token_DeclFn*>(token) : NULL;
}

const swsl::Token_DeclVar *swsl::SyntaxTreeGenerator::FindDeclVar(const mtlChars &name, const swsl::Token *parent)
{
	const Token *token = FindName(name, parent);
	return (token != NULL && token->type == Token::TOKEN_DECL_VAR) ? dynamic_cast<const Token_DeclVar*>(token) : NULL;
}

const swsl::Token_DeclType *swsl::SyntaxTreeGenerator::FindDeclType(const mtlChars &var_name, const swsl::Token *parent)
{
	const Token_DeclVar *token = FindDeclVar(var_name, parent);
	return (token != NULL && token->decl_type != NULL && token->decl_type->type == Token::TOKEN_DECL_TYPE) ? dynamic_cast<const Token_DeclType*>(token->decl_type) : NULL;
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

swsl::Token *swsl::SyntaxTreeGenerator::ProcessDeclType(const mtlChars &rw, const mtlChars &type_name, const mtlChars &arr_size, const mtlChars &ref, const swsl::Token *parent)
{
	Token_DeclType *token = new Token_DeclType(parent);

	token->is_const = rw.Compare("mutable", true) ? false : true;
	token->type_name = type_name;
	token->is_ref = (ref.GetSize() == 1 && ref[0] == '&');
	token->is_user_def = !IsBuiltInType(token->type_name);
	// token->arr_size = ProcessArrSize(arr_size, token); // needs to convert this to compile-time constant
	return token;
}

swsl::Token *swsl::SyntaxTreeGenerator::ProcessDeclVar(const mtlChars &rw, const mtlChars &type_name, const mtlChars &arr_size, const mtlChars &ref, const mtlChars &var_name, const mtlChars &expr, const swsl::Token *parent)
{
	Token_DeclVar *token = new Token_DeclVar(parent);

	token->decl_type = ProcessDeclType(rw, type_name, arr_size, ref, token);
	token->var_name  = var_name;
	token->expr = expr.GetSize() > 0 ? ProcessExpression(expr, token) : NULL;
	token->is_ct_const = false; // TODO: determine this later. How?
	return token;
}

swsl::Token *swsl::SyntaxTreeGenerator::ProcessReadFn(const mtlChars &fn_name, const mtlChars &params, const swsl::Token *parent)
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

swsl::Token *swsl::SyntaxTreeGenerator::ProcessReadLit(const mtlChars &lit, const swsl::Token *parent)
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

swsl::Token *swsl::SyntaxTreeGenerator::ProcessReadVar(mtlSyntaxParser &var, const swsl::Token *parent)
{
	Token_ReadVar *token = new Token_ReadVar(parent);

	mtlArray<mtlChars> m;
	switch (var.Match("%w%0 %| %w. %| %w[%S]%0 %| %w[%S]. %| %s", m)) {

	case 0:
		token->var_name = m[0];
		token->decl_type = FindDeclType(token->var_name, token);
		break;

	case 1:
		token->var_name = m[0];
		token->decl_type = FindDeclType(token->var_name, token);
		if (token->decl_type != NULL && !token->decl_type->is_user_def) {
			token->mem = ProcessReadVar(var, token);
		} else {
			token->mem = ProcessError("Member(" _to_str(ProcessReadVar) ")", var.GetBufferRemaining(), token);
		}
		break;

	case 2:
		token->var_name = m[0];
		token->decl_type = FindDeclType(token->var_name, token);
		token->idx = ProcessExpression(m[0], token);
		break;

	case 3:
		token->var_name = m[0];
		token->decl_type = FindDeclType(token->var_name, token);
		if (token->decl_type != NULL && !token->decl_type->is_user_def) {
			token->mem = ProcessReadVar(var, token);
		} else {
			token->mem = ProcessError("Member(" _to_str(ProcessReadVar) ")", var.GetBufferRemaining(), token);
		}
		break;

	default:
		delete token;
		return ProcessError("Operand(" _to_str(ProcessReadVar) ")", m[0], parent);
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
		token = ProcessReadLit(val, parent);
	} else {
		mtlSyntaxParser p2;
		p2.SetBuffer(val);
		p2.EnableCaseSensitivity();
		token = ProcessReadVar(p2, parent);
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

swsl::Token *swsl::SyntaxTreeGenerator::ProcessRetType(const mtlChars &rw, const mtlChars &type_name, const mtlChars &arr_size, const mtlChars &ref, const Token *parent)
{
	Token *token = NULL;

	if (!type_name.Compare("void", true)) {
		token = ProcessDeclType(rw, type_name, arr_size, ref, parent);
	} else if (rw.GetSize() != 0 || arr_size.GetSize() != 0 || ref.GetSize() != 0) {
		token = ProcessError("Void(" _to_str(ProcessRetType) ")", "No qualifier, array, or reference allowed", parent);
	}

	return token;
}

swsl::Token *swsl::SyntaxTreeGenerator::ProcessFuncDecl(const mtlChars &rw, const mtlChars &type_name, const mtlChars &arr_size, const mtlChars &ref, const mtlChars &fn_name, const mtlChars &params, const swsl::Token *parent)
{
	Token_DeclFn *token = new Token_DeclFn(parent);

	token->fn_name = fn_name;
	token->decl_type = ProcessRetType(rw, type_name, arr_size, ref, token);
	ProcessParamDecl(params, token->params, token);
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
		token = ProcessReadFn(m[0], m[1], parent);
		break;

	case 12:
		token = ProcessOperand(m[0], parent);
		break;

	default:
		token = ProcessError("Syntax(" _to_str(ProcessExpression) ")", p.GetBuffer(), token);
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
		switch (p.Match("{%s} %| if(%S){%s} %| while(%S){%s} %| return %s; %| " _decl_str "=%S; %| " _decl_str "; %| %w=%S; %| %w(%s); %| %s", m)) {
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
			token->tokens.AddLast(ProcessDeclVar(m[_decl_qlf], m[_decl_typ], "", m[_decl_ref], m[_decl_nam], m[_decl_nam + 1], token)); // TODO: read and pass arr_size
			break;

		case 5:
			token->tokens.AddLast(ProcessDeclVar(m[_decl_qlf], m[_decl_typ], "", m[_decl_ref], m[_decl_nam], "", token)); // TODO: read and pass arr_size
			break;

		case 6:
			token->tokens.AddLast(ProcessSet(m[0], m[1], token));
			break;

		case 7:
			token->tokens.AddLast(ProcessReadFn(m[0], m[1], token));
			break;

		default:
			token->tokens.AddLast(ProcessError("Syntax(" _to_str(ProcessBody) ")", m[0], token));
			break;
		}
	}
	return token;
}

void swsl::SyntaxTreeGenerator::ProcessParamDecl(const mtlChars &params, mtlList<Token*> &out_params, const Token *parent)
{
	if (params.Compare("void", true)) { return; }
	mtlArray<mtlChars> m;
	mtlSyntaxParser p;
	p.SetBuffer(params);
	p.EnableCaseSensitivity();
	while (!p.IsEnd()) {
		switch (p.Match(_decl_str " %| %s", m)) {
		case 0:
			out_params.AddLast(ProcessDeclVar(m[_decl_qlf], m[_decl_typ], "", m[_decl_ref], m[_decl_nam], "", parent)); // TODO: read and pass arr_size
			break;
		default:
			out_params.AddLast(ProcessError("Syntax(" _to_str(ProcessFuncDecl) ")", m[0], parent));
		}
		p.Match(",");
	}
}

swsl::Token *swsl::SyntaxTreeGenerator::ProcessFuncDef(const mtlChars &rw, const mtlChars &type_name, const mtlChars &arr_size, const mtlChars &ref, const mtlChars &fn_name, const mtlChars &params, const mtlChars &body, const swsl::Token *parent)
{
	Token_DefFn *token = new Token_DefFn(parent);


	token->fn_name = fn_name;
	token->decl_type = ProcessRetType(rw, type_name, arr_size, ref, token);
	ProcessParamDecl(params, token->params, token);
	token->body = ProcessBody(body, token);
	return token;
}

swsl::Token *swsl::SyntaxTreeGenerator::ProcessTypeMemDecl(const mtlChars &decls)
{
	Token_Body *token = new Token_Body(NULL);

	mtlArray<mtlChars> m;
	mtlSyntaxParser p;
	p.SetBuffer(decls);
	p.EnableCaseSensitivity();
	while (!p.IsEnd()) {
		switch (p.Match(_decl_str "; %| %s", m)) {
		case 0:
			token->tokens.AddLast(ProcessDeclVar(m[_decl_qlf], m[_decl_typ], "", m[_decl_ref], m[_decl_nam], "", token)); // TODO: read and pass arr_size
			break;

		default:
			token->tokens.AddLast(ProcessError("Syntax(" _to_str(ProcessTypeMemDecl) ")", m[0], token));
			break;
		}
	}
	return token;
}

swsl::Token *swsl::SyntaxTreeGenerator::ProcessTypeDef(const mtlChars &struct_name, const mtlChars &decls, const swsl::Token *parent)
{
	if (VerifyName(struct_name) && NewName(struct_name, parent)) {
		Token_DefType *token = new Token_DefType(parent);

		token->type_name = struct_name;
		token->body = ProcessTypeMemDecl(decls);
		return token;
	}
	return ProcessError("Collision(" _to_str(ProcessTypeDef) ")", struct_name, parent);
}

swsl::Token *swsl::SyntaxTreeGenerator::ProcessFile(const mtlChars &contents, const swsl::Token *parent)
{
	Token_Body *token = new Token_Body(parent);

	mtlArray<mtlChars> m;
	mtlSyntaxParser p;
	p.SetBuffer(contents);
	p.EnableCaseSensitivity();
	while (!p.IsEnd()) {
		switch (p.Match(_decl_str "(%s); %| " _decl_str "(%s){%s} %| struct %w{%s}; %| import\"%S\" %| %s", m)) {
		case 0:
			token->tokens.AddLast(ProcessFuncDecl(m[_decl_qlf], m[_decl_typ], "", m[_decl_ref], m[_decl_nam], m[4], token)); // TODO: read and pass arr_size
			break;

		case 1:
			token->tokens.AddLast(ProcessFuncDef(m[_decl_qlf], m[_decl_typ], "", m[_decl_ref], m[_decl_nam], m[4], m[5], token)); // TODO: read and pass arr_size
			break;

		case 2:
			token->tokens.AddLast(ProcessTypeDef(m[0], m[1], token));
			break;

		case 3:
			token->tokens.AddLast(LoadFile(m[0], token));
			break;

		default:
			token->tokens.AddLast(ProcessError("Syntax(" _to_str(ProcessFile) ")", m[0], token));
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
		return ProcessError("File not found(" _to_str(LoadFile) ")", file_name, parent);
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
