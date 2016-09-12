#ifndef COMPILER_H
#define COMPILER_H

#include "MiniLib/MTL/mtlList.h"
#include "MiniLib/MTL/mtlStringMap.h"
#include "MiniLib/MTL/mtlParser.h"

#include "swsl_shader.h"

class Compiler
{
private:
	enum NodeType
	{
		NodeType_Add,
		NodeType_Sub,
		NodeType_Mul,
		NodeType_Div,

		NodeType_Eq,
		NodeType_Neq,
		NodeType_Less,
		NodeType_LessEq,
		NodeType_Greater,
		NodeType_GreaterEq,
		NodeType_And,
		NodeType_Or,

		NodeType_Paranthesis,

		NodeType_Func,
		NodeType_Term
	};

	struct ExprNode
	{
		mtlChars str;
		NodeType type;

		virtual ~ExprNode( void ) {}
		virtual void Eval(mtlString &out) = 0;
		virtual bool IsEnd( void ) const = 0;
	};

	struct ExprBranch : public ExprNode
	{
		ExprNode *left;
		ExprNode *right;

		~ExprBranch( void ) { delete left; delete right; }
		void Eval(mtlString &out) { left->Eval(out); out.Append(str); right->Eval(out); }
		bool IsEnd( void ) const { return false; }
		// 1 stack variable needed
		// if there is a branch and right path leads to another branch then another stack variable is needed
	};

	struct ExprTerm : public ExprNode
	{
		~ExprTerm( void ) {}
		void Eval(mtlString &out) { out.Append(str); }
		bool IsEnd( void ) const { return true; }
	};

	enum Token
	{
		Token_If, Token_Else, Token_For, Token_While,          // Conditionals
		Token_Continue, Token_Break, Token_Return,             // Flow control
		Token_Const, Token_Mutable,                            // Type traits
		Token_Import, Token_Export,                            // Availability (include files, only exported symbols visible)
		Token_Struct,                                          // User defined types
		Token_Void,                                            // Empty type
		Token_Bool,  Token_True,   Token_False,                // Boolean
		Token_Int,   Token_Int2,   Token_Int3,   Token_Int4,   // Integers
		Token_Fixed, Token_Fixed2, Token_Fixed3, Token_Fixed4, // Fixed 16.16
		Token_Unit,  Token_Unit2,  Token_Unit3,  Token_Unit4,  // Fixed 00.32
		Token_Float, Token_Float2, Token_Float3, Token_Float4, // Floating point
		Token_Count
	};

	struct File
	{
		mtlPath   m_path;
		mtlString m_contents;
	};

	enum SymbolType
	{
		SymbolType_Variable,
		SymbolType_Struct,
		SymbolType_Function,
		SymbolType_Unknown
	};

	enum VariableAccess
	{
		VariableAccess_Mutable,
		VariableAccess_Const
	};

	enum VariableType
	{
		VariableType_Void,
		VariableType_Bool,
		VariableType_Int,
		VariableType_Fixed,
		VariableType_Unit,
		VariableType_Float,
		VariableType_Struct,
		VariableType_Unknown
	};

	struct TypeInfo
	{
		mtlChars       name;
		VariableType   type;
		VariableAccess access;
		int            size;
	};

	// list of symbols
	// if found, we know what kind of symbol
	// list of funcs, vars, structs
	// scope contains all lists
	// struct contains all scopes
	// scope struct evaluates expressions

	struct Variable
	{
		mtlChars     name;
		TypeInfo     type_info;
		swsl::addr_t addr;
	};

	struct Function
	{
		mtlChars          name;
		TypeInfo          ret_type;
		mtlList<TypeInfo> param_type;
		mtlChars          body;
		swsl::addr_t      jmp_addr; // points to jump instruction leading to first instruction of function body
		// room for function return value must be allocated on stack when function is called
	};

	struct Struct
	{
		mtlChars          name;
		mtlList<mtlChars> mems; // will not work for structs containing structs
	};

	struct Scope
	{
		mtlStringMap<Variable> var;
		mtlStringMap<Struct>   strc;
		int                    stack_size;
		int                    stack_offset;
	};

public:
	struct Message
	{
		mtlString err;
		mtlString msg;
	};

private:
	mtlList<swsl::Instruction>           m_out;
	mtlList<Scope>                       m_scopes;
	mtlList< mtlStringMap<SymbolType> >  m_sym;
	mtlStringMap<Function>               m_funcs;
	mtlList<File>                        m_files;
	mtlList<File*>                       m_file_stack;
	Scope                               *m_top;
	mtlList<Message>                     m_errors;
	int                                  m_stack_pointer;
	int                                  m_has_main;
	int                                  m_deferred_stack_manip;

private:
	void AddError(const mtlChars &err, const mtlChars &msg);
	File *AddFile(const mtlChars &filename);
	void PopFileStack( void );
	void GenerateExpressionTree(ExprNode *&node, const mtlChars &expr);
	TypeInfo ClassifyType(const mtlChars &type_name) const;
	bool IsKeyword(const mtlChars &str) const;
	bool IsValidName(const mtlChars &name) const;
	bool NameConflict(const mtlChars &sym_name) const;
	SymbolType *GetSymbol(const mtlChars &sym_name);
	void DeclareSymbol(const mtlChars &sym_name, SymbolType type);
	void DeclareVariable(const mtlChars &var_decl);
	void DeclareParameters(const mtlList<mtlChars> &param_list);
	void SplitParameters(const mtlChars &param_decl, mtlList<mtlChars> &out_param);
	void ClassifyParameters(const mtlChars &param_decl, mtlList<TypeInfo> &out_param);
	TypeInfo ClassifyParameter(const mtlChars &param_decl);
	Function *GetFunction(const mtlChars &func_name);
	Function *DeclareFunction(const mtlChars &ret_type, const mtlChars &func_name, const mtlChars &params);
	void DefineFunction(const mtlChars &ret_type, const mtlChars &func_name, const mtlChars &params, const mtlChars &body);
	void CompareFunctionSignature(const mtlChars &ret_type, const mtlChars &func_name, const mtlChars &params);
	void LoadFile(const mtlChars &filename, mtlString &file_contents);
	void CompileCodeUnit(mtlSyntaxParser &parser);
	void CompileLocalCodeUnit(mtlSyntaxParser &parser);
	void CompileGlobalCodeUnit(mtlSyntaxParser &parser);
	void CompileStatement(const mtlChars &scope);
	void CompileCode(const mtlChars &code);
	void CompileScope(const mtlChars &scope);
	void CompileCondition(const mtlChars &condition);
	void CompileIfElse(const mtlChars &condition, const mtlChars &if_code, const mtlChars &else_code);
	void CompileIf(const mtlChars &condition, const mtlChars &code);
	void CompileFunction(const mtlChars &ret_type, const mtlChars &func_name, const mtlChars &params, const mtlChars &body);
	void CompileFile(const mtlChars &filename);
	void CompileDeclarationAndExpression(const mtlChars &decl, const mtlChars &expr);
	void InitializeCompilerState(swsl::Shader &output);
	void DeclareIntrinsics( void );
	bool Success( void ) const;
	void PushScope( void );
	void PopScope( void );
	void ModifyStack(int stack_add);
	void EmitInstruction(swsl::InstructionSet instruction);
	void EmitAddress(swsl::addr_t address);
	void EmitImmutable(float immutable);
	void EmitStackMod( void );
	bool IsGlobalScope( void ) const;
	void ProgramErrorCheck( void );
	void ConvertToShader(swsl::Shader &output) const;

public:
	const mtlItem<Message> *GetError( void ) const;
	bool Compile(const mtlChars &filename, swsl::Shader &output);
};

#endif // COMPILER_H