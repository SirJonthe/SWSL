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
	int    errs;

	SyntaxTree( void );
	~SyntaxTree( void );
};

struct Token_Err : public Token
{
	mtlChars err;
	mtlChars msg;

	Token_Err(const Token *p_parent);
};

struct Token_Word : public Token
{
	mtlChars word;

	Token_Word(const Token *p_parent);
};

struct Token_DeclVar : public Token
{
	Token    *R_type_def; // Token_DefStruct - DO NOT TRAVERSE OR DELETE THIS (CYCLICAL)
	Token    *arr_size;   // Token_Expr
	mtlChars  type_name;  // Token_Word
	mtlChars  var_name;   // Token_Word
	bool      is_ref;
	bool      is_const;

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
	mtlChars  struct_name; // Token_Word
	Token    *struct_body; // Token_Body

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
	mtlChars         fn_name; // Token_Word
	mtlList<Token*>  input;   // Token_Expr

	Token_CallFn(const Token *p_parent);
	~Token_CallFn( void );
};

struct Token_Var : public Token
{
	Token    *R_var_decl; // Token_DeclVar - DO NOT TRAVERSE OR DELETE THIS (CYCLICAL)
	Token    *idx;        // Token_Expr
	Token    *mem;        // Token_Var
	mtlChars  var_name;   // Token_Word

	Token_Var(const Token *p_parent);
	~Token_Var( void );
};

struct Token_Lit : public Token
{
	mtlChars lit;
	enum LitType
	{
		TYPE_BOOL,
		TYPE_INT,
		TYPE_FLOAT
	} lit_type;

	Token_Lit(const Token *p_parent);
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
	SyntaxTree *root;

private:
	bool   IsReserved(const mtlChars &name);
	bool   VerifyName(const mtlChars &name);
	bool   CmpVarDeclName(const mtlChars &name, const Token_DeclVar *tok);
	bool   CmpFnDeclName(const mtlChars &name, const Token_DeclFn *tok);
	bool   CmpFnDefName(const mtlChars &name, const Token_DefFn *tok);
	bool   CmpStructDefName(const mtlChars &name, const Token_DefStruct *tok);
	bool   NewName(const mtlChars &name, const Token *parent);
	bool   FindVar(const mtlChars &name, const Token *parent); // RETURN TOKEN
	bool   FindType(const mtlChars &name, const Token *parent); // RETURN TOKEN

private:
	Token *ProcessError(const mtlChars &msg, const mtlChars &err, const Token *parent);
	Token *ProcessFindVar(const mtlChars &name, const Token *parent);
	Token *ProcessFindType(const mtlChars &name, const Token *parent);
	Token *ProcessDecl(const mtlChars &rw, const mtlChars &type_name, const mtlChars &ref, const mtlChars &fn_name, const Token *parent);
	Token *ProcessFuncCall(const mtlChars &fn_name, const mtlChars &params, const Token *parent);
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
