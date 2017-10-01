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
		LIST_EXPR  = FLOAT_EXPR << 1,
		MATH_OP    = LIST_EXPR  << 1,
		FN_OP      = MATH_OP    << 1,
		MEM_OP     = FN_OP      << 1,
		BOOL_OP    = MEM_OP     << 1,
		INT_OP     = BOOL_OP    << 1,
		FLOAT_OP   = INT_OP     << 1,
		IF         = FLOAT_OP   << 1,
		ELSE       = IF         << 1,
		WHILE      = ELSE       << 1,
		RET        = WHILE      << 1,
		BOOL_SET   = RET        << 1,
		INT_SET    = BOOL_SET   << 1,
		FLOAT_SET  = INT_SET    << 1,
		LIST_SET   = FLOAT_SET  << 1,
		VOID,
		BOOL,
		INT,
		FLOAT,
		LIST
	};

	// COLLAPSE ALL BOOL_*, INT_*, FLOAT_*, LIST_* TO BOOL, INT, FLOAT, LIST ???

	// Composite information in types can be passed and give more information with fewer
	// Con: unsure how to handle translating from SWSL -> C++
	// lit int a = 5 + b;
	// lit int a = LIT|INT|DECL
		// lit = TRAIT
		// int = TYPE|INT
		// a = LIT|INT|NAME
		// 5 + b = EXPR|INT|LIT
			// + = OP|INT|LIT
			// 5 = VAL|LIT|INT
			// b = VAL|LIT|INT
	// var float[2, 2] b = (5, 3 + I * x), (0, 3);
	// var float[2, 2] b = (5.4, 3.3 + I * x), (0.1, 3.0) = DECL
		// var = TRAIT
		// float = TYPE|FLOAT
		// [2, 2] = LIST|EXPR|INT|LIT|VAL
			// 2 = INT|LIT
			// 2 = INT|LIT
		// b = INT|NAME|VAR
		// = = OP|INT|VAR
		// (5.4, 3.3 + I * x), (0.1, 3.0) = EXPR|LIST|VAR
			// 5.4, 3.3 + I * x
				// 5.4 = LIT|FLOAT
				// 3.3 + I * x = EXPR|FLOAT|VAR
					// * = OP|FLOAT|VAR
					// I = LIT|FLOAT
					// x = VAR|FLOAT
					// + = OP|FLOAT|VAR
					// 3.3 = LIT|FLOAT
			// 0.1, 3.0

	// ref - var, imm, lit
	// val - lit
	// fn - var, imm
	//

	/*enum Type
	{
		ERR = 1,
		DECL = ERR << 1,
		DEF = DECL << 1,
		FN = DEF << 1,
		MEM = FN << 1,
		LIT = MEM << 1,
		IMM = LIT << 1,
		VAR = IMM << 1,
		BOOL = VAR << 1,
		INT = BOOL << 1,
		FLOAT = INT << 1,
		LIST = FLOAT << 1,
		IF = LIST << 1,
		WHILE = IF << 1,
		RET = WHILE << 1,
		OP = RET << 1,
	};*/

	mtlString        buffer;
	mtlChars         str;
	Type             type;
	unsigned int     data;
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
		bool     IsEnd( void ) const;
		int      Match(const mtlChars &expr);
		mtlChars GetMatch(int i) const;
		mtlChars Seq( void ) const;
		mtlChars Rem( void ) const;
		bool     Consume(const mtlChars &str);
		void     ConsumeAll(const mtlChars &str);
	};

private:
	bool IsReserved(const mtlChars &name) const;
	bool IsBuiltInType(const mtlChars &name) const;
	bool IsKeyword(const mtlChars &name) const;
	bool IsUnusedReserved(const mtlChars &name) const;
	bool IsValidNameConvention(const mtlChars &name) const;
	bool IsNewName(const mtlChars &name, const new_Token *parent) const;
	bool IsValidName(const mtlChars &name, const new_Token *parent) const;
	bool IsCTConst(const new_Token *expr, unsigned int op_type, bool &result) const;
	bool IsCTConst(const new_Token *expr, unsigned int op_type) const;
	const new_Token *GetTypeFromDecl(const new_Token *decl) const;
	const new_Token *GetTraitFromDecl(const new_Token *decl) const;
	const new_Token *GetSizeFromDecl(const new_Token *decl) const;
	const new_Token *GetTypeFromName(const mtlChars &name, const new_Token *scope) const;
	const new_Token *GetTraitFromName(const mtlChars &name, const new_Token *scope) const;
	const new_Token *GetSizeFromName(const mtlChars &name, const new_Token *scope) const;
	bool CreateTypeList(const mtlChars &type_name, mtlString &out, const new_Token *parent) const;
	bool CreateParamList(const mtlChars &fn_name, mtlString &out, const new_Token *parent) const;
	new_Token::Type GetTypeID(const mtlChars &name, const new_Token *scope) const;

private:
	bool             EvalConstBoolExpr(const new_Token *token) const;
	int              EvalConstIntExpr(const new_Token *token) const;
	float            EvalConstFloatExpr(const new_Token *token) const;
	new_Token       *ProcessListExpr(const mtlChars &expr, const mtlChars &arr_size, const mtlChars &list, const new_Token *parent) const;
	new_Token       *ProcessListExpr(const mtlChars &expr, const mtlChars &list, const new_Token *parent) const;
	new_Token       *ProcessBoolExpr(const mtlChars &expr, const mtlChars &arr_size, const new_Token *parent) const;
	new_Token       *ProcessBoolExpr(const mtlChars &expr, const new_Token *parent) const;
	new_Token       *ProcessIntExpr(const mtlChars &expr, const mtlChars &arr_size, const new_Token *parent) const;
	new_Token       *ProcessIntExpr(const mtlChars &expr, const new_Token *parent) const;
	new_Token       *ProcessFloatExpr(const mtlChars &expr, const mtlChars &arr_size, const new_Token *parent) const;
	new_Token       *ProcessFloatExpr(const mtlChars &expr, const new_Token *parent) const;
	new_Token       *ProcessMemOp(const mtlChars &op, const new_Token *parent) const;
	new_Token       *ProcessMemOpMember(Parser &p, const new_Token *parent) const;
	new_Token       *ProcessMathOp(const mtlChars &lhs, const mtlChars &op, const mtlChars &rhs, unsigned int op_type, const new_Token *parent) const;
	new_Token       *ProcessSet(const mtlChars &lhs, const mtlChars &rhs, const mtlChars &seq, const new_Token *parent) const;
	new_Token       *ProcessIf(const mtlChars &cond, const mtlChars &body, const new_Token *parent, Parser &p) const;
	new_Token       *ProcessWhile(const mtlChars &cond, const mtlChars &body, const mtlChars &seq, const new_Token *parent) const;
	new_Token       *ProcessRet(const mtlChars &expr, const new_Token *parent) const;
	new_Token       *ProcessConstBoolExpr(const mtlChars &expr, const mtlChars &arr_size, const new_Token *parent) const;
	new_Token       *ProcessConstBoolExpr(const mtlChars &expr, const new_Token *parent) const;
	new_Token       *ProcessConstIntExpr(const mtlChars &expr, const mtlChars &arr_size, const new_Token *parent) const;
	new_Token       *ProcessConstIntExpr(const mtlChars &expr, const new_Token *parent) const;
	new_Token       *ProcessConstFloatExpr(const mtlChars &expr, const mtlChars &arr_size, const new_Token *parent) const;
	new_Token       *ProcessConstFloatExpr(const mtlChars &expr, const new_Token *parent) const;
	new_Token       *ProcessArraySize(const mtlChars &arr_size, const new_Token *parent) const;
	new_Token       *ProcessType(const mtlChars &type_name, const new_Token *parent) const;
	new_Token       *ProcessTrait(const mtlChars &trait, const new_Token *parent) const;
	new_Token       *ProcessDeclVar(const mtlChars &trait, const mtlChars &type_name, const mtlChars &arr_size, const mtlChars &var_name, const mtlChars &expr, const mtlChars &seq, const new_Token *parent) const;
	new_Token       *ProcessDeclParamVar(const mtlChars &trait, const mtlChars &type_name, const mtlChars &arr_size, const mtlChars &var_name, const mtlChars &seq, const new_Token *parent) const;
	new_Token       *ProcessScope(const mtlChars &scope, const new_Token *parent) const;
	new_Token       *ProcessNewName(const mtlChars &name, const new_Token *parent) const;
	new_Token       *ProcessRefName(const mtlChars &name, const new_Token *parent) const;
	new_Token       *ProcessRefMemName(const mtlChars &name, const new_Token *parent) const;
	const new_Token *FindName(const mtlChars &name, const new_Token *token) const;
	new_Token       *ProcessDeclParam(new_Token **&token, const mtlChars &param, const new_Token *parent) const;
	//new_Token       *ProcessExpr(const mtlChars &expr, const new_Token *parent) const;
	new_Token       *ProcessDeclFn(const mtlChars &type_name, const mtlChars &arr_size, const mtlChars &fn_name, const mtlChars &params, const mtlChars &seq, const new_Token *parent) const;
	new_Token       *ProcessDefFn(const mtlChars &type_name, const mtlChars &arr_size, const mtlChars &fn_name, const mtlChars &params, const mtlChars &body, const mtlChars &seq, const new_Token *parent) const;
	new_Token       *ProcessDefType(const mtlChars &type_name, const mtlChars &decls, const mtlChars &seq, const new_Token *parent) const;
	new_Token       *ProcessFile(const mtlChars &file_contents, const new_Token *parent) const;
	void             RemoveComments(mtlString &code) const;
	new_Token       *LoadFile(const mtlChars &file_name, const new_Token *parent) const;
	new_Token       *ProcessError(const mtlChars &msg, mtlChars err, const new_Token *parent) const;

public:
	const new_Token *Generate(const mtlChars &entry_file) const;
};

#endif // SWSL_ASTGEN_NEW_H
