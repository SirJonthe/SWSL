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
		TOKEN_FLAG       = TOKEN_START      << 1,
		TOKEN_WORD       = TOKEN_FLAG       << 1,
		TOKEN_DECL_VAR   = TOKEN_WORD       << 1,
		TOKEN_DECL_FN    = TOKEN_DECL_VAR   << 1,
		TOKEN_DEF_FN     = TOKEN_DECL_FN    << 1,
		TOKEN_DEF_STRUCT = TOKEN_DEF_FN     << 1,
		TOKEN_FILE       = TOKEN_DEF_STRUCT << 1,
		TOKEN_BODY       = TOKEN_FILE       << 1,
		TOKEN_SET        = TOKEN_BODY       << 1,
		TOKEN_CALL_FN    = TOKEN_SET        << 1,
		TOKEN_VAR        = TOKEN_CALL_FN    << 1,
		TOKEN_LIT        = TOKEN_VAR        << 1,
		TOKEN_EXPR       = TOKEN_LIT        << 1,
		TOKEN_IF         = TOKEN_EXPR       << 1,
		TOKEN_WHILE      = TOKEN_IF         << 1,
		TOKEN_RET        = TOKEN_WHILE      << 1
	};

	const Token     *const parent;
	const TokenType        type;

	Token(const Token *p_parent, TokenType p_type) :
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

	Token_Err(const Token *p_parent);
};

struct Token_Flag : public Token
{
	bool flag;

	Token_Flag(const Token *p_parent);
};

struct Token_Word : public Token
{
	mtlChars word;

	Token_Word(const Token *p_parent);
};

struct Token_DeclVar : public Token
{
	Token *is_const;  // Token_Flag
	Token *type_name; // Token_Word
	Token *is_ref;    // Token_Flag
	Token *var_name;  // Token_Word
	Token *arr_size;  // Token_Expr

	Token_DeclVar(const Token *p_parent);
	~Token_DeclVar( void );
};

struct Token_DeclFn : public Token
{
	Token           *ret;    // Token_DeclVar
	mtlList<Token*>  params; // Token_DeclVar

	Token_DeclFn(const Token *p_parent);
	~Token_DeclFn( void );
};

struct Token_DefFn : public Token
{
	Token *sig;  // Token_DeclFn
	Token *body; // Token_Body

	Token_DefFn(const Token *p_parent);
	~Token_DefFn( void );
};

struct Token_DefStruct : public Token
{
	Token *struct_name; // Token_Word
	Token *struct_body; // Token_Body

	Token_DefStruct(const Token *p_parent);
	~Token_DefStruct( void );
};

struct Token_File : public Token
{
	mtlChars   file_name;
	mtlString  content;
	Token     *body; // Token_Body

	Token_File(const Token *p_parent);
	~Token_File( void );
};

struct Token_Body : public Token
{
	mtlList<Token*> tokens;

	Token_Body(const Token *p_parent);
	~Token_Body( void );
};

struct Token_Set : public Token
{
	Token *lhs; // Token_Val
	Token *rhs; // Token_Expr

	Token_Set(const Token *p_parent);
	~Token_Set( void );
};

struct Token_CallFn : public Token
{
	Token           *fn_name; // Token_Word
	mtlList<Token*>  input;   // Token_Expr

	Token_CallFn(const Token *p_parent);
	~Token_CallFn( void );
};

struct Token_Var : public Token
{
	Token *var_decl;  // Token_DeclVar - DO NOT TRAVERSE OR DELETE THIS (CYCLICAL)
	Token *var_name;  // Token_Word
	Token *idx;       // Token_Expr
	Token *mem;       // Token_Var

	Token_Var(const Token *p_parent);
	~Token_Var( void );
};

struct Token_Lit : public Token
{
	Token *lit; // Token_Word
	enum LitType
	{
		BOOL,
		INT,
		FLOAT
	} lit_type;

	Token_Lit(const Token *p_parent);
	~Token_Lit( void );
};

struct Token_Expr : public Token
{
	Token    *lhs; // Token_Val, Token_Expr
	Token    *rhs; // Token_Val, Token_Expr
	mtlChars  op;

	Token_Expr(const Token *p_parent);
	~Token_Expr( void );
};

struct Token_If : public Token
{
	Token *cond;    // Token_Expr
	Token *if_body; // Token_Body
	Token *el_body; // Token_Body, Token_If

	Token_If(const Token *p_parent);
	~Token_If( void );
};

struct Token_While : public Token
{
	Token *cond; // Token_Expr
	Token *body; // Token_Body

	Token_While(const Token *p_parent);
	~Token_While( void );
};

struct Token_Ret : public Token
{
	Token *expr; // Token_Expr

	Token_Ret(const Token *p_parent);
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

// TODO: Syntax tree generator must do all detect all coding errors

class SyntaxTreeGenerator
{
private:
	Token *ProcessError(const mtlChars &msg, const mtlChars &err, const Token *parent);
	bool   FindVar(const mtlChars &name, const Token *parent);
	Token *ProcessFindVar(const mtlChars &name, const Token *parent);
	bool   VerifyName(const mtlChars &name);
	bool   NewName(const mtlChars &name, const Token *parent);
	Token *ProcessNewName(const mtlChars &name, const Token *parent);
	Token *ProcessFindType(const mtlChars &name, const Token *parent);
	Token *ProcessConst(const mtlChars &mut, const Token *parent);
	Token *ProcessRef(const mtlChars &ref, const Token *parent);
	Token *ProcessDecl(const mtlChars &rw, const mtlChars &type_name, const mtlChars &ref, const mtlChars &fn_name, const Token *parent);
	Token *ProcessFuncCall(const mtlChars &fn_name, const mtlChars &params, const Token *parent);
	Token *ProcessWord(const mtlChars &word, const Token *parent);
	Token *ProcessLiteral(const mtlChars &lit, const Token *parent);
	Token *ProcessVariable(mtlSyntaxParser &var, const Token *parent);
	Token *ProcessOperand(const mtlChars &val, const Token *parent);
	Token *ProcessSet(const mtlChars &lhs, const mtlChars &rhs, const Token *parent);
	Token *ProcessFuncDecl(const mtlChars &rw, const mtlChars &type_name, const mtlChars &ref, const mtlChars &fn_name, const mtlChars &params, const Token *parent);
	Token *ProcessOperation(const mtlChars &lhs, const mtlChars &op, const mtlChars &rhs, const Token *parent);
	Token *ProcessExpression(const mtlChars &expr, const Token *parent);
	Token *ProcessIf(const mtlChars &cond, const mtlChars &body, const Token *parent, mtlSyntaxParser &p);
	Token *ProcessWhile(const mtlChars &cond, const mtlChars &body, const Token *parent);
	Token *ProcessReturn(const mtlChars &expr, const Token *parent);
	Token *ProcessBody(const mtlChars &body, const Token *parent);
	Token *ProcessFuncDef(const mtlChars &rw, const mtlChars &type_name, const mtlChars &ref, const mtlChars &fn_name, const mtlChars &params, const mtlChars &body, const Token *parent);
	Token *ProcessStructMem(const mtlChars &decls);
	Token *ProcessStructDef(const mtlChars &struct_name, const mtlChars &decls, const Token *parent);
	Token *ProcessFile(const mtlChars &contents, const Token *parent);
	Token *LoadFile(const mtlChars &file_name, const Token *parent);

public:
	SyntaxTree *Generate(const mtlChars &entry_file);
};

}

#endif // SWSL_ASTGEN_H_INCLUDED__
