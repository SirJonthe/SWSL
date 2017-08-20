#ifndef SWSL_ASTGEN_NEW_H
#define SWSL_ASTGEN_NEW_H

#include "MiniLib/MTL/mtlString.h"
#include "MiniLib/MTL/mtlParser.h"

struct new_Token
{
	enum Type
	{
		ERR        = 1,
		FILE       = ERR        << 1,
		TYPE_NAME  = FILE       << 1,
		USR_NAME   = TYPE_NAME  << 1,
		FN_DECL    = USR_NAME   << 1,
		VAR_DECL   = FN_DECL    << 1,
		TYPE_TRAIT = VAR_DECL   << 1,
		FN_DEF     = TYPE_TRAIT << 1,
		TYPE_DEF   = FN_DEF     << 1,
		SCOPE      = TYPE_DEF   << 1,
		BOOL_EXPR  = SCOPE      << 1,
		INT_EXPR   = BOOL_EXPR  << 1,
		FLOAT_EXPR = INT_EXPR   << 1,
		MATH_OP    = FLOAT_EXPR << 1,
		FN_OP      = MATH_OP    << 1,
		MEM_OP     = FN_OP      << 1,
		BOOL_OP    = MEM_OP     << 1,
		INT_OP     = BOOL_OP    << 1,
		FLOAT_OP   = INT_OP     << 1,
		IF         = FLOAT_OP   << 1,
		ELSE       = IF         << 1,
		WHILE      = ELSE       << 1,
		RET        = WHILE      << 1,
		EXPR       = RET        << 1, // DEPRECATE IN FAVOR OF BOOL_EXPR / INT_EXPR / FLOAT_EXPR
		SET        = EXPR       << 1  // DEPRECATE IN FAVOR OF MATH_OP
	};

	mtlString        buffer;
	mtlChars         str;
	Type             type;
	const new_Token *parent;
	new_Token       *sub; // composite tokens are split into sub tokens
	new_Token       *next;
	const new_Token *ref; // references back to something, do not delete

	new_Token(const mtlChars &str_, Type type_, const new_Token *parent_);
	~new_Token( void );
	int CountAscend(unsigned int type_mask) const;
	int CountDescend(unsigned int type_mask) const;
};

class new_SyntaxTreeGenerator
{
private:

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
		mtlChars GetMatch(int i) const;
		mtlChars Seq( void ) const;
		mtlChars Rem( void ) const;
	};

private:
	bool IsReserved(const mtlChars &name) const;
	bool IsBuiltInType(const mtlChars &name) const;
	bool IsKeyword(const mtlChars &name) const;
	bool IsUnusedReserved(const mtlChars &name) const;
	bool IsValidNameConvention(const mtlChars &name) const;
	bool IsNewName(const mtlChars &name, const new_Token *parent) const;
	bool IsValidName(const mtlChars &name, const new_Token *parent) const;
	bool IsCTConst(const new_Token *expr, bool &result) const;
	bool IsCTConst(const new_Token *expr) const;

private:
	new_Token       *ProcessMemOp(const mtlChars &op, const new_Token *parent) const;
	new_Token       *ProcessMemOpMember(Parser &p, const new_Token *parent) const;
	new_Token       *ProcessMathOp(const mtlChars &lhs, const mtlChars &op, const mtlChars &rhs, const new_Token *parent) const;
	new_Token       *ProcessSet(const mtlChars &lhs, const mtlChars &rhs, const new_Token *parent) const;
	new_Token       *ProcessIf(const mtlChars &cond, const mtlChars &body, const new_Token *parent, Parser &p) const;
	new_Token       *ProcessWhile(const mtlChars &cond, const mtlChars &body, const new_Token *parent) const;
	new_Token       *ProcessRet(const mtlChars &expr, const new_Token *parent) const;
	new_Token       *ProcessConstExpr(const mtlChars &expr, const new_Token *parent) const;
	new_Token       *ProcessArraySize(const mtlChars &arr_size, const new_Token *parent) const;
	new_Token       *ProcessType(const mtlChars &type_name, const new_Token *parent) const;
	new_Token       *ProcessTrait(const mtlChars &trait, const new_Token *parent) const;
	new_Token       *ProcessDeclVar(const mtlChars &trait, const mtlChars &type_name, const mtlChars &arr_size, const mtlChars &var_name, const mtlChars &expr, const new_Token *parent) const;
	new_Token       *ProcessScope(const mtlChars &scope, const new_Token *parent) const;
	new_Token       *ProcessNewName(const mtlChars &name, const new_Token *parent) const;
	new_Token       *ProcessRefName(const mtlChars &name, const new_Token *parent) const;
	new_Token       *ProcessRefMemName(const mtlChars &name, const new_Token *parent) const;
	const new_Token *FindName(const mtlChars &name, const new_Token *token) const;
	void             ProcessDeclParam(new_Token **token, const mtlChars &param, const new_Token *parent) const;
	new_Token       *ProcessExpr(const mtlChars &expr, const new_Token *parent) const;
	new_Token       *ProcessDeclFn(const mtlChars &type_name, const mtlChars &arr_size, const mtlChars &fn_name, const mtlChars &params, const new_Token *parent) const;
	new_Token       *ProcessDefFn(const mtlChars &type_name, const mtlChars &arr_size, const mtlChars &fn_name, const mtlChars &params, const mtlChars &body, const new_Token *parent) const;
	new_Token       *ProcessDefType(const mtlChars &type_name, const mtlChars &decls, const new_Token *parent) const;
	new_Token       *ProcessFile(const mtlChars &file_contents, const new_Token *parent) const;
	void             RemoveComments(mtlString &code) const;
	new_Token       *LoadFile(const mtlChars &file_name, const new_Token *parent) const;
	new_Token       *ProcessError(const mtlChars &msg, mtlChars err, const new_Token *parent) const;

public:
	const new_Token *Generate(const mtlChars &entry_file) const;
};

#endif // SWSL_ASTGEN_NEW_H
