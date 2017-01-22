#include "swsl_tokdisp.h"

void swsl::TokenDispatcher::Dispatch(const swsl::Token *t)
{
	if (t != NULL) {
		switch (t->type) {
		case swsl::Token::TOKEN_ERR:
			DispatchErr(dynamic_cast<const Token_Err*>(t));
			break;

		case swsl::Token::TOKEN_START:
			DispatchEntry(dynamic_cast<const SyntaxTree*>(t));
			break;

		case swsl::Token::TOKEN_DECL_VAR:
			DispatchDeclVar(dynamic_cast<const Token_DeclVar*>(t));
			break;

		case swsl::Token::TOKEN_DECL_FN:
			DispatchDeclFn(dynamic_cast<const Token_DeclFn*>(t));
			break;

		case swsl::Token::TOKEN_DEF_FN:
			DispatchDefFn(dynamic_cast<const Token_DefFn*>(t));
			break;

		case swsl::Token::TOKEN_DEF_STRUCT:
			DispatchDefStruct(dynamic_cast<const Token_DefStruct*>(t));
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

		case swsl::Token::TOKEN_CALL_FN:
			DispatchCallFn(dynamic_cast<const Token_CallFn*>(t));
			break;

		case swsl::Token::TOKEN_VAL:
			DispatchVal(dynamic_cast<const Token_Val*>(t));
			break;

		case swsl::Token::TOKEN_EXPR:
			DispatchExpr(dynamic_cast<const Token_Expr*>(t));
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
