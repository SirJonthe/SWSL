#ifndef SWSL_COMPILER_H
#define SWSL_COMPILER_H

// take a look at Yacc

#include "swsl_instr.h"

#include "MiniLib/MTL/mtlString.h"
#include "MiniLib/MTL/mtlParser.h"
#include "MiniLib/MTL/mtlArray.h"
#include "MiniLib/MTL/mtlStringMap.h"
#include "MiniLib/MTL/mtlMathParser.h"

namespace swsl
{

// the compiler can compile swsl->byte code, eventually compile byte code->native code

class Compiler
{
public:
	enum LanguageFormat
	{
		SWSL,
		ASM,
		BYTE_CODE,
		NATIVE
	};

private:
	enum MutabilityType
	{
		Constant, // a true constant; can be substituted with its value
		Readonly, // a read-only constant; can not be substituted with its value
		Variable // a read-write variable
	};

	enum ValueType
	{
		None,
		Void,
		Bool,
		Float
		// Difficult to implement because I can't interpolate them in a register
		// Fixed,
		// Int
	};

	struct Type
	{
		mtlChars       name;
		ValueType      base_type;
		mtlChars       decl_type;
		MutabilityType mutability;
		swsl::instr_t  size;
		swsl::instr_t  scope_depth;
		union {
			float         value[16]; // only valid if const
			swsl::instr_t address; // only valid if not const
		} out;
	};

	struct Scope
	{
		mtlStringMap<Type> types;
		swsl::instr_t      stack_size;
		mtlParser          parser;
		//mtlList<Type> modified;
	};

	struct Errors
	{
		mtlList<mtlString> errors;
		void AddError(mtlChars message, mtlChars instr);
	};

	struct ExpressionNode
	{
		virtual ~ExpressionNode( void ) {}
		virtual int  Evaluate(mtlChars dst, mtlString &out, int depth) = 0;
		virtual bool IsLeaf( void ) const = 0;
		virtual void AppendValue(mtlString &out) = 0;
		virtual bool IsConstant( void ) const = 0;
	};

	struct OperationNode : public ExpressionNode
	{
		char            operation;
		ExpressionNode *left;
		ExpressionNode *right;

		~OperationNode( void ) { delete left; delete right; }
		int  Evaluate(mtlChars dst, mtlString &out, int depth);
		bool IsLeaf( void ) const { return false; }
		void AppendValue(mtlString &out);
		bool IsConstant( void ) const { return left->IsConstant() && right->IsConstant(); }
	};

	struct ValueNode : public ExpressionNode
	{
		mtlString     term;
		swsl::instr_t size;
		bool          is_constant;

		int  Evaluate(mtlChars dst, mtlString &out, int depth);
		bool IsLeaf( void ) const { return true; }
		void AppendValue(mtlString &out);
		bool IsConstant( void ) const;
	};

	class SWSLOutput
	{
	private:
		mtlList<Scope>     m_scopes;
		mtlList<mtlString> m_code;
		mtlList<mtlChars>  m_matches;
		swsl::instr_t      m_stack_size;

	private:
		bool IsBraceBalanced(mtlChars expr) const;
		int  FindOperation(const mtlChars &operation, const mtlChars &expression) const;
		ExpressionNode *GenerateTree(mtlChars expr, int lane);
		bool GenerateTree(ExpressionNode *&node, mtlChars expr, int lane, int depth);
		void SimplifyTree(ExpressionNode *node) const;
		bool ParseExpression(mtlChars dst, mtlChars expr, int lane, mtlString &out, int &out_depth);
		mtlItem<mtlString> *EmitPartialOperation(mtlChars op);
		void EmitOperand(mtlChars operand, int lane, mtlItem<mtlString> *op_loc);

	public:
		SWSLOutput( void );

		void PushScope(mtlChars scope_code);
		void PopScope( void );
		int  GetScopeDepth( void ) const;

		void           PushStack(int size);
		void           PopStack(int size);
		ValueType      Typeof(mtlChars decl_type) const;
		swsl::instr_t  Sizeof(mtlChars decl_type) const;
		Type          *Declare(mtlChars type, mtlChars name, MutabilityType mut);
		Type          *GetType(mtlChars name);

		void Emit(mtlChars instr, mtlItem<mtlString> *insert_loc = NULL);
		void Emit(swsl::instr_t instr, mtlItem<mtlString> *insert_loc = NULL);
		void Emit(float value, mtlItem<mtlString> *insert_loc = NULL);
		bool EmitExpression(int lane, mtlChars expression, swsl::instr_t depth);
		void EmitInput(mtlChars params);

		bool Evaluate(mtlChars str);

		swsl::instr_t GetStackPtr( void ) const;

		bool     Match(mtlChars expr);
		mtlChars GetMatch(int i) const;
		bool     IsEnd( void );

		void CopyOutputTo(mtlArray<char> &out) const;

		void Optimize( void );
	};

private:
	mtlList<mtlString> m_errors;

private:
	void AddError(mtlChars message, mtlChars instr);
	void CompileSWSLScope(mtlChars scope_code, mtlChars params, const bool branch, SWSLOutput &out);
	bool CompileSWSL(mtlArray<char> &out);

	swsl::instr_t ReadFloat(mtlParser &parser);
	swsl::instr_t ReadInt(mtlParser &parser);
	swsl::instr_t ToIntBits(float fl) const { return *(swsl::instr_t*)&fl; }
	bool CompileASM(mtlArray<char> &out);

	bool CompileByteCode(mtlArray<char> &out);

public:
	bool Compile(const mtlChars &file_contents, LanguageFormat in_fmt, LanguageFormat out_fmt, mtlArray<char> &out);

	const mtlItem<mtlString> *GetErrors( void ) const { return m_errors.GetFirst(); }
};

}

#endif // SWSL_COMPILER_H
