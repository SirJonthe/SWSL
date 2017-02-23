#ifndef SWSL_CPPCOMPILER_H_INCLUDED__
#define SWSL_CPPCOMPILER_H_INCLUDED__

#include "swsl_tokdisp.h"
#include "swsl_shader.h"

namespace swsl
{

class CppCompiler : public swsl::TokenDispatcher
{
private:
	mtlArray<char> m_buffer;
	int            m_cond_depth;
	int            m_depth;
	int            m_errs;
	mtlChars       m_bin_name;

private:
	void PrintTabs( void );
	void Print(const mtlChars &ch);
	void PrintNewline( void );
	void PrintMask( void );
	void PrintPrevMask( void );
	void PrintVarName(const mtlChars &name);
	void PrintType(const mtlChars &type);
	void PrintReturnMerge( void );
	void OutputBinary(swsl::Binary &bin);
	bool IsType(const Token *token, Token::TokenType type);

protected:
	void DispatchAlias(const Token_Alias *t);
	void DispatchBody(const Token_Body *t);
	void DispatchDeclFn(const Token_DeclFn *t);
	void DispatchDeclType(const Token_DeclType *t);
	void DispatchDeclVar(const Token_DeclVar *t);
	void DispatchDefFn(const Token_DefFn *t);
	void DispatchDefType(const Token_DefType *t);
	void DispatchErr(const Token_Err *t);
	void DispatchExpr(const Token_Expr *t);
	void DispatchFile(const Token_File *t);
	void DispatchIf(const Token_If *t);
	void DispatchReadFn(const Token_ReadFn *t);
	void DispatchReadLit(const Token_ReadLit *t);
	void DispatchReadVar(const Token_ReadVar *t);
	void DispatchRet(const Token_Ret *t);
	void DispatchRoot(const SyntaxTree *t);
	void DispatchSet(const Token_Set *t);
	void DispatchWhile(const Token_While *t);

private:
	void DispatchTypeName(const Token *t);
	void DispatchCompatMain(const Token_DefFn *t);

public:
	bool Compile(swsl::SyntaxTree *t, const mtlChars &bin_name, swsl::Binary &out_bin);
};

}

#endif // SWSL_CPPCOMPILER_H_INCLUDED__
