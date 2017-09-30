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

bool new_SyntaxTreeGenerator::Parser::Consume(const mtlChars &str)
{
	return p.Match(str) == 0;
}

void new_SyntaxTreeGenerator::Parser::ConsumeAll(const mtlChars &str)
{
	while (p.Match(str) == 0) {}
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
	return false;
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

bool new_SyntaxTreeGenerator::IsCTConst(const new_Token *expr, unsigned int op_type, bool &result) const
{
	if (!result)                               { return false; }
	if (expr == NULL)                          { return true; }
	if (expr->str.GetTrimmed().GetSize() == 0) { return true; }
	switch (expr->type) {
	case new_Token::BOOL_EXPR:
	case new_Token::INT_EXPR:
	case new_Token::FLOAT_EXPR:
	case new_Token::LIST_EXPR:
		{
			result = result && IsCTConst(expr->sub, op_type, result) && IsCTConst(expr->next, op_type, result);
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
		result = (op_type & expr->type) > 0;
		break;
	case new_Token::MATH_OP:
		result = result && IsCTConst(expr->next, op_type, result);
		break;
	case new_Token::FN_OP:
	case new_Token::ERR:
		result = false;
		break;
	default: break;
	}
	return result;
}

bool new_SyntaxTreeGenerator::IsCTConst(const new_Token *expr, unsigned int op_type) const
{
	bool result = true;
	return IsCTConst(expr, op_type, result);
}

const new_Token *new_SyntaxTreeGenerator::GetTypeFromDecl(const new_Token *decl) const
{
	if (decl != NULL && (decl->type == new_Token::VAR_DECL || decl->type == new_Token::FN_DECL || decl->type == new_Token::FN_DEF)) {
		decl = decl->sub;
		while (decl != NULL && decl->type != new_Token::TYPE_NAME) {
			decl = decl->next;
		}
		if (decl != NULL && decl->type == new_Token::TYPE_NAME) {
			if (!IsBuiltInType(decl->str)) {
				decl = decl->ref;
			}
			return decl;
		}
	}
	return NULL;
}

const new_Token *new_SyntaxTreeGenerator::GetTraitFromDecl(const new_Token *decl) const
{
	if (decl != NULL && (decl->type == new_Token::VAR_DECL || decl->type == new_Token::FN_DECL || decl->type == new_Token::FN_DEF)) {
		decl = decl->sub;
		while (decl != NULL && decl->type != new_Token::TYPE_TRAIT) {
			decl = decl->next;
		}
		if (decl != NULL && decl->type == new_Token::TYPE_TRAIT) {
			return decl;
		}
	}
	return NULL;
}

const new_Token *new_SyntaxTreeGenerator::GetTypeFromName(const mtlChars &name, const new_Token *scope) const
{
	return GetTypeFromDecl(FindName(name, scope));
}

const new_Token *new_SyntaxTreeGenerator::GetTraitFromName(const mtlChars &name, const new_Token *scope) const
{
	return GetTraitFromDecl(FindName(name, scope));
}

new_Token *new_SyntaxTreeGenerator::ProcessDefType(const mtlChars &type_name, const mtlChars &decls, const mtlChars &seq, const new_Token *parent) const
{
	new_Token *token = new new_Token(seq, new_Token::TYPE_DEF, parent);

	new_Token **t = &token->sub;
	*t = ProcessNewName(type_name, token);
	t = &(*t)->next;
	*t = new new_Token(decls, new_Token::SCOPE, token);
	t = &(*t)->sub;
	Parser p(decls);
	while (!p.IsEnd()) {
		p.ConsumeAll(";");
		if (p.Match("%?(var %| imm)%w[%S]%w;") == 0) {
			*t = ProcessDeclVar(p.GetMatch(0), p.GetMatch(1), p.GetMatch(2), p.GetMatch(3), "", p.Seq(), token);
		} else if (p.Match("%?(var %| imm)%w%w;") == 0) {
			*t = ProcessDeclVar(p.GetMatch(0), "", p.GetMatch(1), p.GetMatch(2), "", p.Seq(), token);
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
	new_Token *token = new new_Token(p.Seq(), new_Token::IF, parent);

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

new_Token *new_SyntaxTreeGenerator::ProcessWhile(const mtlChars &cond, const mtlChars &body, const mtlChars &seq, const new_Token *parent) const
{
	new_Token *token = new new_Token(seq, new_Token::WHILE, parent);

	new_Token **t = &token->sub;
	*t = ProcessExpr(cond, token);
	t = &(*t)->next;
	*t = ProcessScope(body, token);
	return token;
}

new_Token *new_SyntaxTreeGenerator::ProcessRet(const mtlChars &expr, const new_Token *parent) const
{
	new_Token *token = new new_Token(expr, new_Token::WHILE, parent);

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

new_Token *new_SyntaxTreeGenerator::ProcessDeclVar(const mtlChars &trait, const mtlChars &type_name, const mtlChars &arr_size, const mtlChars &var_name, const mtlChars &expr, const mtlChars &seq, const new_Token *parent) const
{
	new_Token *token = new new_Token(seq, new_Token::VAR_DECL, parent);

	new_Token **t = &token->sub;
	new_Token *trait_token = *t = ProcessTrait(trait, token);
	t = &(*t)->next;
	*t = ProcessType(type_name, token);
	t = &(*t)->next;
	*t = ProcessArraySize(arr_size, token);
	t = &(*t)->next;
	*t = ProcessNewName(var_name, token);
	if (!expr.Compare("void", true)) {
		t = &(*t)->next;
		*t = ProcessExpr(expr, token);
	} else if (trait_token->str.Compare("imm", true) || trait_token->str.Compare("lit", true)) {
		delete token;
		token = ProcessError("[ProcessDeclVar] Immutable and literals must be initialized at declaration", seq, parent);
	}
	return token;
}

new_Token *new_SyntaxTreeGenerator::ProcessDeclParamVar(const mtlChars &trait, const mtlChars &type_name, const mtlChars &arr_size, const mtlChars &var_name, const mtlChars &seq, const new_Token *parent) const
{
	new_Token *token = new new_Token(seq, new_Token::VAR_DECL, parent);

	new_Token **t = &token->sub;
	*t = ProcessTrait(trait, token);
	t = &(*t)->next;
	*t = ProcessType(type_name, token);
	t = &(*t)->next;
	*t = ProcessArraySize(arr_size, token);
	t = &(*t)->next;
	*t = ProcessNewName(var_name, token);
	return token;
}

new_Token *new_SyntaxTreeGenerator::ProcessScope(const mtlChars &scope, const new_Token *parent) const
{
	new_Token *token = new new_Token(scope, new_Token::SCOPE, parent);

	Parser p(scope);
	new_Token **t = &token->sub;
	while (!p.IsEnd()) {
		p.ConsumeAll(";");

		switch (p.Match("{%s} %| if(%S){%s} %| while(%S){%s} %| return %s; %| %?(var %| imm %| lit)%w[%S]%w=%S; %| %?(var %| imm %| lit)%w%w=%S; %| %S=%S; %| %S; %| %s")) {
		case 0:
			*t = ProcessScope(p.GetMatch(0), token);
			break;

		case 1:
			*t = ProcessIf(p.GetMatch(0), p.GetMatch(1), token, p);
			break;

		case 2:
			*t = ProcessWhile(p.GetMatch(0), p.GetMatch(1), p.Seq(), token);
			break;

		case 3:
			*t = ProcessRet(p.GetMatch(0), token);
			break;

		case 4:
			*t = ProcessDeclVar(p.GetMatch(0), p.GetMatch(1), p.GetMatch(2), p.GetMatch(3), p.GetMatch(4), p.Seq(), token);
			break;

		case 5:
			*t = ProcessDeclVar(p.GetMatch(0), p.GetMatch(1), "", p.GetMatch(2), p.GetMatch(3), p.Seq(), token);
			break;

		case 6:
			*t = ProcessSet(p.GetMatch(0), p.GetMatch(1), p.Seq(), token);
			break;

		case 7:
			*t = ProcessExpr(p.GetMatch(0), token);
			break;

		default:
			*t = ProcessError("[ProcessScope] Syntax error", p.Rem(), token);
		}
		t = &(*t)->next;
	}
	return token;
}

// if (BOOL_EXPR)
// search first for logical operators - BOOL_EXPR(BOOL_EXPR && BOOL_EXPR || BOOL_EXPR)
// search for comparators - BOOL_EXPR(INT_EXPR < INT_EXPR)
// search for mathematical operations - INT_EXPR(INT_EXPR + INT_EXPR)

bool new_SyntaxTreeGenerator::CreateTypeList(const mtlChars &type_name, mtlString &out, const new_Token *parent) const
{
	out.Free();
	if (IsBuiltInType(type_name)) {
		out.Copy(type_name);
	} else {
		const new_Token *token = FindName(type_name, parent);
		if (token == NULL || token->type != new_Token::TYPE_DEF) {
			return false;
		}
		token = token->sub;
		while (token != NULL) {
			if (token->type == new_Token::SCOPE) {
				token = token->sub;
				break;
			}
			token = token->next;
		}
		while (token != NULL) {
			if (token->type == new_Token::VAR_DECL) {
				const new_Token *type = GetTypeFromDecl(token);
				const new_Token *size = GetSizeFromDecl(token);
				if (type != NULL && size != NULL) {
					out.Append(type->str);
					out.Append("[");
					out.Append(size->str);
					out.Append("]");
				} else {
					out.Free();
					return false;
				}
			} else {
				out.Free();
				return false;
			}
			if (token->next != NULL) {
				out.Append(", ");
			}
			token = token->next;
		}
	}

	return true;
}

// all list items have size, 0 if non-array
// all expression processing now has to take array string
	// if non-0 then () is expected around the expression
new_Token *new_SyntaxTreeGenerator::ProcessListExpr(const mtlChars &expr, const mtlChars &arr_size, const mtlChars &list, const new_Token *parent) const
{
	// list has format
		// int[0], float[0], bool[2], Color3[0], int[4, 5, 1]
		// 0 means "no-array"

	new_Token *token = new new_Token(expr, new_Token::LIST_EXPR, parent);
	new_Token **t = &(token->sub);
	Parser ep(expr);
	Parser ap(arr_size);
	if (ap.Match("0%0") != 0) {
		if (ep.Match("(%S)%0") == 0) {
			int arr_size = 0;
			if (ap.Match("%i") == 0 && ap.GetMatch(0).ToInt(arr_size)) {
				mtlChars rem = (ap.Match(",") == 0) ? ap.Rem() : "0";
				for (int i = 0; i < arr_size; ++i) {
					*t = ProcessListExpr(ep.GetMatch(0), rem, list, token);
					t = &(*t)->next;
				}
			} else {
				delete token;
				token = ProcessError("[ProcessListExpr] Bad array size", expr, parent);
			}
		} else {
			delete token;
			token = ProcessError("[ProcessListExpr] Array not surrounded by braces", expr, parent);
		}
	} else {
		Parser lp(list);
		while (!ep.IsEnd() && !lp.IsEnd() && token->type != new_Token::ERR) {
			switch (ep.Match("%S, %| %S")) {
			case 0:
			case 1:
				switch (lp.Match("bool[%S] %| int[%S] %| float[%S] %| %w[%S]")) { // TODO: Parse arrays correctly
				case 0:
					*t = ProcessBoolExpr(ep.GetMatch(0), lp.GetMatch(0), token);
					break;

				case 1:
					*t = ProcessIntExpr(ep.GetMatch(0), lp.GetMatch(0), token);
					break;

				case 2:
					*t = ProcessFloatExpr(ep.GetMatch(0), lp.GetMatch(0), token);
					break;

				case 3:
				{
					mtlString type_list;
					if (CreateTypeList(lp.GetMatch(0), type_list, token)) {
						*t = ProcessListExpr(ep.GetMatch(0), lp.GetMatch(1), type_list, token);
					} else {
						delete token;
						token = ProcessError("[ProcessListExpr] Unknown list parameter", lp.GetMatch(0), parent);
					}
					break;
				}

				default:
					delete token;
					token = ProcessError("[ProcessListExpr] Unknown list parameter", lp.Rem(), parent);
					break;
				}
				lp.Match(",");
				break;

			default:
				delete token;
				token = ProcessError("[ProcessListExpr] Syntax error", expr, parent);
				break;
			}
			t = &(*t)->next;
		}
		if (lp.IsEnd() != ep.IsEnd()) {
			delete token;
			token = ProcessError("[ProcessListExpr] List parameter mismatch", expr, parent);
		}
	}
	return token;
}

new_Token *new_SyntaxTreeGenerator::ProcessIntExpr(const mtlChars &expr, const mtlChars &arr_size, const new_Token *parent) const
{
	new_Token *token = new new_Token(expr, new_Token::INT_EXPR, parent);

	Parser p(expr);
	Parser ap(arr_size);
	if (ap.Match("0%0") != 0) {
		if (p.Match("(%S)%0") == 0) {
			int arr_size = 0;
			if (ap.Match("%i") == 0 && ap.GetMatch(0).ToInt(arr_size)) {
				new_Token **t = &token->sub;
				mtlChars rem = (ap.Match(",") == 0) ? ap.Rem() : "0";
				for (int i = 0; i < arr_size; ++i) {
					*t = ProcessIntExpr(p.GetMatch(0), rem, token);
					t = &(*t)->next;
				}
			} else {
				delete token;
				token = ProcessError("[ProcessIntExpr] Bad array size", expr, parent);
			}
		} else {
			delete token;
			token = ProcessError("[ProcessIntExpr] Array not surrounded by braces", expr, parent);
		}
	} else {
		switch (p.Match("%s+%S %| %s-%S %| %S*%S %| %S/%S %| %S%%%S %| %S|%S %| %S&%S %| %S^%S %| ~%S %| %S<<%S %| %S>>%S %| (%S)%0 %| %s")) {
		case 0:
			token->sub = ProcessMathOp(p.GetMatch(0), "ADD", p.GetMatch(1), new_Token::INT_OP, token);
			break;

		case 1:
			token->sub = ProcessMathOp(p.GetMatch(0), "SUB", p.GetMatch(1), new_Token::INT_OP, token);
			break;

		case 2:
			token->sub = ProcessMathOp(p.GetMatch(0), "MUL", p.GetMatch(1), new_Token::INT_OP, token);
			break;

		case 3:
			token->sub = ProcessMathOp(p.GetMatch(0), "DIV", p.GetMatch(1), new_Token::INT_OP, token);
			break;

		case 4:
			token->sub = ProcessMathOp(p.GetMatch(0), "MOD", p.GetMatch(1), new_Token::INT_OP, token);
			break;

		case 5:
			token->sub = ProcessMathOp(p.GetMatch(0), "OR", p.GetMatch(1), new_Token::INT_OP, token);
			break;

		case 6:
			token->sub = ProcessMathOp(p.GetMatch(0), "AND", p.GetMatch(1), new_Token::INT_OP, token);
			break;

		case 7:
			token->sub = ProcessMathOp(p.GetMatch(0), "XOR", p.GetMatch(1), new_Token::INT_OP, token);
			break;

		case 8:
			token->sub = ProcessMathOp("", "NOT", p.GetMatch(0), new_Token::INT_OP, token);
			break;

		case 9:
			token->sub = ProcessMathOp(p.GetMatch(0), "LSHIFT", p.GetMatch(1), new_Token::INT_OP, token);
			break;

		case 10:
			token->sub = ProcessMathOp(p.GetMatch(0), "RSHIFT", p.GetMatch(1), new_Token::INT_OP, token);
			break;

		case 11:
			token->sub = ProcessIntExpr(p.GetMatch(0), "0", token);
			break;

		case 12:
			delete token;
			token = ProcessMemOp(p.GetMatch(0), parent);
			break;

		default:
			delete token;
			token = ProcessError("[ProcessIntExpr] Syntax error", p.Rem(), parent);
			break;
		}
	}
	return token;
}

new_Token *new_SyntaxTreeGenerator::ProcessFloatExpr(const mtlChars &expr, const mtlChars &arr_size, const new_Token *parent) const
{
	new_Token *token = new new_Token(expr, new_Token::FLOAT_EXPR, parent);

	Parser p(expr);
	Parser ap(arr_size);
	if (ap.Match("0%0") != 0) {
		if (p.Match("(%S)%0") == 0) {
			int arr_size = 0;
			if (ap.Match("%i") == 0 && ap.GetMatch(0).ToInt(arr_size)) {
				new_Token **t = &token->sub;
				mtlChars rem = (ap.Match(",") == 0) ? ap.Rem() : "0";
				for (int i = 0; i < arr_size; ++i) {
					*t = ProcessFloatExpr(p.GetMatch(0), rem, token);
					t = &(*t)->next;
				}
			} else {
				delete token;
				token = ProcessError("[ProcessFloatExpr] Bad array size", expr, parent);
			}
		} else {
			delete token;
			token = ProcessError("[ProcessFloatExpr] Array not surrounded by braces", expr, parent);
		}
	} else {
		switch (p.Match("%s+%S %| %s-%S %| %S*%S %| %S/%S %| (%S)%0 %| %s")) {
		case 0:
			token->sub = ProcessMathOp(p.GetMatch(0), "ADD", p.GetMatch(1), new_Token::FLOAT_OP, token);
			break;

		case 1:
			token->sub = ProcessMathOp(p.GetMatch(0), "SUB", p.GetMatch(1), new_Token::FLOAT_OP, token);
			break;

		case 2:
			token->sub = ProcessMathOp(p.GetMatch(0), "MUL", p.GetMatch(1), new_Token::FLOAT_OP, token);
			break;

		case 3:
			token->sub = ProcessMathOp(p.GetMatch(0), "DIV", p.GetMatch(1), new_Token::FLOAT_OP, token);
			break;

		case 4:
			token->sub = ProcessFloatExpr(p.GetMatch(0), "0", token);
			break;

		case 5:
			delete token;
			token = ProcessMemOp(p.GetMatch(0), parent);
			break;

		default:
			delete token;
			token = ProcessError("[ProcessFloatExpr] Syntax error", p.Rem(), parent);
			break;
		}
	}
	return token;
}

new_Token *new_SyntaxTreeGenerator::ProcessBoolExpr(const mtlChars &expr, const mtlChars &arr_size, const new_Token *parent) const
{
	new_Token *token = new new_Token(expr, new_Token::BOOL_EXPR, parent);

	Parser p(expr);
	Parser ap(arr_size);
	if (ap.Match("0%0") != 0) {
		if (p.Match("(%S)%0") == 0) {
			int arr_size = 0;
			if (ap.Match("%i") == 0 && ap.GetMatch(0).ToInt(arr_size)) {
				new_Token **t = &token->sub;
				mtlChars rem = (ap.Match(",") == 0) ? ap.Rem() : "0";
				for (int i = 0; i < arr_size; ++i) {
					*t = ProcessBoolExpr(p.GetMatch(0), rem, token);
					t = &(*t)->next;
				}
			} else {
				delete token;
				token = ProcessError("[ProcessBoolExpr] Bad array size", expr, parent);
			}
		} else {
			delete token;
			token = ProcessError("[ProcessBoolExpr] Array not surrounded by braces", expr, parent);
		}
	} else {
		switch (p.Match("%S==%S %| %S!=%S %| %S<=%S %| %S<%S %| %S>=%S %| %S>%S %| %S||%S %| %S&&%S %| %S|%S %| %S&%S %| %S^%S %| ~%S %| (%S)%0 %| %s")) { // TODO: add && ||
		case 0:
			token->sub = ProcessMathOp(p.GetMatch(0), "EQUAL", p.GetMatch(1), new_Token::BOOL_OP|new_Token::INT_OP|new_Token::FLOAT_OP, token);
			break;

		case 1:
			token->sub = ProcessMathOp(p.GetMatch(0), "NOT_EQUAL", p.GetMatch(1), new_Token::BOOL_OP|new_Token::INT_OP|new_Token::FLOAT_OP, token);
			break;

		case 2:
			token->sub = ProcessMathOp(p.GetMatch(0), "LESS_EQUAL", p.GetMatch(1), new_Token::INT_OP|new_Token::FLOAT_OP, token);
			break;

		case 3:
			token->sub = ProcessMathOp(p.GetMatch(0), "LESS", p.GetMatch(1), new_Token::INT_OP|new_Token::FLOAT_OP, token);
			break;

		case 4:
			token->sub = ProcessMathOp(p.GetMatch(0), "GREATER_EQUAL", p.GetMatch(1), new_Token::INT_OP|new_Token::FLOAT_OP, token);
			break;

		case 5:
			token->sub = ProcessMathOp(p.GetMatch(0), "GREATER", p.GetMatch(1), new_Token::INT_OP|new_Token::FLOAT_OP, token);
			break;

		case 6:
			token->sub = ProcessMathOp(p.GetMatch(0), "LOGICAL_OR", p.GetMatch(1), new_Token::BOOL_OP, token);
			break;

		case 7:
			token->sub = ProcessMathOp(p.GetMatch(0), "LOGICAL_AND", p.GetMatch(1), new_Token::BOOL_OP, token);
			break;

		case 8:
			token->sub = ProcessMathOp(p.GetMatch(0), "OR", p.GetMatch(1), new_Token::BOOL_OP, token);
			break;

		case 9:
			token->sub = ProcessMathOp(p.GetMatch(0), "AND", p.GetMatch(1), new_Token::BOOL_OP, token);
			break;

		case 10:
			token->sub = ProcessMathOp(p.GetMatch(0), "XOR", p.GetMatch(1), new_Token::BOOL_OP, token);
			break;

		case 11:
			token->sub = ProcessMathOp("", "NOT", p.GetMatch(0), new_Token::BOOL_OP, token);
			break;

		case 12:
			token->sub = ProcessBoolExpr(p.GetMatch(0), "0", token);
			break;

		case 13:
			delete token;
			token = ProcessMemOp(p.GetMatch(0), parent);
			break;

		default:
			delete token;
			token = ProcessError("[ProcessBoolExpr] Syntax error", p.Rem(), parent);
			break;
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
		token = (!var.Seq().FindFirstChar(mtlWhitespacesStr) >= 0) ? new new_Token(var.Seq(), new_Token::FLOAT_OP, parent) : ProcessError("[ProcessMemOp] Whitespaces in floating poing value", var.Seq(), parent);
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

new_Token *new_SyntaxTreeGenerator::ProcessMathOp(const mtlChars &lhs, const mtlChars &op, const mtlChars &rhs, unsigned int op_type, const new_Token *parent) const
{
	new_Token *token = new new_Token(op, new_Token::MATH_OP, parent);

	token->next = ProcessExpr(lhs, parent);
	token->next->next = ProcessExpr(rhs, parent);
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

bool new_SyntaxTreeGenerator::EvalConstBoolExpr(const new_Token *token) const
{
	bool ret_val = false;
	if (token == NULL || token->str.Compare("")) { return ret_val; }
	switch (token->type) {
	case new_Token::BOOL_OP:
		token->str.ToBool(ret_val);
		break;
	case new_Token::BOOL_EXPR:
		// we might need to eval int and float here as well
		{
			mtlChars op = token->sub->str;
			if (op.Compare("EQUAL")) {
				switch (token->sub->next->type) {
				case new_Token::BOOL_EXPR:
				case new_Token::BOOL_OP:
					ret_val = EvalConstBoolExpr(token->sub->next) == EvalConstBoolExpr(token->sub->next->next);
					break;
				case new_Token::INT_EXPR:
				case new_Token::INT_OP:
					ret_val = EvalConstIntExpr(token->sub->next) == EvalConstIntExpr(token->sub->next->next);
					break;
				case new_Token::FLOAT_EXPR:
				case new_Token::FLOAT_OP:
					ret_val = EvalConstFloatExpr(token->sub->next) == EvalConstFloatExpr(token->sub->next->next);
					break;
				default: break;
				}
			} else if (op.Compare("NOT_EQUAL")) {
				switch (token->sub->next->type) {
				case new_Token::BOOL_EXPR:
				case new_Token::BOOL_OP:
					ret_val = EvalConstBoolExpr(token->sub->next) != EvalConstBoolExpr(token->sub->next->next);
					break;
				case new_Token::INT_EXPR:
				case new_Token::INT_OP:
					ret_val = EvalConstIntExpr(token->sub->next) != EvalConstIntExpr(token->sub->next->next);
					break;
				case new_Token::FLOAT_EXPR:
				case new_Token::FLOAT_OP:
					ret_val = EvalConstFloatExpr(token->sub->next) != EvalConstFloatExpr(token->sub->next->next);
					break;
				default: break;
				}
			} else if (op.Compare("LESS_EQUAL")) {
				switch (token->sub->next->type) {
				case new_Token::BOOL_EXPR:
				case new_Token::BOOL_OP:
					ret_val = EvalConstBoolExpr(token->sub->next) <= EvalConstBoolExpr(token->sub->next->next);
					break;
				case new_Token::INT_EXPR:
				case new_Token::INT_OP:
					ret_val = EvalConstIntExpr(token->sub->next) <= EvalConstIntExpr(token->sub->next->next);
					break;
				case new_Token::FLOAT_EXPR:
				case new_Token::FLOAT_OP:
					ret_val = EvalConstFloatExpr(token->sub->next) <= EvalConstFloatExpr(token->sub->next->next);
					break;
				default: break;
				}
			} else if (op.Compare("LESS")) {
				switch (token->sub->next->type) {
				case new_Token::BOOL_EXPR:
				case new_Token::BOOL_OP:
					ret_val = EvalConstBoolExpr(token->sub->next) < EvalConstBoolExpr(token->sub->next->next);
					break;
				case new_Token::INT_EXPR:
				case new_Token::INT_OP:
					ret_val = EvalConstIntExpr(token->sub->next) < EvalConstIntExpr(token->sub->next->next);
					break;
				case new_Token::FLOAT_EXPR:
				case new_Token::FLOAT_OP:
					ret_val = EvalConstFloatExpr(token->sub->next) < EvalConstFloatExpr(token->sub->next->next);
					break;
				default: break;
				}
			} else if (op.Compare("GREATER_EQUAL")) {
				switch (token->sub->next->type) {
				case new_Token::BOOL_EXPR:
				case new_Token::BOOL_OP:
					ret_val = EvalConstBoolExpr(token->sub->next) >= EvalConstBoolExpr(token->sub->next->next);
					break;
				case new_Token::INT_EXPR:
				case new_Token::INT_OP:
					ret_val = EvalConstIntExpr(token->sub->next) >= EvalConstIntExpr(token->sub->next->next);
					break;
				case new_Token::FLOAT_EXPR:
				case new_Token::FLOAT_OP:
					ret_val = EvalConstFloatExpr(token->sub->next) >= EvalConstFloatExpr(token->sub->next->next);
					break;
				default: break;
				}
			} else if (op.Compare("GREATER")) {
				switch (token->sub->next->type) {
				case new_Token::BOOL_EXPR:
				case new_Token::BOOL_OP:
					ret_val = EvalConstBoolExpr(token->sub->next) > EvalConstBoolExpr(token->sub->next->next);
					break;
				case new_Token::INT_EXPR:
				case new_Token::INT_OP:
					ret_val = EvalConstIntExpr(token->sub->next) > EvalConstIntExpr(token->sub->next->next);
					break;
				case new_Token::FLOAT_EXPR:
				case new_Token::FLOAT_OP:
					ret_val = EvalConstFloatExpr(token->sub->next) > EvalConstFloatExpr(token->sub->next->next);
					break;
				default: break;
				}
			} else if (op.Compare("LOGICAL_AND")) {
				ret_val = EvalConstBoolExpr(token->sub->next) && EvalConstBoolExpr(token->sub->next->next);
			} else if (op.Compare("LOGICAL_OR")) {
				ret_val = EvalConstBoolExpr(token->sub->next) || EvalConstBoolExpr(token->sub->next->next);
			} else if (op.Compare("OR")) {
				ret_val = EvalConstBoolExpr(token->sub->next) | EvalConstBoolExpr(token->sub->next->next);
			} else if (op.Compare("AND")) {
				ret_val = EvalConstBoolExpr(token->sub->next) & EvalConstBoolExpr(token->sub->next->next);
			} else if (op.Compare("XOR")) {
				ret_val = EvalConstBoolExpr(token->sub->next) ^ EvalConstBoolExpr(token->sub->next->next);
			} else if (op.Compare("NOT")) {
				ret_val = !EvalConstBoolExpr(token->sub->next->next);
			}
		}
		break;
	}
	return ret_val;
}

new_Token *new_SyntaxTreeGenerator::ProcessConstBoolExpr(const mtlChars &expr, const mtlChars &arr_size, const new_Token *parent) const
{
	new_Token *token = ProcessBoolExpr(expr, arr_size, parent);
	if (!IsCTConst(token, new_Token::BOOL_OP) || token->CountDescend(new_Token::ERR) > 0) {
		delete token;
		token = ProcessError("[ProcessConstBoolExpr] Expression not compile-time constant", expr, parent);
	} else {
		// evaluate vlaue
		bool b_val = EvalConstBoolExpr(token);
		delete token;
		mtlString str_val;
		str_val.FromBool(b_val);
		token = new new_Token(str_val, new_Token::BOOL_OP, parent);
	}
	return token;
}

new_Token *new_SyntaxTreeGenerator::ProcessConstIntExpr(const mtlChars &expr, const mtlChars &arr_size, const new_Token *parent) const
{
	new_Token *token = ProcessIntExpr(expr, arr_size, parent);
	if (!IsCTConst(token, new_Token::INT_OP) || token->CountDescend(new_Token::ERR) > 0) {
		delete token;
		token = ProcessError("[ProcessConstIntExpr] Expression not compile-time constant", expr, parent);
	} else {
		// evaluate vlaue
		int i_val = EvalConstIntExpr(token);
		delete token;
		mtlString str_val;
		str_val.FromInt(i_val);
		token = new new_Token(str_val, new_Token::INT_OP, parent);
	}
	return token;
}

new_Token *new_SyntaxTreeGenerator::ProcessConstFloatExpr(const mtlChars &expr, const mtlChars &arr_size, const new_Token *parent) const
{
	new_Token *token = ProcessFloatExpr(expr, arr_size, parent);
	if (!IsCTConst(token, new_Token::FLOAT_OP) || token->CountDescend(new_Token::ERR) > 0) {
		delete token;
		token = ProcessError("[ProcessConstFloatExpr] Expression not compile-time constant", expr, parent);
	} else {
		// evaluate vlaue
		float fl_val = EvalConstFloatExpr(token);
		delete token;
		mtlString str_val;
		str_val.FromFloat(fl_val);
		token = new new_Token(str_val, new_Token::FLOAT_OP, parent);
	}
	return token;
}

new_Token *new_SyntaxTreeGenerator::ProcessArraySize(const mtlChars &arr_size, const new_Token *parent) const
{
	// TODO: Make sure arr_size evals to > 0
	new_Token *token = NULL;
	if (arr_size.GetTrimmed().GetSize() > 0) {
		token = new new_Token(arr_size, new_Token::LIST_EXPR, parent);
		new_Token **t = &token->sub;
		Parser p(arr_size);
		while (!p.IsEnd()) {
			switch (p.Match("%S, %| %S")) {
			case 0:
			case 1:
				*t = ProcessConstIntExpr(p.GetMatch(0), "0", token);
				break;
			default:
				*t = ProcessError("[ProcessArraySize] Syntax error", p.Rem(), token);
				break;
			}
			t = &((*t)->next);
		}
	} else {
		token = new new_Token("0", new_Token::INT_OP, parent); // non-arrays get a size of 0
	}
	return token;
}

new_Token *new_SyntaxTreeGenerator::ProcessDeclParam(new_Token **&token, const mtlChars &params, const new_Token *parent) const
{
	Parser p(params);
	*token = new new_Token(params, new_Token::SCOPE, parent);
	new_Token *parent_scope = *token;
	token = &(*token)->sub;
	if (p.Match("void%0") < 0) {
		while (!p.IsEnd()) {
			const int match = p.Match("%?(var %| imm)%w[%S]%w %| %?(var %| imm)%w%w");
			if (match >= 0 && (p.Consume(",") || p.IsEnd())) {
				switch (match) {
				case 0:
					*token = ProcessDeclParamVar(p.GetMatch(0), p.GetMatch(1), p.GetMatch(2), p.GetMatch(3), p.Seq(), parent_scope);
					break;
				case 1:
					*token = ProcessDeclParamVar(p.GetMatch(0), p.GetMatch(1), "", p.GetMatch(2), p.Seq(), parent_scope);
					break;
				}
				token = &(*token)->next;
			} else {
				*token = ProcessError("[ProcessDeclParam] Syntax error", p.Rem(), parent_scope);
				token = &(*token)->next;
				break;
			}
		}
	} else {
		parent_scope->str = "";
	}
	return parent_scope;
}

new_Token *new_SyntaxTreeGenerator::ProcessDeclFn(const mtlChars &type_name, const mtlChars &arr_size, const mtlChars &fn_name, const mtlChars &params, const mtlChars &seq, const new_Token *parent) const
{
	new_Token *token = new new_Token(seq, new_Token::FN_DECL, parent);

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

new_Token *new_SyntaxTreeGenerator::ProcessDefFn(const mtlChars &type_name, const mtlChars &arr_size, const mtlChars &fn_name, const mtlChars &params, const mtlChars &body, const mtlChars &seq, const new_Token *parent) const
{
	new_Token *token = new new_Token(seq, new_Token::FN_DEF, parent);

	token->ref = FindName(fn_name, parent);
	// TODO; Assign token->ref->ref to token if not already set (and if token->ref->type == FN_DECL)
	new_Token **t = &token->sub;
	*t = ProcessType(type_name, token);
	t = &(*t)->next;
	*t = ProcessArraySize(arr_size, token);
	t = &(*t)->next;
	*t = ProcessNewName(fn_name, token); // This may cause a conflict if there is already an FN_DECL and should be allowed
	t = &(*t)->next;
	new_Token *parent_scope = ProcessDeclParam(t, params, token);
	// ProcessDeclParam already increments t, no need to do it again
	*t = ProcessScope(body, parent_scope);

	return token;
}

new_Token *new_SyntaxTreeGenerator::ProcessSet(const mtlChars &lhs, const mtlChars &rhs, const mtlChars &seq, const new_Token *parent) const
{
	// TODO; Declarations must be explicitly (not) initialized
		// float a = 1.0;
		// float b = void; // uninitialized

	new_Token *token = NULL;

	const new_Token *type = GetTypeFromName(lhs, parent);
	if (type == NULL) {
		return ProcessError("[ProcessSet] ", seq, parent);
	} else if (token->str.Compare("bool", true) && token->next->str.Compare("0")) {
		token = new new_Token(seq, new_Token::BOOL_SET, parent);
	} else if (token->str.Compare("int", true) && token->next->str.Compare("0")) {
		token = new new_Token(seq, new_Token::INT_SET, parent);
	} else if (token->str.Compare("float", true) && token->next->str.Compare("0")) {
		token = new new_Token(seq, new_Token::FLOAT_SET, parent);
	} else {
		token = new new_Token(seq, new_Token::LIST_SET, parent);
	}

	new_Token **t = &token->sub;
	*t = ProcessMemOp(lhs, token);
	t = &(*t)->sub;
	new_Token **rhs_tok = &(*t)->next;
	while (*t != NULL && (*t)->type != new_Token::ERR) {
		if ((*t)->ref == NULL || (*t)->ref->type != new_Token::VAR_DECL || (*t)->ref->sub == NULL || (*t)->ref->sub->type != new_Token::TYPE_TRAIT || !(*t)->ref->sub->str.Compare("var", true)) {
			delete *t;
			*t = ProcessError("[ProcessSet] Assigning an immutable", lhs, parent);
		} else {
			t = &(*t)->next;
		}
	}

	// Emit error if the last member variable of lhs is dected to be a struct in order to disable support for assigning structs
	// TODO; Implement struct assignment feature and remove this error block
	if (*t != NULL && (*t)->ref != NULL && (*t)->ref->type == new_Token::MEM_OP && (*t)->ref->sub != NULL && (*t)->ref->sub->next != NULL && (*t)->ref->sub->next->type == new_Token::TYPE_NAME && !IsBuiltInType((*t)->ref->sub->next->str)) {
		delete *t;
		*t = ProcessError("[ProcessSet] Assigning a struct (not yet implemented)", lhs, parent);
	}
	// End error block

	switch (token->type) {
	case new_Token::BOOL_SET:
		*rhs_tok = ProcessBoolExpr(rhs, token);
		break;
	case new_Token::INT_SET:
		*rhs_tok = ProcessIntExpr(rhs, token);
		break;
	case new_Token::FLOAT_SET:
		*rhs_tok = ProcessFloatExpr(rhs, token);
		break;
	case new_Token::LIST_SET:
		*rhs_tok = ProcessListExpr(rhs, type->str, token);
		break;
	default: break;
	}
	return token;
}

new_Token *new_SyntaxTreeGenerator::ProcessFile(const mtlChars &file_contents, const new_Token *parent) const
{
	new_Token *token = new new_Token(file_contents, new_Token::SCOPE, parent);

	Parser p(file_contents);
	new_Token **t = &token->sub;
	while (!p.IsEnd()) {
		p.ConsumeAll(";");
		switch (p.Match("%w[%S]%w(%s); %| %w%w(%s); %| %w[%S]%w(%s){%s} %| %w%w(%s){%s} %| struct %w{%s} %| import\"%S\" %| %s")) {
		case 0:
			*t = ProcessDeclFn(p.GetMatch(0), p.GetMatch(1), p.GetMatch(2), p.GetMatch(3), p.Seq(), token);
			break;

		case 1:
			*t = ProcessDeclFn(p.GetMatch(0), "", p.GetMatch(1), p.GetMatch(2), p.Seq(), token);
			break;

		case 2:
			*t = ProcessDefFn(p.GetMatch(0), p.GetMatch(1), p.GetMatch(2), p.GetMatch(3), p.GetMatch(4), p.Seq(), token);
			break;

		case 3:
			*t = ProcessDefFn(p.GetMatch(0), "", p.GetMatch(1), p.GetMatch(2), p.GetMatch(3), p.Seq(), token);
			break;

		case 4:
			*t = ProcessDefType(p.GetMatch(0), p.GetMatch(1), p.Seq(), token);
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
