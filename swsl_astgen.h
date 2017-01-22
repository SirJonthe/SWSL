#ifndef SWSL_ASTGEN_H_INCLUDED__
#define SWSL_ASTGEN_H_INCLUDED__

#include "MiniLib/MTL/mtlString.h"
#include "MiniLib/MTL/mtlParser.h"

namespace swsl
{

struct Token
{
	enum TokenType
	{
		TOKEN_ERR        = 0,
		TOKEN_START      = 1,
		TOKEN_DECL_VAR   = TOKEN_START      << 1,
		TOKEN_DECL_FN    = TOKEN_DECL_VAR   << 1,
		TOKEN_DEF_FN     = TOKEN_DECL_FN    << 1,
		TOKEN_DEF_STRUCT = TOKEN_DEF_FN     << 1,
		TOKEN_FILE       = TOKEN_DEF_STRUCT << 1,
		TOKEN_BODY       = TOKEN_FILE       << 1,
		TOKEN_SET        = TOKEN_BODY       << 1,
		TOKEN_CALL_FN    = TOKEN_SET        << 1,
		TOKEN_VAL        = TOKEN_CALL_FN    << 1,
		TOKEN_EXPR       = TOKEN_VAL        << 1,
		TOKEN_IF         = TOKEN_EXPR       << 1,
		TOKEN_WHILE      = TOKEN_IF         << 1,
		TOKEN_RET        = TOKEN_WHILE      << 1
	};

	const Token     * const parent;
	const TokenType         type;

	Token(Token *p_parent, TokenType p_type) :
		parent(p_parent), type(p_type)
	{}
	virtual ~Token( void ) {}
};

struct SyntaxTree : public Token
{
	Token *file; // Token_File

	SyntaxTree( void );
	~SyntaxTree( void );
};

struct Token_Err : public Token
{
	mtlChars err;
	mtlChars msg;

	Token_Err(Token *p_parent);
};

struct Token_DeclVar : public Token
{
	mtlChars  rw;
	mtlChars  type_name;
	mtlChars  ref;
	mtlChars  var_name;
	Token    *arr_size; // Token_Expr

	Token_DeclVar(Token *p_parent);
};

struct Token_DeclFn : public Token
{
	Token           *ret;    // Token_DeclVar
	mtlList<Token*>  params; // Token_DeclVar

	Token_DeclFn(Token *p_parent);
	~Token_DeclFn( void );
};

struct Token_DefFn : public Token
{
	Token *sig; // Token_DeclFn
	Token *body; // Token_Body

	Token_DefFn(Token *p_parent);
	~Token_DefFn( void );
};

struct Token_DefStruct : public Token
{
	mtlChars        struct_name;
	mtlList<Token*> decls;

	Token_DefStruct(Token *p_parent);
	~Token_DefStruct( void );
};

struct Token_File : public Token
{
	mtlChars   file_name;
	mtlString  content;
	Token     *body; // Token_Body

	Token_File(Token *p_parent);
	~Token_File( void );
};

struct Token_Body : public Token
{
	mtlList<Token*> tokens;

	Token_Body(Token *p_parent);
	~Token_Body( void );
};

struct Token_Set : public Token
{
	Token *lhs; // Token_Val
	Token *rhs; // Token_Expr

	Token_Set(Token *p_parent);
	~Token_Set( void );
};

struct Token_CallFn : public Token
{
	mtlChars        fn_name;
	mtlList<Token*> input; // Token_Expr

	Token_CallFn(Token *p_parent);
	~Token_CallFn( void );
};

struct Token_Val : public Token
{
	mtlChars val;

	Token_Val(Token *p_parent);
};

struct Token_Expr : public Token
{
	Token    *lhs; // Token_Val, Token_Expr
	Token    *rhs; // Token_Val, Token_Expr
	mtlChars  op;

	Token_Expr(Token *p_parent);
	~Token_Expr( void );
};

struct Token_If : public Token
{
	Token *cond;    // Token_Expr
	Token *if_body; // Token_Body
	Token *el_body; // Token_Body, Token_If

	Token_If(Token *p_parent);
	~Token_If( void );
};

struct Token_While : public Token
{
	Token *cond; // Token_Expr
	Token *body; // Token_Body

	Token_While(Token *p_parent);
	~Token_While( void );
};

struct Token_Ret : public Token
{
	Token *expr; // Token_Expr

	Token_Ret(Token *p_parent);
	~Token_Ret( void );
};

//class SourceTree
//{
//private:
//	class FileRef
//	{
//		mtlChars         file_name;
//		mtlList<FileRef> incl;
//	};
//
//private:
//	FileRef                 m_entry;
//	mtlStringMap<mtlString> m_contents;
//
//public:
//	void RecursiveLoad(const mtlChars &entry_file);
//};

class SyntaxTreeGenerator
{
private:
	Token *ProcessError(const mtlChars &msg, const mtlChars &err, Token *parent);
	Token *ProcessDecl(const mtlChars &rw, const mtlChars &type_name, const mtlChars &ref, const mtlChars &fn_name, Token *parent);
	Token *ProcessFuncCall(const mtlChars &fn_name, const mtlChars &params, Token *parent);
	Token *ProcessValue(const mtlChars &val, Token *parent);
	Token *ProcessSet(const mtlChars &lhs, const mtlChars &rhs, Token *parent);
	Token *ProcessFuncDecl(const mtlChars &rw, const mtlChars &type_name, const mtlChars &ref, const mtlChars &fn_name, const mtlChars &params, Token *parent);
	Token *ProcessOperation(const mtlChars &lhs, const mtlChars &op, const mtlChars &rhs, Token *parent);
	Token *ProcessExpression(const mtlChars &expr, Token *parent);
	Token *ProcessIf(const mtlChars &cond, const mtlChars &body, Token *parent, mtlSyntaxParser &p);
	Token *ProcessWhile(const mtlChars &cond, const mtlChars &body, Token *parent);
	Token *ProcessReturn(const mtlChars &expr, Token *parent);
	Token *ProcessBody(const mtlChars &body, Token *parent);
	Token *ProcessFuncDef(const mtlChars &rw, const mtlChars &type_name, const mtlChars &ref, const mtlChars &fn_name, const mtlChars &params, const mtlChars &body, Token *parent);
	Token *ProcessStructDef(const mtlChars &struct_name, const mtlChars &decls, Token *parent);
	Token *ProcessFile(const mtlChars &contents, Token *parent);
	Token *LoadFile(const mtlChars &file_name, Token *parent);

public:
	SyntaxTree *Generate(const mtlChars &entry_file);
};

}

#endif // SWSL_ASTGEN_H_INCLUDED__
