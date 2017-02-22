#include "swsl_tokdisp.h"

void swsl::TokenDispatcher::Dispatch(const swsl::Token *t)
{
	if (t != NULL) {
		switch (t->type) {
		case swsl::Token::TOKEN_ERR:
			DispatchErr(dynamic_cast<const Token_Err*>(t));
			break;

		case swsl::Token::TOKEN_ROOT:
			DispatchRoot(dynamic_cast<const SyntaxTree*>(t));
			break;

		case swsl::Token::TOKEN_ALIAS:
			DispatchAlias(dynamic_cast<const Token_Alias*>(t));
			break;

		case swsl::Token::TOKEN_DECL_TYPE:
			DispatchDeclType(dynamic_cast<const Token_DeclType*>(t));
			break;

		case swsl::Token::TOKEN_DECL_VAR:
			DispatchDeclVar(dynamic_cast<const Token_DeclVar*>(t));
			break;

		case swsl::Token::TOKEN_DECL_FN:
			DispatchDeclFn(dynamic_cast<const Token_DeclFn*>(t));
			break;

		case swsl::Token::TOKEN_DEF_TYPE:
			DispatchDefType(dynamic_cast<const Token_DefType*>(t));
			break;

		case swsl::Token::TOKEN_DEF_FN:
			DispatchDefFn(dynamic_cast<const Token_DefFn*>(t));
			break;

		case swsl::Token::TOKEN_FILE:
			DispatchFile(dynamic_cast<const Token_File*>(t));
			break;

		case swsl::Token::TOKEN_BODY:
			DispatchBody(dynamic_cast<const Token_Body*>(t));
			break;

		case swsl::Token::TOKEN_SET:
			DispatchSet(dynamic_cast<const Token_Set*>(t));
			break;

		case swsl::Token::TOKEN_EXPR:
			DispatchExpr(dynamic_cast<const Token_Expr*>(t));
			break;

		case swsl::Token::TOKEN_READ_FN:
			DispatchReadFn(dynamic_cast<const Token_ReadFn*>(t));
			break;

		case swsl::Token::TOKEN_READ_VAR:
			DispatchReadVar(dynamic_cast<const Token_ReadVar*>(t));
			break;

		case swsl::Token::TOKEN_READ_LIT:
			DispatchReadLit(dynamic_cast<const Token_ReadLit*>(t));
			break;

		case swsl::Token::TOKEN_IF:
			DispatchIf(dynamic_cast<const Token_If*>(t));
			break;

		case swsl::Token::TOKEN_WHILE:
			DispatchWhile(dynamic_cast<const Token_While*>(t));
			break;

		case swsl::Token::TOKEN_RET:
			DispatchRet(dynamic_cast<const swsl::Token_Ret*>(t));
			break;

		default:
			DispatchNull();
			break;
		}
	} else {
		DispatchNull();
	}
}

void swsl::TokenDispatcher::Dispatch(const mtlList<swsl::Token*> &t)
{
	const mtlItem<swsl::Token*> *i = t.GetFirst();
	while (i != NULL) {
		Dispatch(i->GetItem());
		i = i->GetNext();
	}
}
