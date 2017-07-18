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

bool new_SyntaxTreeGenerator::IsReserved(const mtlChars &name)
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

bool new_SyntaxTreeGenerator::IsBuiltInType(const mtlChars &name)
{
	for (int i = 0; i < _built_in_n; ++i) {
		if (name.Compare(_built_in_types[i], true)) { return true; }
	}
	return false;
}

bool new_SyntaxTreeGenerator::VerifyName(const mtlChars &name)
{
	if (name.GetSize() > 0 && !IsReserved(name)) {
		for (int i = 1; i < name.GetSize(); ++i) {
			if (!mtlChars::IsAlphanumeric(name[i]) && name[i] != '_') { return false; }
		}
		return mtlChars::IsAlpha(name[0]) || name[0] == '_';
	}
	return false;
}

bool new_SyntaxTreeGenerator::CmpVarDeclName(const mtlChars &name, const new_Token *tok)
{
	return tok != NULL && name.Compare(tok->str, true);
}

bool new_SyntaxTreeGenerator::CmpFnDeclName(const mtlChars &name, const new_Token *tok)
{
	return tok != NULL && name.Compare(tok->str, true);
}

bool new_SyntaxTreeGenerator::CmpFnDefName(const mtlChars &name, const new_Token *tok)
{
	return tok != NULL && name.Compare(tok->str, true);
}

bool new_SyntaxTreeGenerator::CmpVarDefName(const mtlChars &name, const new_Token *tok)
{
	return tok != NULL && name.Compare(tok->str, true);
}

bool new_SyntaxTreeGenerator::NewName(const mtlChars &name, const new_Token *parent)
{
	while (parent != NULL && parent->type != new_Token::BODY) {
		parent = parent->parent;
	}

	if (parent != NULL && parent->type == new_Token::BODY) {
		const mtlItem<new_Token*> *i = dynamic_cast<const new_Token_Body*>(parent)->new_Tokens.GetFirst();
		while (i != NULL) {
			const new_Token *t = i->GetItem();
			switch (t->type) {
			case new_Token::FN_DECL:
				if (CmpFnDeclName(name, dynamic_cast<const new_Token_DeclFn*>(t))) { return false; }
				break;
			case new_Token::VAR_DECL:
				if (CmpVarDeclName(name, dynamic_cast<const new_Token_DeclVar*>(t))) { return false; }
				break;
			case new_Token::FN_DEF:
				if (CmpFnDefName(name, dynamic_cast<const new_Token_DefFn*>(t))) { return false; }
				break;
			case new_Token::TYPE_DEF:
				if (CmpVarDefName(name, dynamic_cast<const new_Token_DefType*>(t))) { return false; }
				break;
			default: break;
			}
			i = i->GetNext();
		}
	}
	return true;
}

bool new_SyntaxTreeGenerator::IsCTConst(const new_Token *expr, bool &result) const
{
	if (!result)      { return false; }
	if (expr == NULL) { return true; }
	switch (expr->type) {
	case new_Token::EXPR:
		{
			const new_Token_Expr *e = dynamic_cast<const new_Token_Expr*>(expr);
			result = result && IsCTConst(e->lhs, result) && IsCTConst(e->rhs, result);
			break;
		}
	case new_Token::VAR_OP:
		{
			const new_Token_ReadVar *v = dynamic_cast<const new_Token_ReadVar*>(expr);
			result = result && (v != NULL && v->decl_type != NULL && v->decl_type->permissions == new_Token_DeclVarType::Constant);
			break;
		}
		result = result && expr->ref->str.Compare("const", true);
		break;
	case new_Token::FN_OP:
	case new_Token::ERR:
		result = false;
		break;
	default: break;
	}
	return result;
}

new_Token *new_SyntaxTreeGenerator::ProcessFile(const mtlChars &file_contents, const new_Token *parent)
{
	new_Token *token = new new_Token(file_contents, new_Token::BODY, parent);

	Parser p(file_contents);
	new_Token **t = &token->sub;
	while (!p.IsEnd()) {
		while (p.Match(";")) {}
		switch (p.Match(_fn_decl_str "; %| " _fn_decl_str "{%s} %| struct %w{%s} %| import\"%S\" %| %s")) {
		case 0:
			break;

		case 1:
			break;

		case 2:
			break;

		case 3:
			break;

		default:
			break;
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
