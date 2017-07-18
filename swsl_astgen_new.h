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
		BODY       = TYPE_DEF   << 1,
		SET        = BODY       << 1,
		EXPR       = SET        << 1,
		MATH_OP    = EXPR       << 1,
		FN_OP      = MATH_OP    << 1,
		VAR_OP     = FN_OP      << 1,
		LIT_OP     = VAR_OP     << 1,
		IF         = LIT_OP     << 1,
		ELSE       = IF         << 1,
		WHILE      = ELSE       << 1,
		RET        = WHILE      << 1,
		BODY       = RET        << 1
	};

	mtlString  contents;
	mtlChars   str;
	Type       type;
	new_Token *parent;
	new_Token *sub; // composite new_Tokens are split into subnew_Tokens
	new_Token *next;
	new_Token *ref; // references back to something, do not delete

	new_Token(const mtlChars &str_, Type type_, const new_Token *parent_);
	~new_Token( void );
};

class new_SyntaxTreeGenerator
{
private:
	bool IsReserved(const mtlChars &name);
	bool IsBuiltInType(const mtlChars &name);
	bool VerifyName(const mtlChars &name);
	bool CmpVarDeclName(const mtlChars &name, const new_Token *tok);
	bool CmpFnDeclName(const mtlChars &name, const new_Token *tok);
	bool CmpFnDefName(const mtlChars &name, const new_Token *tok);
	bool CmpVarDefName(const mtlChars &name, const new_Token *tok);
	bool NewName(const mtlChars &name, const new_Token *parent);
	bool IsCTConst(const new_Token *expr, bool &result) const;

private:
	new_Token *ProcessFile(const mtlChars &file_contents, const new_Token *parent);
	void       RemoveComments(mtlString &code) const;
	new_Token *LoadFile(const mtlChars &file_name, const new_Token *parent);
	new_Token *ProcessError(const mtlChars &msg, const mtlChars &err, const new_Token *parent);

public:
	const new_Token *Generate(const mtlChars &entry_file);
};

/*
Examples of an AST
	Note: we can actually use the below format, or convert it to JSON
		new_Token_TYPE "new_Token_CONTENTS"
			new_Token_TYPE "new_Token_CONTENTS"
			.
			.
			.
			new_Token_TYPE "new_TokenS_CONTENTS"
		new_Token_TYPE "new_Token_CONTENTS"

	new_Tokens on same indentation level denote sub new_Tokens of the parent new_Token
	Parent new_Token can be inferred from level
	Next new_Token can be inferred from new_Tokens on same level as previous new_Token
	Reference new_Token can be recalculated when the AST is loaded


	Input:
		import "file.swsl"

	Output:
		FILE "file.swsl"
			BODY "void main() {}" <-- Just an example of contents of file
				FN_DEF "void main() {}"
					VAR_DECL "void main"
						TYPE_NAME "void"
						USR_NAME "main"
					BODY ""

	Input:
		mutable float[3] x = { a, 1 - 3 * 4, b[a + 1] };

	Output:
		VAR_DECL "float[3] x = { a, 1 - 3 * 4, b[a + 1] }"
			TYPE_TRAIT "mutable"
			TYPE_NAME "float"
			EXPR "3"
				LIT_OP "3"
			USR_NAME "x"
			SET "{ a, 1 - 3 * 4, b[a + 1] }"
				EXPR "a"
					VAR_OP "a"
						USR_NAME "a"
				EXPR "1 - 3 * 4"
					EXPR "3 * 4"
						LIT_OP "3"
						MATH_OP "*"
						LIT_OP "4"
					EXPR "1 - "
						MATH_OP "-"
						LIT_OP "1"
				EXPR
					VAR_OP "b[a + 1]"
						USR_NAME "b"
						EXPR "a + 1"
							VAR_OP "a"
								USR_NAME "a"
							MATH_OP "+"
							LIT_OP "1"

	Input:
		readonly float func(float &a);

	Output:
		FN_DECL "readonly float func(float &a)"
			VAR_DECL "readonly float func"
				TYPE_TRAIT "readonly"
				TYPE_NAME "float"
				USR_NAME "func"
			VAR_DECL "float &a"
				TYPE_NAME "float"
				TYPE_TRAIT "&"
				USR_NAME "a"

*/

#endif // SWSL_ASTGEN_NEW_H
