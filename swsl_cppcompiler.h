#ifndef SWSL_CPPCOMPILER_H_INCLUDED__
#define SWSL_CPPCOMPILER_H_INCLUDED__

#include "swsl_tokdisp.h"
#include "swsl_shader.h"

namespace swsl
{

class CppCompiler : swsl::TokenDispatcher
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
	void PrintType(const mtlChars &type);
	void OutputBinary(swsl::Binary &bin);

protected:
	void DispatchBody(const Token_Body *t);
	void DispatchCallFn(const Token_CallFn *t);
	void DispatchDeclFn(const Token_DeclFn *t);
	void DispatchDeclVar(const Token_DeclVar *t);
	void DispatchDefFn(const Token_DefFn *t);
	void DispatchDefStruct(const Token_DefStruct *t);
	void DispatchEntry(const SyntaxTree *t);
	void DispatchErr(const Token_Err *t);
	void DispatchExpr(const Token_Expr *t);
	void DispatchFile(const Token_File *t);
	void DispatchIf(const Token_If *t);
	void DispatchRet(const Token_Ret *t);
	void DispatchSet(const Token_Set *t);
	void DispatchVar(const Token_Var *t);
	void DispatchLit(const Token_Lit *);
	void DispatchWhile(const Token_While *t);

public:
	bool Compile(swsl::SyntaxTree *t, const mtlChars &bin_name, swsl::Binary &out_bin);
};

}

#endif // SWSL_CPPCOMPILER_H_INCLUDED__
