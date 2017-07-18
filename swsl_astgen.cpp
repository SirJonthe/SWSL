#include "swsl_astgen.h"

// NOTE:
// I've removed arrays for now
// Arrays should always be declared
// Type[5] var_name := { a, b, c, d, e };

#define _decl_str "%?(const %| mutable %| readonly) %w%?(&)%w"
//#define _decl_str "%?(const %| mutable) %w%?([%S])%?(&)%w"
#define _decl_qlf 0
#define _decl_typ 1
#define _decl_ref 2
#define _decl_nam 3

#define _built_in_n 4
#define _reserved_n 16
#define _intrin_n 15
#define _keywords_n 11
enum {
	Typename_Void,
	Typename_Bool,
	Typename_Int,
	Typename_Float
};
static const mtlChars _built_in_types[_built_in_n] = { "void", "bool", "int", "float" };
static const mtlChars _reserved[_reserved_n]       = { "min", "max", "abs", "round", "trunc", "floor", "ceil", "sin", "cos", "tan", "asin", "acos", "atan", "sqrt", "pow", "fixed" };
static const mtlChars _intrin[_intrin_n]           = { "min", "max", "abs", "round", "trunc", "floor", "ceil", "sin", "cos", "tan", "asin", "acos", "atan", "sqrt", "pow" };
static const mtlChars _keywords[_keywords_n]       = { "const", "readonly", "mutable", "if", "else", "while", "return", "true", "false", "import", "struct" };

#define _const_idx    0
#define _readonly_idx 1
#define _mutable_idx  2

swsl::Token::Token(const Token *p_parent, TokenType p_type) :
	parent(p_parent), type(p_type)
{}
swsl::Token::~Token( void ) {}

int swsl::Token::CountAscend(unsigned int type_mask) const
{
	return ((type_mask & (unsigned int)type) > 0 ? 1 : 0) + (parent != NULL ? parent->CountAscend(type_mask) : 0);
}

swsl::Token_Err::Token_Err(const swsl::Token *p_parent) :
	Token(p_parent, TOKEN_ERR), line(0)
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

swsl::Token_DeclVarType::Token_DeclVarType(const Token *p_parent) :
	Token(p_parent, TOKEN_DECL_VAR_TYPE), arr_size(NULL), def_type(NULL)
{}
swsl::Token_DeclVarType::~Token_DeclVarType( void )
{
	// delete def_type;
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
	Token(p_parent, TOKEN_FILE), body(NULL)
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
	Token(p_parent, TOKEN_READ_VAR), decl_type(NULL), idx(NULL), member(NULL)
{}
swsl::Token_ReadVar::~Token_ReadVar( void )
{
	//delete decl_type;
	delete idx;
	delete member;
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

bool swsl::SyntaxTreeGenerator::IsCTConst(const Token *expr, bool &result) const
{
	if (!result)      { return false; }
	if (expr == NULL) { return true; }
	switch (expr->type) {
	case Token::TOKEN_EXPR:
		{
			const Token_Expr *e = dynamic_cast<const Token_Expr*>(expr);
			result = result && IsCTConst(e->lhs, result) && IsCTConst(e->rhs, result);
			break;
		}
	case Token::TOKEN_READ_VAR:
		{
			const Token_ReadVar *v = dynamic_cast<const Token_ReadVar*>(expr);
			result = result && (v != NULL && v->decl_type != NULL && v->decl_type->permissions == Token_DeclVarType::Constant);
			break;
		}
		result = result && dynamic_cast<const Token_ReadVar*>(expr)->decl_type->permissions == Token_DeclVarType::Constant;
		break;
	case Token::TOKEN_READ_FN:
	case Token::TOKEN_ERR:
		result = false;
		break;
	default: break;
	}
	return result;
}

const swsl::Token *swsl::SyntaxTreeGenerator::FindName(const mtlChars &name, const swsl::Token *parent)
{
	if (parent != NULL && parent->type != Token::TOKEN_DEF_TYPE) {
		if (parent->type == Token::TOKEN_BODY) {
			const Token_Body *body = dynamic_cast<const Token_Body*>(parent);
			const mtlItem<Token*> *iter = body->tokens.GetFirst();
			while (iter != NULL) {
				const Token *token = iter->GetItem();
				switch (iter->GetItem()->type) {

				case Token::TOKEN_DECL_VAR:
					{
						const Token_DeclVar *decl = dynamic_cast<const Token_DeclVar*>(iter->GetItem());
						if (decl != NULL && name.Compare(decl->var_name, true)) { return decl; }
					}
					break;

				case Token::TOKEN_DEF_TYPE:
					{
						const Token_DefType *def = dynamic_cast<const Token_DefType*>(iter->GetItem());
						if (def != NULL && name.Compare(def->type_name, true)) { return def; }
					}
					break;

				case Token::TOKEN_DEF_FN:
					{
						const Token_DefFn *def = dynamic_cast<const Token_DefFn*>(iter->GetItem());
						if (def != NULL && name.Compare(def->fn_name, true)) { return def; }
					}
					break;

				case Token::TOKEN_DECL_FN:
					{
						const Token_DeclFn *decl = dynamic_cast<const Token_DeclFn*>(iter->GetItem());
						if (parent->parent != NULL && parent->parent->type == Token::TOKEN_DEF_FN) {
							const mtlItem<Token*> *iter = decl->params.GetFirst();
							while (iter != NULL) {
								const Token *t = FindName(name, iter->GetItem());
								if (t != NULL) { return t; }
								iter = iter->GetNext();
							}
						}
						if (decl != NULL && name.Compare(decl->fn_name, true)) { return decl; }
					}
					break;

				default: break;
				}
				iter = iter->GetNext();
			}
		} else if (parent->type == Token::TOKEN_DEF_FN) {
			const Token_DefFn *def_fn = dynamic_cast<const Token_DefFn*>(parent);
			const mtlItem<Token*> *iter = def_fn->params.GetFirst();
			while (iter != NULL) {
				if (iter->GetItem()->type == Token::TOKEN_DECL_VAR) {
					const Token_DeclVar *decl = dynamic_cast<const Token_DeclVar*>(iter->GetItem());
					if (decl != NULL && name.Compare(decl->var_name, true)) { return decl; }
				}
				iter = iter->GetNext();
			}
		}
		return FindName(name, parent->parent);
	}
	return NULL;
}

const swsl::Token_DefType *swsl::SyntaxTreeGenerator::FindDefType(const mtlChars &name, const swsl::Token *parent)
{
	const Token *token = FindName(name, parent);
	return
		(token != NULL && token->type == Token::TOKEN_DEF_TYPE) ?
		dynamic_cast<const Token_DefType*>(token) :
		NULL;
}

const swsl::Token_DeclFn *swsl::SyntaxTreeGenerator::FindDeclFn(const mtlChars &name, const swsl::Token *parent)
{
	const Token *token = FindName(name, parent);
	return
		(token != NULL && token->type == Token::TOKEN_DECL_FN) ?
		dynamic_cast<const Token_DeclFn*>(token) :
		NULL;
}

const swsl::Token_DeclVar *swsl::SyntaxTreeGenerator::FindDeclVar(const mtlChars &name, const swsl::Token *parent)
{
	const Token *token = FindName(name, parent);
	return
		(token != NULL && token->type == Token::TOKEN_DECL_VAR) ?
		dynamic_cast<const Token_DeclVar*>(token) :
		NULL;
}

const swsl::Token_DeclVarType *swsl::SyntaxTreeGenerator::FindDeclVarType(const mtlChars &var_name, const swsl::Token *parent)
{
	const Token_DeclVar *token = FindDeclVar(var_name, parent);
	return
		(token != NULL && token->decl_type != NULL && token->decl_type->type == Token::TOKEN_DECL_VAR_TYPE) ?
		dynamic_cast<const Token_DeclVarType*>(token->decl_type) :
		NULL;
}

swsl::Token *swsl::SyntaxTreeGenerator::ProcessError(const mtlChars &msg, mtlChars err, int line, const swsl::Token *parent)
{
	Token_Err *token = new Token_Err(parent);

	mtlSyntaxParser p;
	p.SetBuffer(err);
	p.EnableCaseSensitivity();
	mtlArray<mtlChars> m;
	while (!p.IsEnd()) {
		const int match_num = p.Match("%s{%s} %| %s;", m);
		if (match_num > -1) {
			err = m[0];
			p.SetBuffer(err);
		} else {
			err = p.GetBufferRemaining();
			break;
		}
	}
	++root->errs;
	token->err = err;
	token->msg = msg;
	token->line = line;
	const Token *t = parent;
	while (t != NULL) {
		if (t->type == Token::TOKEN_FILE) {
			const Token_File *f = dynamic_cast<const Token_File*>(t);
			if (f != NULL) {
				token->file = f->file_name;
			}
			break;
		}
		t = t->parent;
	}
	return token;
}

swsl::Token *swsl::SyntaxTreeGenerator::ProcessDeclType(const mtlChars &rw, const mtlChars &type_name, const mtlChars &arr_size, const mtlChars &ref, int line, const swsl::Token *parent)
{
	// TODO; Process array size

	Token_DeclVarType *token = new Token_DeclVarType(parent);

	if (rw.Compare(_keywords[_const_idx]))        { token->permissions = Token_DeclVarType::Constant; }
	else if (rw.Compare(_keywords[_mutable_idx])) { token->permissions = Token_DeclVarType::ReadWrite; }
	else                                          { token->permissions = Token_DeclVarType::ReadOnly; }
	token->type_name = type_name;
	token->is_ref = (ref.GetSize() == 1 && ref[0] == '&');
	token->is_std_type = IsBuiltInType(token->type_name);
	// token->arr_size = ProcessArrSize(arr_size, token); // needs to convert this to compile-time constant
	token->def_type = FindDefType(token->type_name, token);
	if (!token->is_std_type && token->def_type == NULL) {
		delete token;
		return ProcessError("[ProcessDeclType] Undefined type", type_name, line, parent);
	}
	return token;
}

swsl::Token *swsl::SyntaxTreeGenerator::ProcessDeclVar(const mtlChars &rw, const mtlChars &type_name, const mtlChars &arr_size, const mtlChars &ref, const mtlChars &var_name, const mtlChars &expr, int line, const swsl::Token *parent)
{
	Token_DeclVar *token = new Token_DeclVar(parent);

	token->decl_type = ProcessDeclType(rw, type_name, arr_size, ref, line, token);
	token->var_name  = var_name;
	token->expr = expr.GetSize() > 0 ? ProcessExpression(expr, line, token) : NULL;
	if (token->decl_type->type == Token::TOKEN_DECL_VAR_TYPE) {
		Token_DeclVarType *t = dynamic_cast<Token_DeclVarType*>(token->decl_type);
		bool result = true;
		if (token->expr == NULL) {
			if (t->permissions != Token_DeclVarType::ReadWrite && parent->type != Token::TOKEN_DEF_TYPE && parent->type != Token::TOKEN_DEF_FN && parent->type != Token::TOKEN_DECL_FN) {
				// how to initialize struct members inside struct
					// in-place initialization is used in lieu of explicit initialization at declaration
					// mutables can be initialized in-place or at declaration
					// readonly MUST be initialized in-place OR at declaration
					// const MUST be initialized in-place
				delete token;
				return ProcessError("[ProcessDeclVar] Immutable must be initialized at declaration", var_name, line, parent);
			}
		} else if (t->permissions == Token_DeclVarType::Constant && (!IsCTConst(token->expr, result) || !t->is_std_type)) { // const, must be compile-time constant / can't declare a struct const (yet)
			delete token;
			return ProcessError("[ProcessDeclVar] Expression not compile-time constant, try \'readonly\'", expr, line, parent);
		}

		// TODO; implement feature, remove error block
		else if (!t->is_std_type) {
			delete token;
			return ProcessError("[ProcessDeclVar] Assigning a struct (not yet implemented)", var_name, line, parent);
		}
		// End error block
	}
	return token;
}

swsl::Token *swsl::SyntaxTreeGenerator::ProcessReadFn(const mtlChars &fn_name, const mtlChars &params, int line, const swsl::Token *parent)
{
	Token_ReadFn *token = new Token_ReadFn(parent);

	token->fn_name = fn_name;
	mtlArray<mtlChars> m;
	mtlSyntaxParser p;
	p.SetBuffer(params, line);
	p.EnableCaseSensitivity();
	while (!p.IsEnd()) {
		switch (p.Match("%S, %| %s", m)) {
		case 0:
		case 1:
			token->input.AddLast(ProcessExpression(m[0], p.GetLineIndex(), parent));
			break;
		default:
			delete token;
			return ProcessError("[ProcessReadFn] Syntax error", p.GetBufferRemaining(), p.GetLineIndex(), parent);
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
		token->decl_type = FindDeclVarType(token->var_name, token);
		break;

	case 1:
		token->var_name = m[0];
		token->decl_type = FindDeclVarType(token->var_name, token);
		token->member = ProcessReadVarMem(var, token);
		break;

	case 2:
		token->var_name = m[0];
		token->decl_type = FindDeclVarType(token->var_name, token);
		token->idx = ProcessExpression(m[1], var.GetLineIndex(), token);
		break;

	case 3:
		token->var_name = m[0];
		token->decl_type = FindDeclVarType(token->var_name, token);
		token->member = ProcessReadVarMem(var, token);
		token->idx = ProcessExpression(m[1], var.GetLineIndex(), token);
		break;

	default:
		delete token;
		return ProcessError("[ProcessReadVar] Syntax error", var.GetBufferRemaining(), var.GetLineIndex(), parent);
	}
	if (token->decl_type == NULL) {
		delete token;
		return ProcessError("[ProcessReadVar] Undeclared alias", m[0], var.GetLineIndex(), parent);
	}
	return token;
}

const swsl::Token_DeclVarType *swsl::SyntaxTreeGenerator::FindDeclVarMemType(const mtlChars &name, const swsl::Token_ReadVar *parent)
{
	return
		(parent->decl_type != NULL && parent->decl_type->def_type != NULL && !parent->decl_type->is_std_type) ?
		FindDeclVarType(name, parent->decl_type->def_type->body) :
		NULL;
}

swsl::Token *swsl::SyntaxTreeGenerator::ProcessReadVarMem(mtlSyntaxParser &var, const swsl::Token_ReadVar *parent)
{
	Token_ReadVar *token = new Token_ReadVar(parent);

	mtlArray<mtlChars> m;
	switch (var.Match("%w%0 %| %w. %| %w[%S]%0 %| %w[%S]. %| %s", m)) {

	case 0:
		token->var_name = m[0];
		token->decl_type = FindDeclVarMemType(token->var_name, parent);
		break;

	case 1:
		token->var_name = m[0];
		token->decl_type = FindDeclVarMemType(token->var_name, parent);
		token->member = ProcessReadVarMem(var, token);
		break;

	case 2:
		token->var_name = m[0];
		token->decl_type = FindDeclVarMemType(token->var_name, parent);
		token->idx = ProcessExpression(m[1], var.GetLineIndex(), token);
		break;

	case 3:
		token->var_name = m[0];
		token->decl_type = FindDeclVarMemType(token->var_name, parent);
		token->member = ProcessReadVarMem(var, token);
		token->idx = ProcessExpression(m[1], var.GetLineIndex(), token);
		break;

	default:
		delete token;
		return ProcessError("[ProcessReadVarMem] Syntax error", var.GetBufferRemaining(), var.GetLineIndex(), parent);
	}
	if (token->decl_type == NULL) {
		delete token;
		return ProcessError("[ProcessReadVarMem] Undeclared member alias", m[0], var.GetLineIndex(), parent);
	}
	return token;
}

swsl::Token *swsl::SyntaxTreeGenerator::ProcessOperand(const mtlChars &val, int line, const swsl::Token *parent)
{
	Token *token = NULL;

	mtlArray<mtlChars> m;
	mtlSyntaxParser p;
	p.SetBuffer(val, line);
	p.EnableCaseSensitivity();
	if (p.Match("%i%0 %| %r%0 %| true%0 %| false%0", m) > -1) {
		token = ProcessReadLit(val, parent);
	} else {
		token = ProcessReadVar(p, parent);
	}
	return token;
}

swsl::Token *swsl::SyntaxTreeGenerator::ProcessSet(const mtlChars &lhs, const mtlChars &rhs, int line, const swsl::Token *parent)
{
	Token_Set *token = new Token_Set(parent);

	token->lhs = ProcessOperand(lhs, line, token);

	Token *mem = token->lhs;
	Token_ReadVar *t = dynamic_cast<Token_ReadVar*>(mem);
	while (mem != NULL && mem->type != Token::TOKEN_ERR) {
		if (mem->type != Token::TOKEN_READ_VAR) {
			delete token->lhs;
			token->lhs = ProcessError("[ProcessSet] Assigning an immutable", lhs, line, parent);
			mem = NULL;
			t = NULL;
		} else {
			if (t->decl_type != NULL && t->decl_type->permissions != Token_DeclVarType::ReadWrite) {
				delete token->lhs;
				token->lhs = ProcessError("[ProcessSet] Assigning an immutable", lhs, line, parent);
				mem = NULL;
				t = NULL;
			}

			// TODO; Implement feature and remove this error block
			else if (t->member == NULL) {
				mem = NULL;
				t = NULL;
			}
			// End error block

			else {
				mem = t->member;
				t = dynamic_cast<Token_ReadVar*>(mem);
			}
		}
	}

	// TODO; Implement feature and remove this error block
	if (t != NULL && t->decl_type != NULL && !t->decl_type->is_std_type) {
		delete token->lhs;
		token->lhs = ProcessError("[ProcessSet] Assigning a struct (not yet implemented)", lhs, line, parent);
	}
	// End error block

	token->rhs = ProcessExpression(rhs, line, token);
	return token;
}

swsl::Token *swsl::SyntaxTreeGenerator::ProcessRetType(const mtlChars &rw, const mtlChars &type_name, const mtlChars &arr_size, const mtlChars &ref, int line, const Token *parent)
{
	Token *token = NULL;

	if (ref.GetSize() != 0) {
		token = ProcessError("[ProcessRetType] Return values can not be references", ref, line, parent);
	} else if (rw.Compare(_keywords[_const_idx], true)) {
		token = ProcessError("[ProcessRetType] Return types can not be compile time constants, try \'readonly\'", rw, line, parent);
	} else if (!type_name.Compare(_built_in_types[Typename_Void], true)) {
		token = ProcessDeclType(rw, type_name, arr_size, ref, line, parent);
	} else if (rw.GetSize() != 0) {
		token = ProcessError("[ProcessRetType] No qualifier allowed on void return type", rw, line, parent);
	} else if (arr_size.GetSize() != 0) {
		token = ProcessError("[ProcessRetType] No array allowed on void return type", arr_size, line, parent);
	}
	return token;
}

swsl::Token *swsl::SyntaxTreeGenerator::ProcessFuncDecl(const mtlChars &rw, const mtlChars &type_name, const mtlChars &arr_size, const mtlChars &ref, const mtlChars &fn_name, const mtlChars &params, int line, const swsl::Token *parent)
{
	Token_DeclFn *token = new Token_DeclFn(parent);

	token->fn_name = fn_name;
	token->decl_type = ProcessRetType(rw, type_name, arr_size, ref, line, token);
	ProcessParamDecl(params, token->params, line, token);
	return token;
}

swsl::Token *swsl::SyntaxTreeGenerator::ProcessOperation(const mtlChars &lhs, const mtlChars &op, const mtlChars &rhs, int line, const swsl::Token *parent)
{
	Token_Expr *token = new Token_Expr(parent);

	token->op = op;
	token->lhs = lhs.GetSize() > 0 ? ProcessExpression(lhs, line, token) : NULL;
	token->rhs = ProcessExpression(rhs, line, token);
	return token;
}

swsl::Token *swsl::SyntaxTreeGenerator::ProcessExpression(const mtlChars &expr, int line, const swsl::Token *parent)
{
	Token *token = NULL;

	mtlArray<mtlChars> m;
	mtlSyntaxParser p;
	p.SetBuffer(expr, line);
	p.EnableCaseSensitivity();
	switch (p.Match("%s+%S %| %s-%S %| %S*%S %| %S/%S %| %S==%S %| %S!=%S %| %S<=%S %| %S<%S %| %S>=%S %| %S>%S %| (%S)%0 %| %w(%s)%0 %| %s", m)) {
	case 0:
		token = ProcessOperation(m[0], "+", m[1], p.GetLineIndex(), parent);
		break;

	case 1:
		token = ProcessOperation(m[0], "-", m[1], p.GetLineIndex(), parent);
		break;

	case 2:
		token = ProcessOperation(m[0], "*", m[1], p.GetLineIndex(), parent);
		break;

	case 3:
		token = ProcessOperation(m[0], "/", m[1], p.GetLineIndex(), parent);
		break;

	case 4:
		token = ProcessOperation(m[0], "==", m[1], p.GetLineIndex(), parent);
		break;

	case 5:
		token = ProcessOperation(m[0], "!=", m[1], p.GetLineIndex(), parent);
		break;

	case 6:
		token = ProcessOperation(m[0], "<=", m[1], p.GetLineIndex(), parent);
		break;

	case 7:
		token = ProcessOperation(m[0], "<", m[1], p.GetLineIndex(), parent);
		break;

	case 8:
		token = ProcessOperation(m[0], ">=", m[1], p.GetLineIndex(), parent);
		break;

	case 9:
		token = ProcessOperation(m[0], ">", m[1], p.GetLineIndex(), parent);
		break;

	case 10:
		token = ProcessExpression(m[0], p.GetLineIndex(), parent);
		break;

	case 11:
		token = ProcessReadFn(m[0], m[1], p.GetLineIndex(), parent);
		break;

	case 12:
		token = ProcessOperand(m[0], p.GetLineIndex(), parent);
		break;

	default:
		token = ProcessError("[ProcessExpression] Syntax error", p.GetBufferRemaining(), p.GetLineIndex(), token);
		return token;
	}
	return token;
}

swsl::Token *swsl::SyntaxTreeGenerator::ProcessIf(const mtlChars &cond, const mtlChars &body, const swsl::Token *parent, mtlSyntaxParser &p)
{
	Token_If *token = new Token_If(parent);

	token->cond = ProcessExpression(cond, p.GetLineIndex(), token);
	token->if_body = ProcessBody(body, p.GetLineIndex(), token);
	mtlArray<mtlChars> m;
	switch (p.Match("else if(%S){%s} %| else{%s}", m)) {
	case 0:
		token->el_body = ProcessIf(m[0], m[1], token, p);
		break;

	case 1:
		token->el_body = ProcessBody(m[0], p.GetLineIndex(), token);
		break;

	default: break; // Not an error
	}
	return token;
}

swsl::Token *swsl::SyntaxTreeGenerator::ProcessWhile(const mtlChars &cond, const mtlChars &body, int line, const swsl::Token *parent)
{
	Token_While *token = new Token_While(parent);

	token->cond = ProcessExpression(cond, line, token);
	token->body = ProcessBody(body, line, token);
	return token;
}

swsl::Token *swsl::SyntaxTreeGenerator::ProcessReturn(const mtlChars &expr, int line, const swsl::Token *parent)
{
	Token_Ret *token = new Token_Ret(parent);

	const Token *p = parent;
	while (p != NULL && p->type != Token::TOKEN_DEF_FN) {
		p = p->parent;
	}

	if (p != NULL && p->type == Token::TOKEN_DEF_FN && dynamic_cast<const Token_DefFn*>(p) != NULL) {
		token->is_void = dynamic_cast<const Token_DefFn*>(p)->decl_type == NULL;
		if (token->is_void && expr.GetSize() > 0) {
			delete token;
			return ProcessError("[ProcessReturn] Return from void", expr, line, parent);
		} else {
			token->expr = expr.GetSize() > 0 ? ProcessExpression(expr, line, token) : NULL;
		}
	} else {
		delete token;
		return ProcessError("[ProcessReturn] Unknown", "Return in global scope?", line, parent);
	}
	return token;
}

swsl::Token *swsl::SyntaxTreeGenerator::ProcessBody(const mtlChars &body, int line, const swsl::Token *parent)
{
	Token_Body *token = new Token_Body(parent);

	mtlArray<mtlChars> m;
	mtlSyntaxParser p;
	p.SetBuffer(body, line);
	p.EnableCaseSensitivity();
	while (!p.IsEnd()) {

		while (p.Match(";") == 0) {}

		switch (p.Match("{%s} %| if(%S){%s} %| while(%S){%s} %| return %s; %| " _decl_str "=%S; %| " _decl_str "; %| %w(%s); %| %S=%S; %| %s", m)) {
		case 0:
			token->tokens.AddLast(ProcessBody(m[0], p.GetLineIndex(), token));
			break;

		case 1:
			token->tokens.AddLast(ProcessIf(m[0], m[1], token, p));
			break;

		case 2:
			token->tokens.AddLast(ProcessWhile(m[0], m[1], p.GetLineIndex(), token));
			break;

		case 3:
			token->tokens.AddLast(ProcessReturn(m[0], p.GetLineIndex(), token));
			break;

		case 4:
			token->tokens.AddLast(ProcessDeclVar(m[_decl_qlf], m[_decl_typ], "", m[_decl_ref], m[_decl_nam], m[_decl_nam + 1], p.GetLineIndex(), token)); // TODO: read and pass arr_size
			break;

		case 5:
			token->tokens.AddLast(ProcessDeclVar(m[_decl_qlf], m[_decl_typ], "", m[_decl_ref], m[_decl_nam], "", p.GetLineIndex(), token)); // TODO: read and pass arr_size
			break;

		case 6:
			token->tokens.AddLast(ProcessReadFn(m[0], m[1], p.GetLineIndex(), token));
			break;

		case 7:
			token->tokens.AddLast(ProcessSet(m[0], m[1], p.GetLineIndex(), token));
			break;

		default:
			token->tokens.AddLast(ProcessError("[ProcessBody] Syntax error", p.GetBufferRemaining(), p.GetLineIndex(), token));
			return token;
		}
	}
	return token;
}

void swsl::SyntaxTreeGenerator::ProcessParamDecl(const mtlChars &params, mtlList<Token*> &out_params, int line, const Token *parent)
{
	if (params.Compare(_built_in_types[Typename_Void], true)) { return; }
	mtlArray<mtlChars> m;
	mtlSyntaxParser p;
	p.SetBuffer(params, line);
	p.EnableCaseSensitivity();
	while (!p.IsEnd()) {
		switch (p.Match(_decl_str " %| %s", m)) {
		case 0:
			if (m[_decl_qlf].Compare(_keywords[_const_idx], true)) {
				out_params.AddLast(ProcessError("[ProcessFuncDecl] Parameters cannot be compile time constants, try \'readonly\'", m[_decl_nam], p.GetLineIndex(), parent));
			} else {
				out_params.AddLast(ProcessDeclVar(m[_decl_qlf], m[_decl_typ], "", m[_decl_ref], m[_decl_nam], "", p.GetLineIndex(), parent)); // TODO: read and pass arr_size
			}
			break;
		default:
			out_params.AddLast(ProcessError("[ProcessFuncDecl] Syntax error", p.GetBufferRemaining(), p.GetLineIndex(), parent));
			return;
		}
		p.Match(",");
	}
}

swsl::Token *swsl::SyntaxTreeGenerator::ProcessFuncDef(const mtlChars &rw, const mtlChars &type_name, const mtlChars &arr_size, const mtlChars &ref, const mtlChars &fn_name, const mtlChars &params, const mtlChars &body, int line, const swsl::Token *parent)
{
	Token_DefFn *token = new Token_DefFn(parent);

	token->fn_name = fn_name;
	token->decl_type = ProcessRetType(rw, type_name, arr_size, ref, line, token);
	ProcessParamDecl(params, token->params, line, token);
	token->body = ProcessBody(body, line, token);
	return token;
}

swsl::Token *swsl::SyntaxTreeGenerator::ProcessTypeMemDecl(const mtlChars &decls, int line, const swsl::Token *parent)
{
	// Token_Body *token = new Token_Body(NULL); // used this before to prevent name searches inside struct scope to look for names outside of scope if not found inside scope
	Token_Body *token = new Token_Body(parent);

	mtlArray<mtlChars> m;
	mtlSyntaxParser p;
	p.SetBuffer(decls, line);
	p.EnableCaseSensitivity();
	while (!p.IsEnd()) {
		switch (p.Match(_decl_str "; %| %s", m)) {
		case 0:
			token->tokens.AddLast(ProcessDeclVar(m[_decl_qlf], m[_decl_typ], "", m[_decl_ref], m[_decl_nam], "", p.GetLineIndex(), token)); // TODO: read and pass arr_size
			break;

		default:
			token->tokens.AddLast(ProcessError("[ProcessTypeMemDecl] Syntax error", p.GetBufferRemaining(), p.GetLineIndex(), token));
			return token;
		}
	}
	return token;
}

swsl::Token *swsl::SyntaxTreeGenerator::ProcessTypeDef(const mtlChars &struct_name, const mtlChars &decls, int line, const swsl::Token *parent)
{
	if (VerifyName(struct_name) && NewName(struct_name, parent)) {
		Token_DefType *token = new Token_DefType(parent);

		token->type_name = struct_name;
		token->body = ProcessTypeMemDecl(decls, line, parent);
		return token;
	}
	return ProcessError("[ProcessTypeDef] Illegal/colliding naming", struct_name, line, parent);
}

void swsl::SyntaxTreeGenerator::RemoveComments(mtlString &code)
{
	const int size = code.GetSize();
	int comment_token = 0;
	for (int i = 0; i < size; ++i) {
		if (comment_token == 2) {
			if (!mtlChars::IsNewline(code[i])) { code[i] = ' '; }
			else                               { comment_token = 0; }
		} else {
			comment_token = code[i] == '/' ? comment_token + 1 : 0;
			if (comment_token == 2) {
				code[i - 1] = ' ';
				code[i]     = ' ';
			}
		}
	}
}

swsl::Token *swsl::SyntaxTreeGenerator::ProcessFile(const mtlChars &contents, const swsl::Token *parent)
{
	Token_Body *token = new Token_Body(parent);

	mtlArray<mtlChars> m;
	mtlSyntaxParser p;
	p.SetBuffer(contents);
	p.EnableCaseSensitivity();
	while (!p.IsEnd()) {

		while (p.Match(";") == 0) {}

		switch (p.Match(_decl_str "(%s); %| " _decl_str "(%s){%s} %| struct %w{%s} %| import\"%S\" %| %s", m)) {
		case 0:
			token->tokens.AddLast(ProcessFuncDecl(m[_decl_qlf], m[_decl_typ], "", m[_decl_ref], m[_decl_nam], m[4], p.GetLineIndex(), token)); // TODO: read and pass arr_size
			break;

		case 1:
			token->tokens.AddLast(ProcessFuncDef(m[_decl_qlf], m[_decl_typ], "", m[_decl_ref], m[_decl_nam], m[4], m[5], p.GetLineIndex(), token)); // TODO: read and pass arr_size
			break;

		case 2:
			token->tokens.AddLast(ProcessTypeDef(m[0], m[1], p.GetLineIndex(), token));
			break;

		case 3:
			token->tokens.AddLast(LoadFile(m[0], p.GetLineIndex(), token));
			break;

		default:
			token->tokens.AddLast(ProcessError("[ProcessFile] Syntax error", p.GetBufferRemaining(), p.GetLineIndex(), token));
			return token;
		}
	}
	return token;
}

swsl::Token *swsl::SyntaxTreeGenerator::LoadFile(const mtlChars &file_name, int line, const swsl::Token *parent)
{
	Token_File *token = new Token_File(parent);

	if (!mtlSyntaxParser::BufferFile(file_name, token->content)) {
		delete token;
		return ProcessError("[LoadFile] File not found", file_name, line, parent);
	}
	RemoveComments(token->content);
	token->file_name = file_name;
	token->body = ProcessFile(token->content, token);
	return token;
}

swsl::SyntaxTree *swsl::SyntaxTreeGenerator::Generate(const mtlChars &entry_file)
{
	root = new SyntaxTree();

	root->file = LoadFile(entry_file, 1, root);
	return root;
}
