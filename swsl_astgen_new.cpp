#include "swsl_astgen_new.h"

#define _param_decl_str "%?(mutable %| readonly) %w%?(&)%w"
#define _param_decl_qlf 0
#define _param_decl_typ 1
#define _param_decl_ref 2
#define _param_decl_nam 3

#define _var_decl_str   "%?(const %| mutable %| readonly) %w%%w"
#define _var_decl_qlf 0
#define _var_decl_typ 1
#define _var_decl_nam 2

#define _fn_decl_str "%?(mutable %| readonly) %w%w(%s)"
#define _fn_decl_qlf 0
#define _fn_decl_typ 1
#define _fn_decl_nam 2
#define _fn_decl_par 3

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

typedef mtlArray<mtlChars> Matches;

class Parser
{
private:
	mtlSyntaxParser p;
	Matches         m;
	mtlChars        s;

public:
	Parser(const mtlChars &buffer);
	bool            IsEnd( void ) const;
	int             Match(const mtlChars &expr);
	const mtlChars &Match(int i) const;
};

Parser::Parser(const mtlChars &buffer)
{
	p.SetBuffer(buffer);
	p.EnableCaseSensitivity();
}

bool Parser::IsEnd( void ) const
{
	return p.IsEnd();
}

int Parser::Match(const mtlChars &expr)
{
	return p.Match(expr, m, &s);
}

const mtlChars &Parser::Match(int i) const
{
	if (i >= m.GetSize() || i < 0) { return p.GetBufferRemaining(); }
	return m[i];
}

new_Token::new_Token(const mtlChars &str_, Type type_, const new_Token *parent_) :
	str(str_), type(type_), parent(parent_), sub(NULL), next(NULL), ref(NULL)
{}
new_Token::~new_Token( void )
{
	delete sub;
	delete next;
	// don't delete ref
}

int new_Token::CountAscend(unsigned int type_mask) const
{
	return ((type_mask & (unsigned int)type) > 0 ? 1 : 0) + (parent != NULL ? parent->CountAscend(type_mask) : 0);
}

bool new_SyntaxTreeGenerator::IsReserved(const mtlChars &name) const
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

bool new_SyntaxTreeGenerator::IsBuiltInType(const mtlChars &name) const
{
	for (int i = 0; i < _built_in_n; ++i) {
		if (name.Compare(_built_in_types[i], true)) { return true; }
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
	while (parent != NULL && parent->type != new_Token::SCOPE) {
		parent = parent->parent;
	}

	if (parent != NULL && parent->type == new_Token::SCOPE) {
		const new_Token *statement = parent->sub;
		while (statement != NULL) {
			switch (statement->type) {
			case new_Token::FN_DECL:
			case new_Token::FN_DEF:
			case new_Token::VAR_DECL:
			case new_Token::TYPE_DEF:
				{
					new_Token *t = statement->sub;
					while (t != NULL && t->type != new_Token::USR_NAME) {
						t = t->next;
					}
					if (t != NULL) {
						return t->str.Compare(name, true);
					}
				}
				break;
			default: break;
			}
			statement = statement->next;
		}
	}
	return true;
}

bool new_SyntaxTreeGenerator::IsValidName(const mtlChars &name, const new_Token *parent) const
{
	return IsValidNameConvention(name) && !IsReserved(name) && IsNewName(name, parent);
}

bool new_SyntaxTreeGenerator::IsCTConst(const new_Token *expr, bool &result) const
{
	if (!result)      { return false; }
	if (expr == NULL) { return true; }
	switch (expr->type) {
	case new_Token::EXPR:
		{
			result = result && IsCTConst(expr->sub, result) && IsCTConst(expr->sub->next->next, result);
			break;
		}
	case new_Token::VAR_OP:
		{
			result = result && (expr->ref != NULL && expr->ref->type == new_Token::VAR_DECL && expr->ref->sub != NULL && expr->ref->sub->type == new_Token::TYPE_TRAIT && expr->ref->sub->str.Compare("const", true));
			break;
		}
		break;
	case new_Token::FN_OP:
	case new_Token::ERR:
		result = false;
		break;
	default: break;
	}
	return result;
}

new_Token *new_SyntaxTreeGenerator::ProcessDeclFn(const mtlChars &full, const mtlChars &rw, const mtlChars &type_name, const mtlChars &arr_size, const mtlChars &ref, const mtlChars &fn_name, const mtlChars &params, const new_Token *parent)
{
	new_Token *token = new new_Token(full, new_Token::FN_DECL, parent);

	new_Token **t = &token->sub;
	t = ProcessName(fn_name, token);
	t = &(*t)->next;
	t = ProcessDeclVar(rw, type_name, arr_size, ref, token);
	t = &(*t)->next;
	t = ProcessParamDecl(params, token);

	return token;
}

new_Token *new_SyntaxTreeGenerator::ProcessFile(const mtlChars &file_contents, const new_Token *parent)
{
	new_Token *token = new new_Token(file_contents, new_Token::SCOPE, parent);

	Parser p(file_contents);
	new_Token **t = &token->sub;
	while (!p.IsEnd()) {
		while (p.Match(";")) {}
		switch (p.Match(_fn_decl_str "; %| " _fn_decl_str "{%s} %| struct %w{%s} %| import\"%S\" %| %s")) {
		case 0:
			token->sub = ProcessDeclFn(p.Match(_fn_decl_qlf), p.Match(_fn_decl_typ), p.Match(_fn_decl_nam), p.Match(_fn_decl_par), token);
			break;

		case 1:
			token->sub = ProcessDeclFn(p.Match(_fn_decl_qlf), p.Match(_fn_decl_typ), p.Match(_fn_decl_nam), p.Match(_fn_decl_par), p.Match(_fn_decl_par + 1), token);
			break;

		case 2:
			token->sub = ProcessTypeDef(p.Match(0), p.Match(1), token);
			break;

		case 3:
			token->sub = LoadFile(p.Match(0), token);
			break;

		default:
			token->sub = ProcessError("[ProcessFile] Syntax error", p.Match(0), token);
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

new_Token *new_SyntaxTreeGenerator::LoadFile(const mtlChars &file_name, const new_Token *parent)
{
	new_Token *token = new new_Token(file_name, new_Token::FILE, parent);

	if (!mtlSyntaxParser::BufferFile(file_name, token->contents)) {
		delete token;
		return ProcessError("[LoadFile] File not found", file, parent);
	}
	RemoveComments(token->contents);
	return ProcessFile(token->contents, token);
}

new_Token *new_SyntaxTreeGenerator::ProcessError(const mtlChars &msg, mtlChars err, const new_Token *parent)
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
	token->contents.Copy(err);
	token->contents.Append(" ");
	token->contents.Append(msg);
	token->str = token->contents;
	return token;
}

const new_Token *new_SyntaxTreeGenerator::Generate(const mtlChars &entry_file)
{
	return LoadFile(entry_file, NULL);
}
