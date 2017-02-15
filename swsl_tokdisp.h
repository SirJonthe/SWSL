#ifndef SWSL_TOKDISP_H_INCLUDED__
#define SWSL_TOKDISP_H_INCLUDED__

#include "swsl_astgen.h"

namespace swsl
{

class TokenDispatcher
{
protected:
	virtual void DispatchNull( void )                    {}
	virtual void DispatchErr(const Token_Err*)           = 0;
	virtual void DispatchRoot(const SyntaxTree*)         = 0;
	virtual void DispatchAlias(const Token_Alias*)       = 0;
	virtual void DispatchDeclType(const Token_DeclType*) = 0;
	virtual void DispatchDeclVar(const Token_DeclVar*)   = 0;
	virtual void DispatchDeclFn(const Token_DeclFn*)     = 0;
	virtual void DispatchDefVar(const Token_DefVar*)     = 0;
	virtual void DispatchDefFn(const Token_DefFn*)       = 0;
	virtual void DispatchFile(const Token_File*)         = 0;
	virtual void DispatchBody(const Token_Body*)         = 0;
	virtual void DispatchSet(const Token_Set*)           = 0;
	virtual void DispatchExpr(const Token_Expr*)         = 0;
	virtual void DispatchReadFn(const Token_ReadFn*)     = 0;
	virtual void DispatchReadVar(const Token_ReadVar*)   = 0;
	virtual void DispatchReadLit(const Token_ReadLit*)   = 0;
	virtual void DispatchIf(const Token_If*)             = 0;
	virtual void DispatchWhile(const Token_While*)       = 0;
	virtual void DispatchRet(const Token_Ret*)           = 0;

public:
	virtual ~TokenDispatcher( void ) {}

	void Dispatch(const swsl::Token *t);
	void Dispatch(const mtlList<swsl::Token*> &t);
};

}

#endif // SWSL_TOKDISP_H_INCLUDED__
