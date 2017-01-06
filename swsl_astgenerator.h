#ifndef SWSL_ASTGENERATOR_H_INCLUDED__
#define SWSL_ASTGENERATOR_H_INCLUDED__

#include "MiniLib/MTL/mtlString.h"

namespace swsl
{

enum ast_token
{
	DECL, // instansiate a var or function
	DEF, // tell compiler blue prints
	VAR, // variable, built in or user defined
	DATA, // struct, custom
	FN, // function
	IMM, // const
	MUT, // mutable
	REF, // & reference
	LIT, // 123, 1.3, true, false, etc.
	BOOL, // bool
	FL, // float
	FL2, // float2
	FL3, // float3
	FL4, // float4
	SET, // left hand side assignment
	EXPR, // right hand side assignment
	ADD, // +
	SUB, // -
	MUL, // *
	DIV, // /
	EQ, // ==
	NEQ, // !=
	LT, // <
	LET, // <=
	GT, // >
	GET, // >=
	AND, // &&
	OR // ||
};

struct file_node
{
};

struct ast_node
{
public:
	ast_token  token;
	ast_node  *next;
};

struct decl_node
{
public:
	union {
		var_node var;
		fn_node  fn;
	} node;
};

struct def_node
{
public:
	var_node var;
};

struct fn_node
{
public:
	def_node fn;
	def_node *prm;
};

struct var_node
{
public:
	mtlChars var_name;
	mtlChars type_name;
};

}

#endif // SWSL_ASTGENERATOR_H_INCLUDED__
