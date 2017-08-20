#include "swsl_astgen_new.h"

// float assignment expects MEM_OP FLOAT_EXPR
// int assignment expects MEM_OP INT_EXPR
// bool assignment expexts MEM_OP BOOL_EXPR
// bool operations expects == != < > <= >= expects INT_EXPR MATH_OP INT_EXPR / FLOAT_EXPR MATH_OP FLOAT_EXPR / BOOL_EXPR MATH_OP BOOL_EXPR
// bool operations AND and OR expects BOOL_EXPR MATH_OP BOOL_EXPR

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
static const mtlChars _keywords[_keywords_n]       = { "lit", "imm", "var", "if", "else", "while", "return", "true", "false", "import", "struct" };

new_SyntaxTreeGenerator::Parser::Parser(const mtlChars &buffer)
{
	p.SetBuffer(buffer);
	p.EnableCaseSensitivity();
}

bool new_SyntaxTreeGenerator::Parser::IsEnd( void ) const
{
	return p.IsEnd();
}

int new_SyntaxTreeGenerator::Parser::Match(const mtlChars &expr)
{
	return p.Match(expr, m, &s);
}

mtlChars new_SyntaxTreeGenerator::Parser::GetMatch(int i) const
{
	if (i >= m.GetSize() || i <  0) { return p.GetBufferRemaining(); }
	return m[i];
}

mtlChars new_SyntaxTreeGenerator::Parser::Seq( void ) const
{
	return s;
}

mtlChars new_SyntaxTreeGenerator::Parser::Rem( void ) const
{
	return p.GetBufferRemaining();
}

new_Token::new_Token(const mtlChars &str_, Type type_, const new_Token *parent_) :
	str(str_), type(type_), parent(parent_), sub(NULL), next(NULL), ref(NULL)
{}
new_Token::~new_Token( void )
{
	// don't delete parent
	delete sub;
	delete next;
	// don't delete ref
}

int new_Token::CountAscend(unsigned int type_mask) const
{
	return ((type_mask & (unsigned int)type) > 0 ? 1 : 0) + (parent != NULL ? parent->CountAscend(type_mask) : 0);
}

int new_Token::CountDescend(unsigned int type_mask) const
{
	return
		((type_mask & (unsigned int)type) > 0 ? 1 : 0) +
		(sub != NULL ? sub->CountDescend(type_mask) : 0) +
		(next != NULL ? next->CountDescend(type_mask) : 0);
}

bool new_SyntaxTreeGenerator::IsReserved(const mtlChars &name) const
{
	if (IsBuiltInType(name))    { return true; }
	if (IsKeyword(name))        { return true; }
	if (IsUnusedReserved(name)) { return true; }
	return false;
}

bool new_SyntaxTreeGenerator::IsBuiltInType(const mtlChars &name) const
{
	for (int i = 0; i < _built_in_n; ++i) {
		if (name.Compare(_built_in_types[i], true)) { return true; }
	}
	return false;
}

bool new_SyntaxTreeGenerator::IsKeyword(const mtlChars &name) const
{
	for (int i = 0; i < _keywords_n; ++i) {
		if (name.Compare(_keywords[i], true)) { return true; }
	}
	return true;
}

bool new_SyntaxTreeGenerator::IsUnusedReserved(const mtlChars &name) const
{
	for (int i = 0; i < _reserved_n; ++i) {
		if (name.Compare(_reserved[i], true)) { return true; }
	}
	return false;
}

bool new_SyntaxTreeGenerator::IsValidNameConvention(const mtlChars &name) const
{
	if (name.GetSize() > 0 && !IsReserved(name)) {
		for (int i = 1; i < name.GetSize(); ++i) {
			if (!mtlChars::IsAlphanumeric(name[i]) && name[i] != '_') { return false; }
		}
		return mtlChars::IsAlpha(name[0]) || name[0] == '_';
	}
	return false;
}

bool new_SyntaxTreeGenerator::IsNewName(const mtlChars &name, const new_Token *parent) const
{
	return FindName(name, parent) == NULL;
}

const new_Token *new_SyntaxTreeGenerator::FindName(const mtlChars &name, const new_Token *token) const
{
	// hops backward to parent scope if no match found, and continues search
	while (token != NULL && token->type != new_Token::SCOPE) {
		token = token->parent;
	}

	if (token != NULL && token->type == new_Token::SCOPE) {
		const new_Token *statement = token->sub;
		while (statement != NULL) {
			switch (statement->type) {
			case new_Token::FN_DECL:
			case new_Token::FN_DEF:
			case new_Token::VAR_DECL:
			case new_Token::TYPE_DEF:
				{
					new_Token *t = statement->sub;
					while (t != NULL) {
						if (t->type == new_Token::USR_NAME && t->str.Compare(name, true)) {
							return statement;
						}
						t = t->next;
					}
				}
				break;
			default: break;
			}
			statement = statement->next;
		}
	} else {
		return NULL;
	}
	return FindName(name, token->parent);
}

bool new_SyntaxTreeGenerator::IsValidName(const mtlChars &name, const new_Token *parent) const
{
	return
		IsValidNameConvention(name) &&
		!IsReserved(name) &&
		IsNewName(name, parent);
}

bool new_SyntaxTreeGenerator::IsCTConst(const new_Token *expr, bool &result) const
{
	if (!result)                               { return false; }
	if (expr == NULL)                          { return true; }
	if (expr->str.GetTrimmed().GetSize() == 0) { return true; }
	switch (expr->type) {
	case new_Token::BOOL_EXPR:
	case new_Token::INT_EXPR:
	case new_Token::FLOAT_EXPR:
	case new_Token::EXPR:
		{
			result = result && IsCTConst(expr->sub, result) && IsCTConst(expr->next, result);
			break;
		}
	case new_Token::MEM_OP:
		{
			result =
				result &&
				(
					expr->ref != NULL &&
					expr->ref->type == new_Token::VAR_DECL &&
					expr->ref->sub != NULL &&
					expr->ref->sub->type == new_Token::TYPE_TRAIT &&
					expr->ref->sub->str.Compare("lit", true)
				);
			break;
		}
		break;
	case new_Token::BOOL_OP:
	case new_Token::INT_OP:
	case new_Token::FLOAT_OP:
		result = true;
		break;
	case new_Token::MATH_OP:
		result = result && IsCTConst(expr->next, result);
		break;
	case new_Token::FN_OP:
	case new_Token::ERR:
		result = false;
		break;
	default: break;
	}
	return result;
}

bool new_SyntaxTreeGenerator::IsCTConst(const new_Token *expr) const
{
	bool result = true;
	return IsCTConst(expr, result);
}

new_Token *new_SyntaxTreeGenerator::ProcessDefType(const mtlChars &type_name, const mtlChars &decls, const new_Token *parent) const
{
	new_Token *token = new new_Token("", new_Token::TYPE_DEF, parent);

	new_Token **t = &token->sub;
	*t = ProcessNewName(type_name, token);
	t = &(*t)->next;
	*t = new new_Token("", new_Token::SCOPE, token);
	t = &(*t)->sub;
	Parser p(decls);
	while (!p.IsEnd()) {
		while (p.Match(";") == 0) {}
		if (p.Match("%?(var %| imm)%w%w;")) {
			*t = ProcessDeclVar(p.GetMatch(0), p.GetMatch(1), "", p.GetMatch(2), "", token);
		} else {
			*t = ProcessError("[ProcessDefType] Syntax error", p.Rem(), token);
		}
		t = &(*t)->next;
	}

	return token;
}

new_Token *new_SyntaxTreeGenerator::ProcessTrait(const mtlChars &trait, const new_Token *parent) const
{
	if (trait.GetTrimmed().GetSize() == 0) {
		return new new_Token("imm", new_Token::TYPE_TRAIT, parent);
	} else if (trait.Compare("var", true) || trait.Compare("imm", true) || trait.Compare("lit", true)) {
		return new new_Token(trait, new_Token::TYPE_TRAIT, parent);
	}
	return ProcessError("[ProcessTrait] Unknown type trait", trait, parent);
}

new_Token *new_SyntaxTreeGenerator::ProcessType(const mtlChars &type_name, const new_Token *parent) const
{
	const new_Token *ref = FindName(type_name, parent);
	new_Token *token = NULL;
	if (ref == NULL || ref->parent->type != new_Token::TYPE_DEF) {
		for (int i = 0; i < _built_in_n; ++i) {
			if (type_name.Compare(_built_in_types[i], true)) {
				token = new new_Token(type_name, new_Token::TYPE_NAME, parent);
				token->ref = token;
				break;
			}
		}
		if (token == NULL) {
			token = ProcessError("[ProcessType] Undeclared type", type_name, parent);
		}
	} else {
		new_Token *token = new new_Token(type_name, new_Token::TYPE_NAME, parent);
		token->ref = ref->parent;
	}
	return token;
}

new_Token *new_SyntaxTreeGenerator::ProcessIf(const mtlChars &cond, const mtlChars &body, const new_Token *parent, Parser &p) const
{
	new_Token *token = new new_Token("", new_Token::IF, parent);

	new_Token **t = &token->sub;
	*t = ProcessExpr(cond, token);
	t = &(*t)->next;
	*t = ProcessScope(body, token);
	t = &(*t)->next;
	switch (p.Match("else if(%S){%s} %| else{%s}")) {
	case 0:
		*t = ProcessIf(p.GetMatch(0), p.GetMatch(1), token, p);
		break;

	case 1:
		*t = ProcessScope(p.GetMatch(0), token);
		break;

	default: break; // Not an error
	}
	return token;
}

new_Token *new_SyntaxTreeGenerator::ProcessWhile(const mtlChars &cond, const mtlChars &body, const new_Token *parent) const
{
	new_Token *token = new new_Token("", new_Token::WHILE, parent);

	new_Token **t = &token->sub;
	*t = ProcessExpr(cond, token);
	t = &(*t)->next;
	*t = ProcessScope(body, token);
	return token;
}

new_Token *new_SyntaxTreeGenerator::ProcessRet(const mtlChars &expr, const new_Token *parent) const
{
	new_Token *token = new new_Token("", new_Token::WHILE, parent);

	const new_Token *p = parent;
	while (p != NULL && p->type != new_Token::FN_DEF) {
		p = p->parent;
	}

	if (p != NULL && p->type == new_Token::FN_DEF) {
		bool is_void = token->sub->next->type == new_Token::TYPE_NAME && token->sub->next->str.Compare("void", true);
		if (is_void && expr.GetSize() > 0) {
			delete token;
			return ProcessError("[ProcessRet] Return from void", expr, parent);
		} else {
			token->next = expr.GetSize() > 0 ? ProcessExpr(expr, token) : NULL;
		}
	} else {
		delete token;
		return ProcessError("[ProcessRet] Unknown", "Return in global scope?", parent);
	}
	return token;
}

new_Token *new_SyntaxTreeGenerator::ProcessDeclVar(const mtlChars &trait, const mtlChars &type_name, const mtlChars &arr_size, const mtlChars &var_name, const mtlChars &expr, const new_Token *parent) const
{
	new_Token *token = new new_Token("", new_Token::VAR_DECL, parent);

	new_Token **t = &token->sub;
	*t = ProcessTrait(trait, token);
	t = &(*t)->next;
	*t = ProcessType(type_name, token);
	t = &(*t)->next;
	*t = ProcessArraySize(arr_size, token);
	t = &(*t)->next;
	*t = ProcessNewName(var_name, token);
	t = &(*t)->next;
	*t = ProcessExpr(expr, token);
	return token;
}

new_Token *new_SyntaxTreeGenerator::ProcessScope(const mtlChars &scope, const new_Token *parent) const
{
	new_Token *token = new new_Token(scope, new_Token::SCOPE, parent);

	Parser p(scope);
	new_Token **t = &token->sub;
	while (!p.IsEnd()) {
		while (p.Match(";") == 0) {}

		//*t =
		//	TryProcessScope(p, token) ||
		//	TryProcessIf(p, token) ||
		//	TryProcessWhile(p, token) ||
		//	TryProcessRet(p, token) ||
		//	TryProcessDeclVar(p, token) ||
		//	TryProcessExpr(p, token) ||
		//	ProcessError("[ProcessScope] Syntax error", p.Rem(), token);
		//t = &(*t)->next;

		switch (p.Match("{%s} %| if(%S){%s} %| while(%S){%s} %| return %s; %| %?(var %| imm %| lit)%w[%S]%w=%S; %| %?(var %| imm %| lit)%w%w=%S; %| %?(var %| imm %| lit)%w[%S]%w; %| %?(var %| imm %| lit)%w%w; %| %S; %| %s")) {
		case 0:
			*t = ProcessScope(p.GetMatch(0), token);
			break;

		case 1:
			*t = ProcessIf(p.GetMatch(0), p.GetMatch(1), token, p);
			break;

		case 2:
			*t = ProcessWhile(p.GetMatch(0), p.GetMatch(1), token);
			break;

		case 3:
			*t = ProcessRet(p.GetMatch(0), token);
			break;

		case 4:
			*t = ProcessDeclVar(p.GetMatch(0), p.GetMatch(1), p.GetMatch(2), p.GetMatch(3), p.GetMatch(4), token);
			break;

		case 5:
			*t = ProcessDeclVar(p.GetMatch(0), "", p.GetMatch(1), p.GetMatch(2), p.GetMatch(3), token);
			break;

		case 6:
			*t = ProcessDeclVar(p.GetMatch(0), p.GetMatch(1), p.GetMatch(2), p.GetMatch(3), "", token);
			break;

		case 7:
			*t = ProcessDeclVar(p.GetMatch(0), "", p.GetMatch(1), p.GetMatch(2), "", token);
			break;

		case 8:
			*t = ProcessExpr(p.GetMatch(0), token);
			break;

		default:
			*t = ProcessError("[ProcessScope] Syntax error", p.Rem(), token);
		}
		t = &(*t)->next;
	}
	return token;
}

new_Token *new_SyntaxTreeGenerator::ProcessExpr(const mtlChars &expr, const new_Token *parent) const
{
	// TODO: Token sequence should be S-expr; MATH_OP -> LVAL -> RVAL (next ptrs)
	// LVAL and RVAL are recursively divided into S-exprs via sub ptr

	// FIX: Only root expr token need to be EXPR, leaves can be MEM_OP / BOOL_OP / INT_OP / FLOAT_OP / FN_OP

	new_Token *token = new new_Token(expr, new_Token::EXPR, parent);

	Parser p(expr);
	new_Token **t = &token->sub;
	if (p.Match("%S,") == 0) {
		do {
			*t = ProcessExpr(p.GetMatch(0), token);
			t = &(*t)->next;
		} while (p.Match("%S, %| %S") > 0);
	} else {
		switch (p.Match("%S=%S %| %s+%S %| %s-%S %| %S*%S %| %S/%S %| %S==%S %| %S!=%S %| %S<=%S %| %S<%S %| %S>=%S %| %S>%S %| (%S)%0 %| %s")) {
		case 0:
			ProcessMathOp(p.GetMatch(0), "=", p.GetMatch(1), token);
			break;

		case 1:
			*t = ProcessMathOp(p.GetMatch(0), "+", p.GetMatch(1), token);
			break;

		case 2:
			*t = ProcessMathOp(p.GetMatch(0), "-", p.GetMatch(1), token);
			break;

		case 3:
			*t = ProcessMathOp(p.GetMatch(0), "*", p.GetMatch(1), token);
			break;

		case 4:
			*t = ProcessMathOp(p.GetMatch(0), "/", p.GetMatch(1), token);
			break;

		case 5:
			*t = ProcessMathOp(p.GetMatch(0), "==", p.GetMatch(1), token);
			break;

		case 6:
			*t = ProcessMathOp(p.GetMatch(0), "!=", p.GetMatch(1), token);
			break;

		case 7:
			*t = ProcessMathOp(p.GetMatch(0), "<=", p.GetMatch(1), token);
			break;

		case 8:
			*t = ProcessMathOp(p.GetMatch(0), "<", p.GetMatch(1), token);
			break;

		case 9:
			*t = ProcessMathOp(p.GetMatch(0), ">=", p.GetMatch(1), token);
			break;

		case 10:
			*t = ProcessMathOp(p.GetMatch(0), ">", p.GetMatch(1), token);
			break;

		case 11:
			*t = ProcessExpr(p.GetMatch(0), token);
			break;

		case 12:
			*t = ProcessMemOp(p.GetMatch(0), token);
			break;

		default:
			*t = ProcessError("[ProcessExpr] Syntax error", p.Rem(), token);
			return token;
		}
	}
	return token;
}

new_Token *new_SyntaxTreeGenerator::ProcessMemOp(const mtlChars &op, const new_Token *parent) const
{
	new_Token *token = NULL;

	Parser var(op);
	switch (var.Match("true%0 %| false%0 %| %i.%i%0 %| %i%0 %| %w%0 %| %w. %| %w[%S]%0 %| %w[%S]. %| %w(%s)%0 %| %w(%s). %| %w(%s)[%S]%0 %| %w(%s)[%S]. %| %s")) {

	case 0:
	case 1:
		token = new new_Token(var.GetMatch(0), new_Token::BOOL_OP, parent);
		break;

	case 2:
		token = new new_Token(var.Seq(), new_Token::FLOAT_OP, parent);
		break;

	case 3:
		token = new new_Token(var.GetMatch(0), new_Token::INT_OP, parent);
		break;

	case 4:
		token = new new_Token(var.Seq(), new_Token::MEM_OP, parent);
		token->sub = ProcessRefName(var.GetMatch(0), token);
		break;

	case 5:
		token = new new_Token(var.Seq(), new_Token::MEM_OP, parent);
		token->sub = ProcessRefName(var.GetMatch(0), token);
		token->sub->next = ProcessMemOpMember(var, token);
		break;

	case 6:
		token = new new_Token(var.Seq(), new_Token::MEM_OP, parent);
		token->sub = ProcessRefName(var.GetMatch(0), token);
		token->sub->next = ProcessExpr(var.GetMatch(1), token);
		break;

	case 7:
		token = new new_Token(var.Seq(), new_Token::MEM_OP, parent);
		token->sub = ProcessRefName(var.GetMatch(0), token);
		token->sub->next = ProcessMemOpMember(var, token);
		token->sub->next->next = ProcessExpr(var.GetMatch(1), token);
		break;

	case 8:
		token = new new_Token(var.Seq(), new_Token::FN_OP, parent);
		token->sub = ProcessRefName(var.GetMatch(0), token);
		token->sub->next = ProcessExpr(var.GetMatch(1), token);
		break;

	case 9:
		token = new new_Token(var.Seq(), new_Token::FN_OP, parent);
		token->sub = ProcessRefName(var.GetMatch(0), token);
		token->sub->next = ProcessExpr(var.GetMatch(1), token);
		token->sub->next->next = ProcessMemOpMember(var, token);
		break;

	case 10:
		token = new new_Token(var.Seq(), new_Token::FN_OP, parent);
		token->sub = ProcessRefName(var.GetMatch(0), token);
		token->sub->next = ProcessExpr(var.GetMatch(1), token);
		token->sub->next->next = ProcessExpr(var.GetMatch(2), token);
		break;

	case 11:
		token = new new_Token(var.Seq(), new_Token::FN_OP, parent);
		token->sub = ProcessRefName(var.GetMatch(0), token);
		token->sub->next = ProcessExpr(var.GetMatch(1), token);
		token->sub->next->next = ProcessExpr(var.GetMatch(2), token);
		token->sub->next->next->next = ProcessMemOpMember(var, token);
		break;

	default:
		token = ProcessError("[ProcessMemOp] Syntax error", var.Rem(), parent);
	}
	return token;
}

new_Token *new_SyntaxTreeGenerator::ProcessMemOpMember(new_SyntaxTreeGenerator::Parser &var, const new_Token *parent) const
{
	new_Token *token = NULL;

	switch (var.Match("%w%0 %| %w. %| %w[%S]%0 %| %w[%S]. %| %s")) {

	case 0:
		token = new new_Token(var.Seq(), new_Token::MEM_OP, parent);
		token->sub = ProcessRefMemName(var.GetMatch(0), token);
		break;

	case 1:
		token = new new_Token(var.Seq(), new_Token::MEM_OP, parent);
		token->sub = ProcessRefMemName(var.GetMatch(0), token);
		token->sub->next = ProcessMemOpMember(var, token);
		break;

	case 2:
		token = new new_Token(var.Seq(), new_Token::MEM_OP, parent);
		token->sub = ProcessRefMemName(var.GetMatch(0), token);
		token->sub->next = ProcessExpr(var.GetMatch(1), token);
		break;

	case 3:
		token = new new_Token(var.Seq(), new_Token::MEM_OP, parent);
		token->sub = ProcessRefMemName(var.GetMatch(0), token);
		token->sub->next = ProcessExpr(var.GetMatch(1), token);
		token->sub->next->next = ProcessMemOpMember(var, token);
		break;

	default:
		token = ProcessError("[ProcessMemOpMember] Syntax error", var.Rem(), parent);
	}
	return token;
}

new_Token *new_SyntaxTreeGenerator::ProcessMathOp(const mtlChars &lhs, const mtlChars &op, const mtlChars &rhs, const new_Token *parent) const
{
	new_Token *token = new new_Token(op, new_Token::MATH_OP, parent);

	token->sub = ProcessExpr(lhs, parent);
	token->sub->next = ProcessExpr(rhs, parent);
	return token;
}

new_Token *new_SyntaxTreeGenerator::ProcessNewName(const mtlChars &name, const new_Token *parent) const
{
	if (!IsValidName(name, parent)) {
		return ProcessError("[ProcessNewName] Name collision", name, parent);
	}
	return new new_Token(name, new_Token::USR_NAME, parent);
}

new_Token *new_SyntaxTreeGenerator::ProcessRefName(const mtlChars &name, const new_Token *parent) const
{
	const new_Token *ref = FindName(name, parent);
	new_Token *token;
	if (ref != NULL) {
		token = new new_Token(name, new_Token::USR_NAME, parent);
		token->ref = ref;
	} else {
		token = ProcessError("[ProcessRefName] Undeclared alias", name, parent);
	}
	return token;
}

new_Token *new_SyntaxTreeGenerator::ProcessRefMemName(const mtlChars &name, const new_Token *parent) const
{
	const new_Token *ref = NULL;
	if (parent != NULL && parent->type == new_Token::TYPE_DEF) {
		new_Token *t = parent->sub;
		while (t != NULL && t->type != new_Token::SCOPE) {
			t = t->next;
		}
		if (t != NULL && t->type == new_Token::SCOPE) {
			ref = FindName(name, t);
			if (ref != NULL && ref->parent != t) {
				ref = NULL;
			}
		}
	}
	new_Token *token;
	if (ref != NULL) {
		token = new new_Token(name, new_Token::USR_NAME, parent);
		token->ref = ref;
	} else {
		token = ProcessError("[ProcessRefMemName] Undeclared alias", name, parent);
	}
	return token;
}

new_Token *new_SyntaxTreeGenerator::ProcessConstExpr(const mtlChars &expr, const new_Token *parent) const
{
	new_Token *token = ProcessExpr(expr, parent);
	if (!IsCTConst(token)) {
		delete token;
		token = ProcessError("[ProcessConstExpr] Expression not compile-time constant", expr, parent);
	}
	return token;
}

new_Token *new_SyntaxTreeGenerator::ProcessArraySize(const mtlChars &arr_size, const new_Token *parent) const
{
	// TODO: Make sure arr_size evals to > 0
	return ProcessConstExpr(arr_size.GetTrimmed().GetSize() > 0 ? arr_size : "1", parent);
}

void new_SyntaxTreeGenerator::ProcessDeclParam(new_Token **token, const mtlChars &params, const new_Token *parent) const
{
	Parser p(params);
	*token = new new_Token(params, new_Token::SCOPE, parent);
	token = &(*token)->sub;
	if (p.Match("void%0") < 0) {
		while (!p.IsEnd()) {
			const int match = p.Match("%?(var %| imm)%w[%s]%w %| %?(var %| imm)%w%w");
			if (match >= 0 && ((p.Match(",") == 0 || p.IsEnd()))) {
				switch (match) {
				case 0:
					*token = ProcessDeclVar(p.GetMatch(0), p.GetMatch(1), p.GetMatch(2), p.GetMatch(3), "", parent);
					break;
				case 1:
					*token = ProcessDeclVar(p.GetMatch(0), p.GetMatch(1), "", p.GetMatch(2), "", parent);
					break;
				}
				token = &(*token)->next;
			} else {
				*token = ProcessError("[ProcessDeclParam] Syntax error", p.Rem(), parent);
				token = &(*token)->next;
				break;
			}
		}
	}
}

new_Token *new_SyntaxTreeGenerator::ProcessDeclFn(const mtlChars &type_name, const mtlChars &arr_size, const mtlChars &fn_name, const mtlChars &params, const new_Token *parent) const
{
	new_Token *token = new new_Token("", new_Token::FN_DECL, parent);

	// TODO: Do a search for the name
	new_Token **t = &token->sub;
	*t = ProcessType(type_name, token);
	t = &(*t)->next;
	*t = ProcessArraySize(arr_size, token);
	t = &(*t)->next;
	*t = ProcessNewName(fn_name, token);
	t = &(*t)->next;
	ProcessDeclParam(t, params, token);

	return token;
}

new_Token *new_SyntaxTreeGenerator::ProcessDefFn(const mtlChars &type_name, const mtlChars &arr_size, const mtlChars &fn_name, const mtlChars &params, const mtlChars &body, const new_Token *parent) const
{
	new_Token *token = new new_Token("", new_Token::FN_DEF, parent);

	token->ref = FindName(fn_name, parent);
	// TODO; Assign token->ref->ref to token if not already set (and if token->ref->type == FN_DECL)
	new_Token **t = &token->sub;
	*t = ProcessType(type_name, token);
	t = &(*t)->next;
	*t = ProcessArraySize(arr_size, token);
	t = &(*t)->next;
	*t = ProcessNewName(fn_name, token); // This may cause a conflict if there is already an FN_DECL and should be allowed
	t = &(*t)->next;
	ProcessDeclParam(t, params, token);
	// ProcessDeclParam already increments t, no need to do it again
	*t = ProcessScope(body, token);

	return token;
}

new_Token *new_SyntaxTreeGenerator::ProcessSet(const mtlChars &lhs, const mtlChars &rhs, const new_Token *parent) const
{
	new_Token *token = new new_Token("", new_Token::SET, parent);

	new_Token **t = &token->sub;
	*t = ProcessMemOp(lhs, token);
	t = &(*t)->sub;
	new_Token **rhs_tok = &(*t)->next;
	while (*t != NULL && (*t)->type != new_Token::ERR) {
		if ((*t)->type != new_Token::MEM_OP) {
			delete *t;
			*t = ProcessError("[ProcessSet] Assigning an immutable", lhs, parent);
		} else {
			if ((*t)->ref != NULL && !(*t)->ref->sub->str.Compare("mutable", true)) {
				delete *t;
				*t = ProcessError("[ProcessSet] Assigning an immutable", lhs, parent);
			}

			// TODO; Implement feature and remove this error block
			//else if ((*t)->next == NULL) {
			//	*t = NULL;
			//}
			// End error block

			else {
				t = &(*t)->next;
			}
		}
	}

	// TODO; Implement feature and remove this error block
	if (*t != NULL && (*t)->ref != NULL && !IsBuiltInType((*t)->ref->sub->str)) {
		delete *t;
		*t = ProcessError("[ProcessSet] Assigning a struct (not yet implemented)", lhs, parent);
	}
	// End error block

	*rhs_tok = ProcessExpr(rhs, token);
	return token;
}

new_Token *new_SyntaxTreeGenerator::ProcessFile(const mtlChars &file_contents, const new_Token *parent) const
{
	new_Token *token = new new_Token(file_contents, new_Token::SCOPE, parent);

	Parser p(file_contents);
	new_Token **t = &token->sub;
	while (!p.IsEnd()) {
		while (p.Match(";") == 0) {}
		switch (p.Match("%w[%S]%w(%s); %| %w%w(%s); %| %w[%S]%w(%s){%s} %| %w%w(%s){%s} %| struct %w{%s} %| import\"%S\" %| %s")) {
		case 0:
			*t = ProcessDeclFn(p.GetMatch(0), p.GetMatch(1), p.GetMatch(2), p.GetMatch(3), token);
			break;

		case 1:
			*t = ProcessDeclFn(p.GetMatch(0), "", p.GetMatch(1), p.GetMatch(2), token);
			break;

		case 2:
			*t = ProcessDefFn(p.GetMatch(0), p.GetMatch(1), p.GetMatch(2), p.GetMatch(3), p.GetMatch(4), token);
			break;

		case 3:
			*t = ProcessDefFn(p.GetMatch(0), "", p.GetMatch(1), p.GetMatch(2), p.GetMatch(3), token);
			break;

		case 4:
			*t = ProcessDefType(p.GetMatch(0), p.GetMatch(1), token);
			break;

		case 5:
			*t = LoadFile(p.GetMatch(0), token);
			break;

		default:
			*t = ProcessError("[ProcessFile] Syntax error", p.GetMatch(0), token);
			return token;
		}
		t = &(*t)->next;
	}
	return token;
}

void new_SyntaxTreeGenerator::RemoveComments(mtlString &code) const
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

new_Token *new_SyntaxTreeGenerator::LoadFile(const mtlChars &file_name, const new_Token *parent) const
{
	new_Token *token = new new_Token(file_name, new_Token::FILE, parent);

	if (!mtlSyntaxParser::BufferFile(file_name, token->buffer)) {
		delete token;
		return ProcessError("[LoadFile] File not found", file_name, parent);
	}
	RemoveComments(token->buffer);
	token->sub = ProcessFile(token->buffer, token);
	return token;
}

new_Token *new_SyntaxTreeGenerator::ProcessError(const mtlChars &msg, mtlChars err, const new_Token *parent) const
{
	new_Token *token = new new_Token("", new_Token::ERR, parent);

	mtlSyntaxParser p;
	p.SetBuffer(err);
	p.EnableCaseSensitivity();
	Matches m;
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
	token->buffer.Copy(msg);
	token->buffer.Append(": ");
	token->buffer.Append(err);
	token->str = token->buffer;
	return token;
}

const new_Token *new_SyntaxTreeGenerator::Generate(const mtlChars &entry_file) const
{
	return LoadFile(entry_file, NULL);
}
