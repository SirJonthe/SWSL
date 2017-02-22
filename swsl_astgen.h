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
		TOKEN_ERR        = 1,
		TOKEN_ROOT       = TOKEN_ERR       << 1,
		TOKEN_ALIAS      = TOKEN_ROOT      << 1,
		TOKEN_DECL_TYPE  = TOKEN_ALIAS     << 1,
		TOKEN_DECL_VAR   = TOKEN_DECL_TYPE << 1,
		TOKEN_DECL_FN    = TOKEN_DECL_VAR  << 1,
		TOKEN_DEF_TYPE   = TOKEN_DECL_FN   << 1,
		TOKEN_DEF_FN     = TOKEN_DEF_TYPE  << 1,
		TOKEN_FILE       = TOKEN_DEF_FN    << 1,
		TOKEN_BODY       = TOKEN_FILE      << 1,
		TOKEN_SET        = TOKEN_BODY      << 1,
		TOKEN_EXPR       = TOKEN_SET       << 1,
		TOKEN_READ_FN    = TOKEN_EXPR      << 1,
		TOKEN_READ_VAR   = TOKEN_READ_FN   << 1,
		TOKEN_READ_LIT   = TOKEN_READ_VAR  << 1,
		TOKEN_IF         = TOKEN_READ_LIT  << 1,
		TOKEN_WHILE      = TOKEN_IF        << 1,
		TOKEN_RET        = TOKEN_WHILE     << 1
	};

	const Token     *const parent;
	const TokenType        type;

	Token(const Token *p_parent, TokenType p_type) :
		parent(p_parent), type(p_type)
	{}
	virtual ~Token( void ) {}
};

struct Token_Err : public Token
{
	mtlChars err;
	mtlChars msg;

	Token_Err(const Token *p_parent);
};

struct SyntaxTree : public Token
{
	Token *file; // Token_File
	int    errs;

	SyntaxTree( void );
	~SyntaxTree( void );
};

struct Token_Alias : public Token
{
	mtlChars alias;
	int      scope;

	Token_Alias(const Token *p_parent);
};

struct Token_DeclType : public Token
{
	mtlChars  type_name;
	Token    *arr_size; // NOTE: This must be evaluated to a constant at compile time, else error
	bool      is_ref;
	bool      is_const;
	bool      is_user_def;

	Token_DeclType(const Token *p_parent);
	~Token_DeclType( void );
};

struct Token_DeclVar : public Token
{
	Token    *decl_type;  // Token_DeclType
	Token    *expr;       // Token_Expr
	mtlChars  var_name;
	bool      is_ct_const;

	Token_DeclVar(const Token *p_parent);
	~Token_DeclVar( void );
};

struct Token_DeclFn : public Token
{
	Token           *decl_type; // Token_DeclType
	mtlChars         fn_name;
	mtlList<Token*>  params;    // Token_DeclVar

	Token_DeclFn(const Token *p_parent);
	~Token_DeclFn( void );
};

struct Token_DefType : public Token
{
	mtlChars  type_name;
	Token    *body;      // Token_Body

	Token_DefType(const Token *p_parent);
	~Token_DefType( void );
};

struct Token_DefFn : public Token
{
	Token           *decl_type; // Token_DeclType
	mtlChars         fn_name;
	mtlList<Token*>  params;    // Token_DeclVar
	Token           *body;      // Token_Body

	Token_DefFn(const Token *p_parent);
	~Token_DefFn( void );
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
	mtlList<Token*> tokens; // Token_Body, Token_Set, Token_If, Token_While, Token_DeclVar

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

struct Token_Expr : public Token
{
	Token    *lhs; // Token_ReadVal, Token_ReadLit, Token_ReadFn, Token_Expr
	Token    *rhs; // Token_ReadVal, Token_ReadLit, Token_ReadFn, Token_Expr
	mtlChars  op;

	Token_Expr(const Token *p_parent);
	~Token_Expr( void );
};

struct Token_ReadFn : public Token
{
	const Token_DeclType *decl_type;
	mtlChars              fn_name;
	mtlList<Token*>       input;   // Token_Expr

	Token_ReadFn(const Token *p_parent);
	~Token_ReadFn( void );
};

struct Token_ReadVar : public Token
{
	const Token_DeclType *decl_type; // Token_DeclType
	Token                *idx;       // Token_Expr
	Token                *mem;       // Token_Var
	mtlChars              var_name;

	Token_ReadVar(const Token *p_parent);
	~Token_ReadVar( void );
};

struct Token_ReadLit : public Token
{
	mtlChars lit;
	enum LitType
	{
		TYPE_BOOL,
		TYPE_INT,
		TYPE_FLOAT
	} lit_type;

	Token_ReadLit(const Token *p_parent);
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
	bool   IsBuiltInType(const mtlChars &name);
	bool   VerifyName(const mtlChars &name);
	bool   CmpVarDeclName(const mtlChars &name, const Token_DeclVar *tok);
	bool   CmpFnDeclName(const mtlChars &name, const Token_DeclFn *tok);
	bool   CmpFnDefName(const mtlChars &name, const Token_DefFn *tok);
	bool   CmpVarDefName(const mtlChars &name, const Token_DefType *tok);
	bool   NewName(const mtlChars &name, const Token *parent);
	const Token *FindName(const mtlChars &name, const Token *parent);
	const Token_DefType *FindDefType(const mtlChars &name, const Token *parent);
	const Token_DeclFn *FindDeclFn(const mtlChars &name, const Token *parent);
	const Token_DeclVar *FindDeclVar(const mtlChars &name, const Token *parent);
	const Token_DeclType *FindDeclType(const mtlChars &var_name, const Token *parent);

private:
	Token *ProcessError(const mtlChars &msg, mtlChars err, const Token *parent);
	Token *ProcessDeclType(const mtlChars &rw, const mtlChars &type_name, const mtlChars &arr_size, const mtlChars &ref, const Token *parent);
	Token *ProcessDeclVar(const mtlChars &rw, const mtlChars &type_name, const mtlChars &arr_size, const mtlChars &ref, const mtlChars &var_name, const mtlChars &expr, const Token *parent);
	Token *ProcessReadFn(const mtlChars &fn_name, const mtlChars &params, const Token *parent);
	Token *ProcessReadLit(const mtlChars &lit, const Token *parent);
	Token *ProcessReadVar(mtlSyntaxParser &var, const Token *parent);
	Token *ProcessOperand(const mtlChars &val, const Token *parent);
	Token *ProcessSet(const mtlChars &lhs, const mtlChars &rhs, const Token *parent);
	Token *ProcessFuncDecl(const mtlChars &rw, const mtlChars &type_name, const mtlChars &arr_size, const mtlChars &ref, const mtlChars &fn_name, const mtlChars &params, const Token *parent);
	Token *ProcessOperation(const mtlChars &lhs, const mtlChars &op, const mtlChars &rhs, const Token *parent);
	Token *ProcessExpression(const mtlChars &expr, const Token *parent);
	Token *ProcessIf(const mtlChars &cond, const mtlChars &body, const Token *parent, mtlSyntaxParser &p);
	Token *ProcessWhile(const mtlChars &cond, const mtlChars &body, const Token *parent);
	Token *ProcessReturn(const mtlChars &expr, const Token *parent);
	Token *ProcessBody(const mtlChars &body, const Token *parent);
	void   ProcessParamDecl(const mtlChars &params, mtlList<Token*> &out_params, const Token *parent);
	Token *ProcessFuncDef(const mtlChars &rw, const mtlChars &type_name, const mtlChars &arr_size, const mtlChars &ref, const mtlChars &fn_name, const mtlChars &params, const mtlChars &body, const Token *parent);
	Token *ProcessTypeMemDecl(const mtlChars &decls);
	Token *ProcessTypeDef(const mtlChars &struct_name, const mtlChars &decls, const Token *parent);
	Token *ProcessFile(const mtlChars &contents, const Token *parent);
	Token *LoadFile(const mtlChars &file_name, const Token *parent);

public:
	SyntaxTree *Generate(const mtlChars &entry_file);
};

}

#endif // SWSL_ASTGEN_H_INCLUDED__
