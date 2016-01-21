#include <iostream>
#include "swsl_compiler.h"
#include "MiniLib/MTL/mtlParser.h"
#include "MiniLib/MTL/mtlList.h"
#include "MiniLib/MTL/mtlMathParser.h"
#include "MiniLib/MML/mmlMath.h"

int swsl::Compiler::OperationNode::Evaluate(mtlChars dst, mtlString &out, int depth)
{
	int ldepth = left->Evaluate(dst, out, depth);
	int rdepth = right->IsLeaf() ? 0 : right->Evaluate(dst, out, depth+1);
	int mdepth = 0;

	mtlString addr_str;
	if (depth > 0 || dst.GetSize() == 0) {
		out.Append('[');
		addr_str.FromInt(depth);
		out.Append(addr_str);
		out.Append(']');
		mdepth = depth;
	} else {
		out.Append(dst);
	}

	AppendValue(out);
	out.Append('=');

	if (right->IsLeaf()) {
		right->AppendValue(out);
	} else {
		addr_str.FromInt(depth+1);
		out.Append('[');
		out.Append(addr_str);
		out.Append(']');
	}
	out.Append(';');

	return mmlMax3(ldepth, mdepth, rdepth);
}

void swsl::Compiler::OperationNode::AppendValue(mtlString &out)
{
	out.Append(operation);
}

int swsl::Compiler::ValueNode::Evaluate(mtlChars dst, mtlString &out, int depth)
{
	int out_depth = 0;
	if (depth > 0 || dst.GetSize() == 0) {
		out.Append('[');
		mtlString addr_str;
		addr_str.FromInt(depth);
		out.Append(addr_str);
		out.Append(']');
		out_depth = depth;
	} else {
		out.Append(dst);
		out_depth = 0;
	}
	out.Append("=");
	AppendValue(out);
	out.Append(';');

	return out_depth;
}

void swsl::Compiler::ValueNode::AppendValue(mtlString &out)
{
	out.Append(term);
}

bool swsl::Compiler::ValueNode::IsConstant( void ) const
{
	return is_constant;
}

bool swsl::Compiler::SWSLOutput::IsBraceBalanced(mtlChars expr) const
{
	int stack = 0;
	for (int i = 0; i < expr.GetSize(); ++i) {
		char ch = expr.GetChars()[i];
		if (ch == '(') {
			++stack;
		} else if (ch == ')') {
			if (stack > 0) {
				--stack;
			} else {
				return false;
			}
		}
	}
	return stack == 0;
}

int swsl::Compiler::SWSLOutput::FindOperation(const mtlChars &operations, const mtlChars &expression) const
{
	int braceStack = 0;
	for (int i = 0; i < expression.GetSize(); ++i) {
		char ch = expression.GetChars()[i];
		if (ch == '(') {
			++braceStack;
		} else if (ch == ')') {
			--braceStack;
		} else if (braceStack == 0 && operations.SameAsAny(ch)) { // contents of parenthesis are not parsed
			return i;
		}
	}
	return -1;
}

swsl::Compiler::ExpressionNode *swsl::Compiler::SWSLOutput::GenerateTree(mtlChars expr, int lane)
{
	ExpressionNode *tree = NULL;
	if (!GenerateTree(tree, expr, lane, 0)) {
		if (tree != NULL) {
			delete tree;
			tree = NULL;
		}
	}
	return tree;
}

bool swsl::Compiler::SWSLOutput::GenerateTree(ExpressionNode *&node, mtlChars expr, int lane, int depth)
{
	static const char zero_str[] = "0";
	static const int OperationClasses = 2;
	static const char *Operations[OperationClasses] = {
		"+-", "*/" //,"|&"
	};

	expr.TrimBraces();

	if (expr.GetSize() == 0) {
		node = NULL;
		return false;
	}

	bool retval = true;
	int opIndex = -1;

	for (int i = 0; i < OperationClasses; ++i) {
		mtlChars ops = mtlChars::FromDynamic(Operations[i]);
		opIndex = FindOperation(ops, expr);
		if (opIndex != -1) {
			break;
		}
	}

	if (opIndex != -1) {

		OperationNode *op_node = new OperationNode;
		op_node->operation = expr[opIndex];
		op_node->left = NULL;
		op_node->right = NULL;

		mtlChars lexpr = mtlChars(expr, 0, opIndex);
		mtlChars rexpr = mtlChars(expr, opIndex + 1, expr.GetSize());

		if (expr[opIndex] == '-' && lexpr.GetSize() == 0) {
			lexpr = zero_str;
		}

		retval = GenerateTree(op_node->left, lexpr, lane, depth) && GenerateTree(op_node->right, rexpr, lane, depth + 1);

		node = op_node;
	} else { // STOPPING CONDITION

		ValueNode *val_node = new ValueNode;
		Type *type = GetType(expr);

		if (type != NULL && type->mutability == Constant) {
			val_node->size = type->size;
			val_node->term.FromFloat(type->size > 1 ? type->out.value[lane] : type->out.value[0]);
			val_node->is_constant = true;
		} else {
			mtlParser p(expr);
			mtlList<mtlChars> match;
			if (p.MatchAll("{%s}", match)) {
				mtlChars c = match.GetFirst()->GetItem();
				c.SplitByChar(match, ',');
				val_node->size = match.GetSize();
				if (lane >= match.GetSize() || match.GetSize() <= 0 || match.GetSize() > 4) {
					retval = false;
				} else {
					mtlItem<mtlChars> *iter = match.GetFirst();
					for (int i = 0; i < lane; ++i) {
						iter = iter->GetNext();
					}
					val_node->term.Copy(iter->GetItem());
				}
			} else {
				val_node->size = 1;
				val_node->term.Copy(expr);
			}
			val_node->is_constant = val_node->term.IsFloat();
		}

		node = val_node;
	}
	return retval;
}

void swsl::Compiler::SWSLOutput::SimplifyTree(ExpressionNode *node) const
{
	if (node == NULL) { return; }
}

bool swsl::Compiler::SWSLOutput::ParseExpression(mtlChars dst, mtlChars expr, int lane, mtlString &out, int &out_depth)
{
	if (!IsBraceBalanced(expr)) { return false; }
	ExpressionNode *tree = GenerateTree(expr, lane);
	if (tree == NULL) { return false; }
	SimplifyTree(tree); // combine constant terms
	out_depth = tree->Evaluate(dst, out, 0);
	std::cout << out_depth << " : " << out.GetChars() << std::endl;
	delete tree;
	return true;
}

mtlItem<mtlString> *swsl::Compiler::SWSLOutput::EmitPartialOperation(mtlChars op)
{
	if (op.Compare("+")) {
		Emit("add_");
		return m_code.GetLast();
	} else if (op.Compare("-")) {
		Emit("sub_");
		return m_code.GetLast();
	} else if (op.Compare("*")) {
		Emit("mul_");
		return m_code.GetLast();
	} else if (op.Compare("/")) {
		Emit("div_");
		return m_code.GetLast();
	}
	return NULL;
}

void swsl::Compiler::SWSLOutput::EmitOperand(mtlChars operand, int lane, mtlItem<mtlString> *op_loc)
{
	Type *type = GetType(operand);
	for (int i = 0; i < operand.GetSize(); ++i) {
		std::cout << operand[i];
	}
	std::cout << std::endl;
	if (type == NULL) {
		if (operand.IsFloat()) {
			Emit(operand);
			op_loc->GetItem().Append('c');
		} else {
			mtlParser p(operand);
			mtlList<mtlChars> m;
			if (p.MatchAll("[%i]", m) == mtlParser::ExpressionFound) {
				int stack_offset;
				m.GetFirst()->GetItem().ToInt(stack_offset);
				Emit(m_stack_size + stack_offset - 1);
				op_loc->GetItem().Append('f');
			} else {
				Emit(0.0f);
				op_loc->GetItem().Append('c'); // HACK: will cause error (I should really emit error here)
			}
		}
	} else {
		if (type->size <= 1) {
			Emit(type->out.address);
		} else if (lane < type->size) {
			Emit(type->out.address + lane);
		} else {
			// AddError("Type mismatch", operand);
		}
		op_loc->GetItem().Append('f');
	}
}

swsl::Compiler::SWSLOutput::SWSLOutput( void ) : m_stack_size(0)
{
}

void swsl::Compiler::SWSLOutput::PushScope(mtlChars scope_code)
{
	m_scopes.AddLast();
	m_scopes.GetLast()->GetItem().parser.SetBuffer(scope_code);
	m_scopes.GetLast()->GetItem().stack_size = 0;
	for (int i = 0; i < scope_code.GetSize(); ++i) {
		std::cout << scope_code[i];
	}
	std::cout << std::endl;
}

void swsl::Compiler::SWSLOutput::PopScope( void )
{
	Emit("pop");
	Emit((swsl::instr_t)m_scopes.GetLast()->GetItem().stack_size);
	m_stack_size -= m_scopes.GetLast()->GetItem().stack_size;
	m_scopes.RemoveLast();
}

int swsl::Compiler::SWSLOutput::GetScopeDepth( void ) const
{
	return m_scopes.GetSize();
}

void swsl::Compiler::SWSLOutput::PushStack(int size)
{
	Emit("push");
	Emit((swsl::instr_t)size);
	m_stack_size += size;
	m_scopes.GetLast()->GetItem().stack_size += size;
}

void swsl::Compiler::SWSLOutput::PopStack(int size)
{
	Emit("pop");
	Emit((swsl::instr_t)size);
	m_stack_size -= size;
	m_scopes.GetLast()->GetItem().stack_size -= size;
}

swsl::Compiler::ValueType swsl::Compiler::SWSLOutput::Typeof(mtlChars decl_type) const
{
	if (decl_type.Compare("void")) {
		return Void;
	} else if (decl_type.Compare("bool")) {
		return Bool;
	} else if (decl_type.Compare("float")) {
		return Float;
	} else if (decl_type.Compare("float2")) {
		return Float;
	} else if (decl_type.Compare("float3")) {
		return Float;
	} else if (decl_type.Compare("float4")) {
		return Float;
	}
	return None;
}

swsl::instr_t swsl::Compiler::SWSLOutput::Sizeof(mtlChars decl_type) const
{
	if (decl_type.Compare("void")) {
		return 0;
	} else if (decl_type.Compare("bool")) {
		return 1;
	} else if (decl_type.Compare("float")) {
		return 1;
	} else if (decl_type.Compare("float2")) {
		return 2;
	} else if (decl_type.Compare("float3")) {
		return 3;
	} else if (decl_type.Compare("float4")) {
		return 4;
	}
	return 0;
}

swsl::Compiler::Type *swsl::Compiler::SWSLOutput::Declare(mtlChars type, mtlChars name, MutabilityType mut)
{
	Type t;
	t.name = name;
	t.out.address = m_stack_size;
	t.mutability = mut;
	t.scope_depth = GetScopeDepth();
	t.decl_type = type;
	t.base_type = Typeof(type);
	t.size = Sizeof(type);
	if (t.size == 0) { return NULL; }

	PushStack(t.size);

	if (name.GetSize() > 0 && (mtlChars::IsAlpha(name.GetChars()[0]) || name.GetChars()[0] == '_')) {
		for (int i = 1; i < name.GetSize(); ++i) {
			if (!mtlChars::IsAlphanumeric(name.GetChars()[i]) && name.GetChars()[i] != '_') {
				return NULL;
			}
		}
	} else {
		return NULL;
	}

	if (GetType(name) != NULL) {
		return NULL;
	}

	Type *t_ptr = m_scopes.GetLast()->GetItem().types.CreateEntry(name);
	*t_ptr = t;

	return t_ptr;
}

swsl::Compiler::Type *swsl::Compiler::SWSLOutput::GetType(mtlChars name)
{
	mtlItem<Scope> *scope = m_scopes.GetLast();
	Type *type = NULL;
	while (scope != NULL && type == NULL) {
		type = scope->GetItem().types.GetEntry(name);
		scope = scope->GetPrev();
	}
	return type;
}

void swsl::Compiler::SWSLOutput::Emit(mtlChars instr, mtlItem<mtlString> *insert_loc)
{
	m_code.Insert(insert_loc)->GetItem().Copy(instr);
}

void swsl::Compiler::SWSLOutput::Emit(swsl::instr_t instr, mtlItem<mtlString> *insert_loc)
{
	m_code.Insert(insert_loc)->GetItem().FromInt(instr);
}

void swsl::Compiler::SWSLOutput::Emit(float value, mtlItem<mtlString> *insert_loc)
{
	m_code.Insert(insert_loc)->GetItem().FromFloat(value);
}

bool swsl::Compiler::SWSLOutput::EmitExpression(int lane, mtlChars expression, swsl::instr_t depth)
{
	if (depth > 0) {
		Emit("push");
		Emit(depth);
	}

	mtlList<mtlChars> operations;
	expression.SplitByChar(operations, ';');
	mtlItem<mtlChars> *operation = operations.GetFirst();

	while (operation != NULL) {
		if (operation->GetItem().GetSize() == 0) { break; }

		mtlParser op_parser(operation->GetItem());
		mtlList<mtlChars> match;

		if (op_parser.MatchAll("%s+=%s", match) == mtlParser::ExpressionFound) {
			Emit("add_");
		} else if (op_parser.MatchAll("%s-=%s", match) == mtlParser::ExpressionFound) {
			Emit("sub_");
		} else if (op_parser.MatchAll("%s*=%s", match) == mtlParser::ExpressionFound) {
			Emit("mul_");
		} else if (op_parser.MatchAll("%s/=%s", match) == mtlParser::ExpressionFound) {
			Emit("div_");
		} else if (op_parser.MatchAll("%s=%s", match) == mtlParser::ExpressionFound) {
			Emit("mov_");
		}

		mtlItem<mtlString> *op_loc = m_code.GetLast();
		mtlChars dst = match.GetFirst()->GetItem();
		mtlChars src = match.GetFirst()->GetNext()->GetItem();

		EmitOperand(dst, lane, op_loc);
		EmitOperand(src, lane, op_loc);

		operation = operation->GetNext();
	}

	if (depth > 0) {
		Emit("pop");
		Emit(depth);
	}

	return true;
}

void swsl::Compiler::SWSLOutput::EmitInput(mtlChars params)
{
	Emit("input");
	mtlList<mtlChars> p;
	params.SplitByChar(p, ',');
	mtlItem<mtlChars> *param = p.GetFirst();
	swsl::instr_t input_count = 0;
	while (param != NULL) {
		mtlList<mtlChars> match;
		mtlParser parser(param->GetItem());
		if (parser.Match("%w", match) == mtlParser::ExpressionFound) {
			input_count += Sizeof(match.GetFirst()->GetItem());
		}
		param = param->GetNext();
	}
	Emit(input_count);
}

bool swsl::Compiler::SWSLOutput::Evaluate(mtlChars str)
{
	mtlList<mtlChars> match;
	str.SplitByChar(match, '=');
	mtlChars dst_str;
	mtlChars expr_str;
	mtlChars dst_type;
	mtlChars dst_name;
	if (match.GetSize() <= 2) {
		dst_str = match.GetFirst()->GetItem();
	} else {
		return false;
	}
	if (match.GetSize() == 2) {
		expr_str = match.GetFirst()->GetNext()->GetItem();
	}

	mtlParser parser(dst_str);
	Type *dst = NULL;
	bool readonly_decl = false;
	if (parser.MatchAll("const %w %w", match) == mtlParser::ExpressionFound) {
		dst_type = match.GetFirst()->GetItem();
		dst_name = match.GetFirst()->GetNext()->GetItem();
		dst = Declare(dst_type, dst_name, Readonly);
		readonly_decl = true;
	} else if (parser.MatchAll("%w %w", match) == mtlParser::ExpressionFound) {
		dst_type = match.GetFirst()->GetItem();
		dst_name = match.GetFirst()->GetNext()->GetItem();
		dst = Declare(dst_type, dst_name, Variable);
	} else if (parser.MatchAll("%w", match) == mtlParser::ExpressionFound) {
		dst_name = match.GetFirst()->GetItem();
		dst = GetType(dst_name);
		if (dst != NULL && dst->mutability != Variable) { return false; }
	}

	if (dst == NULL) { return false; }

	if (expr_str.GetSize() > 0) {
		for (int i = 0; i < dst->size; ++i) {
			mtlString order;
			int depth;
			if (!ParseExpression(dst_name, expr_str, i, order, depth)) {
				return false;
			}

			// IF readonly_decl AND expression = Constant
			// dst->mutability = Constant;
			// dst->out.value = expression.result;

			EmitExpression(i, order, depth);
		}
	}

	return true;
}

swsl::instr_t swsl::Compiler::SWSLOutput::GetStackPtr( void ) const
{
	return m_stack_size;
}

bool swsl::Compiler::SWSLOutput::Match(mtlChars expr)
{
	return m_scopes.GetLast()->GetItem().parser.Match(expr, m_matches) == mtlParser::ExpressionFound;
}

mtlChars swsl::Compiler::SWSLOutput::GetMatch(int i) const
{
	const mtlItem<mtlChars> *iter = m_matches.GetFirst();
	for (int j = 0; j < i; ++j) {
		if (iter != NULL) {
			iter = iter->GetNext();
		} else {
			return mtlChars();
		}
	}
	return iter->GetItem();
}

bool swsl::Compiler::SWSLOutput::IsEnd( void )
{
	return m_scopes.GetLast()->GetItem().parser.IsEnd();
}

void swsl::Compiler::SWSLOutput::CopyOutputTo(mtlArray<char> &out) const
{
	const mtlItem<mtlString> *instr = m_code.GetFirst();
	int len = 0;
	while (instr != NULL) {
		len += instr->GetItem().GetSize() + 1;
		instr = instr->GetNext();
	}
	out.Create(len);
	instr = m_code.GetFirst();
	int j = 0;
	while (instr != NULL) {
		for (int i = 0; i < instr->GetItem().GetSize(); ++i) {
			out[j++] = instr->GetItem().GetChars()[i];
		}
		out[j++] = ' ';
		instr = instr->GetNext();
	}
}

void swsl::Compiler::SWSLOutput::Optimize( void )
{
	// For now;
	// Only finds adjacent push/pop instructions and merges them together

	mtlItem<mtlString> *instr = m_code.GetFirst();
	mtlItem<mtlString> *ins = NULL;
	int counter = 0;
	int type = 0;
	while (instr != NULL) {
		if (instr->GetItem().Compare("push")) {
			if (type < 0) {
				ins->GetItem().FromInt(counter);
				counter = 0;
			}
			type = 1;
			if (ins == NULL) {
				instr = instr->GetNext();
				if (instr->GetItem().ToInt(counter)) {
					ins = instr;
				}
				instr = instr->GetNext();
			} else {
				int c;
				if (instr->GetNext()->GetItem().ToInt(c)) {
					counter += c;
				}
				instr = instr->Remove()->Remove();
			}
		} else if (instr->GetItem().Compare("pop")) {
			if (type > 0) {
				ins->GetItem().FromInt(counter);
				counter = 0;
			}
			type = -1;
			if (ins == NULL) {
				instr = instr->GetNext();
				if (instr->GetItem().ToInt(counter)) {
					ins = instr;
				}
				instr = instr->GetNext();
			} else {
				int c;
				if (instr->GetNext()->GetItem().ToInt(c)) {
					counter += c;
				}
				instr = instr->Remove()->Remove();
			}
		} else {
			if (ins != NULL) {
				ins->GetItem().FromInt(counter);
			}
			ins = NULL;
			counter = 0;
			type = 0;
			instr = instr->GetNext();
		}
	}
}

void swsl::Compiler::AddError(mtlChars message, mtlChars instr)
{
	m_errors.AddLast();
	m_errors.GetLast()->GetItem().Copy(message);
	m_errors.GetLast()->GetItem().Append(": ").Append(instr);
}

void swsl::Compiler::CompileSWSLScope(mtlChars scope_code, mtlChars parameters, const bool branch, SWSLOutput &out)
{
	out.PushScope(scope_code);
	if (parameters.GetSize() != 0) {
		mtlList<mtlChars> params;
		parameters.SplitByChar(params, ',');
		mtlItem<mtlChars> *i = params.GetFirst();
		while (i != NULL) {
			mtlParser parser(i->GetItem());
			mtlList<mtlChars> match;
			if (parser.MatchAll("%w %w", match) == mtlParser::ExpressionFound) {
				if (!out.Evaluate(i->GetItem())) {
					AddError("Declaration", i->GetItem());
				}
			} else {
				AddError("Syntax", i->GetItem());
			}
			i = i->GetNext();
		}
	}

	// Conditionals require:
	// A stack copy of an any variable
		// 1) declared outside of the conditional scope, and
		// 2) is modified inside the conditional scope
	// Store the test as a mask on the top of the stack
	// Merge instructions between the modified stack copy and the original variables at the end of the scope
	mtlItem<mtlString> *conditional = NULL;

	while (!out.IsEnd()) {

		if (out.Match("{%s}")) {
			mtlChars scope_code = out.GetMatch(0);
			CompileSWSLScope(scope_code, "", false, out);
		}

		/*else if (out.Match("if(%s){%s}")) {
			// compile expression and test
			// push branching values
			// push comparison mask
			mtlChars scope_code = out.GetMatch(1);
			CompileSWSLScope(scope_code, "", true, out);
			// Supporting else ifs might require more than one copy of branching values...
			// Maybe force programmer to do if inside else-block
			//while (out.Match("else if(%s){%s}")) {
			//	// compile expression and test
			//	scope_code = out.GetMatch(1);
			//	CompileSWSLScope(scope_code, true, "", out);
			//}
			if (out.Match("else{%s}")) {
				// compile expression and test
				scope_code = out.GetMatch(0);
				CompileSWSLScope(scope_code, false, "", out);
			}
			// merge values
			// pop comparison mask
			// pop branching values
		}*/

		else if (out.Match("%s;")) {
			out.Evaluate(out.GetMatch(0));
		}

		else {
			AddError("Statement", "N/A");
			return;
		}
	}

	out.PopScope();
}

bool swsl::Compiler::CompileSWSL(mtlArray<char> &out)
{
	mtlString file_contents;
	file_contents.Copy(mtlChars::FromDynamic(out, out.GetSize()));
	out.Free();

	SWSLOutput out_asm;
	out_asm.PushScope(file_contents);

	// In this function we iterate over global scope (functions, global variables (textures etc.))

	while (!out_asm.IsEnd()) {
		if (out_asm.Match("%w %w(%s){%s}")) {
			mtlChars func_ret_type   = out_asm.GetMatch(0);
			mtlChars func_name       = out_asm.GetMatch(1);
			mtlChars func_params     = out_asm.GetMatch(2);
			mtlChars func_scope_code = out_asm.GetMatch(3);
			if (func_name.Compare("main")) {
				if (!func_ret_type.Compare("void")) {
					AddError("Main can not have return value", "N/A");
				}
				out_asm.EmitInput(func_params);
			}
			CompileSWSLScope(func_scope_code, func_params, false, out_asm);
		}

		else if (out_asm.Match("struct %w{%s};")) {

		}

		else {
			AddError("Global", "N/A");
		}
	}

	if (m_errors.GetSize() == 0) {
		out_asm.Emit("end");
		out_asm.Optimize();
		out_asm.CopyOutputTo(out);
	}

	return m_errors.GetSize() == 0;
}

/*bool swsl::Compiler::CompileSWSL(mtlArray<char> &out)
{
	// TODO
	// Conditionals
	// Expressions
	// float2, float3, float4 element access
	// Math functions (dot, cross, sqrt, etc.)
	// Texture sampling
	// Comments
	// Functions
	// Structs
	// Includes
	// Defines

	mtlString file_contents;
	file_contents.Copy(mtlChars::FromDynamic(out, out.GetSize()));

	mtlList<mtlString> asm_program;
	mtlList<Scope> scopes;
	PushScope(scopes, file_contents);
	mtlList<mtlChars> parse_match;

	int char_count = 0;

	while (scopes.GetSize() > 0 && !TopParser(scopes).IsEnd()) {

		if (IsGlobalScope(scopes)) {

			if (TopParser(scopes).Match("void main(%s){%s}", parse_match)) { // allow for any function declaration later

				PushScope(scopes, parse_match.GetFirst()->GetNext()->GetItem());

				mtlList<mtlChars> params;
				parse_match.GetFirst()->GetItem().SplitByChar(params, ',');
				mtlItem<mtlChars> *param = params.GetFirst();
				while (param != NULL) {
					AddFunctionParameter(scopes, param->GetItem());
					param = param->GetNext();
				}

				AddInstruction("input", asm_program, char_count);
				AddInstruction(TopScope(scopes).stack_size, asm_program, char_count);

			} else {
				AddError("main not found", TopParser(scopes).GetBuffer());
			}

		} else {
			if (TopParser(scopes).Match("%w=%s;", parse_match)) {
				mtlChars dst_name = parse_match.GetFirst()->GetItem();
				mtlChars src_name = parse_match.GetFirst()->GetNext()->GetItem(); // This will fail if src is anything but a single unary term
				Type *dst = GetDataType(scopes, dst_name);
				if (dst != NULL) {
					Type *src = GetDataType(scopes, src_name);
					if (src != NULL) {
						if (dst->type == src->type && dst->type >= Float) {
							for (int i = 0; i < dst->type; ++i) {
								AddInstruction("mov_ff", asm_program, char_count);
								AddInstruction(dst->address + i, asm_program, char_count);
								AddInstruction(src->address + i, asm_program, char_count);
							}
						} else {
							AddError("Type mismatch", TopParser(scopes).GetBuffer());
						}
					} else {
						float fl;
						if (src_name.ToFloat(fl)) {
							if (dst->type >= Float) {
								mtlString fl_str;
								fl_str.FromFloat(fl);
								for (int i = 0; i < dst->type; ++i) {
									AddInstruction("mov_fc", asm_program, char_count);
									AddInstruction(dst->address + i, asm_program, char_count);
									AddInstruction(fl, asm_program, char_count);
								}
							}
						}
					}

				} else {
					AddError("Expression", dst_name);
				}
			} else if (TopParser(scopes).Match("if(%s){%s}", parse_match)) {

			} else {
				AddError("Expression", TopParser(scopes).GetBuffer());
			}
		}

		while (scopes.GetSize() > 0 && TopParser(scopes).IsEnd()) {
			scopes.RemoveLast();
		}
	}

	AddInstruction("end", asm_program, char_count);

	// struct:      "struct %w{%s};"
	// function:    "%w %w(%s){%s}"
	// conditional: "if(%s){%s}" | "if(%s)" (unsure if I should support no-braces)
	// declaration: "%w %w={%s};" | "%w %w=%s;" | "%w %w;"

	out.Create(char_count);
	mtlItem<mtlString> *instruction = asm_program.GetFirst();
	int i = 0;
	while (instruction != NULL) {
		for (int n = 0; n < instruction->GetItem().GetSize(); ++n, ++i) {
			out[i] = instruction->GetItem().GetChars()[n];
		}
		out[i++] = ' ';
		instruction = instruction->GetNext();
	}

	return m_errors.GetSize() == 0;
}*/

swsl::instr_t swsl::Compiler::ReadFloat(mtlParser &parser)
{
	mtlList<mtlChars> out;
	float to_float = 0.0f;
	if (parser.Match("%(%d.f)", out, false) == mtlParser::ExpressionFound && out.GetSize() == 1 && out.GetFirst()->GetItem().ToFloat(to_float)) {
		return ToIntBits(to_float);
	}
	AddError("Invalid constant [float]", parser.GetBuffer());
	return ToIntBits(0.0f);
}

swsl::instr_t swsl::Compiler::ReadInt(mtlParser &parser)
{
	mtlChars num = parser.ReadWord();
	swsl::instr_t to_uint = 0;
	if (!num.ToInt(*(int*)&to_uint)) {
		AddError("Invalid constant [uint]", num);
		return 0;
	}
	return to_uint;
}

bool swsl::Compiler::CompileASM(mtlArray<char> &out)
{
	mtlString file_contents;
	file_contents.Copy(mtlChars::FromDynamic(out, out.GetSize()));

	mtlParser parser;
	parser.SetBuffer(file_contents);

	mtlList<instr_t> bytecode;

	mtlChars instr = parser.ReadWord();
	if (!instr.Compare("input")) {
		AddError("First instruction is not [input]", instr);
	}
	bytecode.AddLast(ReadInt(parser));

	while (!parser.IsEnd()) {
		instr = parser.ReadWord();
		if (instr.Compare("nop")) {
			bytecode.AddLast(swsl::NOP);
		} else if (instr.Compare("end")) {
			bytecode.AddLast(swsl::END);
		} else if (instr.Compare("jmp")) {
			bytecode.AddLast(swsl::JMP);
			bytecode.AddLast(ReadInt(parser));
		} else if (instr.Compare("pop")) {
			bytecode.AddLast(swsl::POP);
			bytecode.AddLast(ReadInt(parser));
		} else if (instr.Compare("push")) {
			bytecode.AddLast(swsl::PUSH);
			bytecode.AddLast(ReadInt(parser));
		} else if (instr.Compare("mov_ff")) {
			bytecode.AddLast(swsl::MOV_FF);
			bytecode.AddLast(ReadInt(parser));
			bytecode.AddLast(ReadInt(parser));
		} else if (instr.Compare("mov_fc")) {
			bytecode.AddLast(swsl::MOV_FC);
			bytecode.AddLast(ReadInt(parser));
			bytecode.AddLast(ReadFloat(parser));
		} else if (instr.Compare("add_ff")) {
			bytecode.AddLast(swsl::ADD_FF);
			bytecode.AddLast(ReadInt(parser));
			bytecode.AddLast(ReadInt(parser));
		} else if (instr.Compare("add_fc")) {
			bytecode.AddLast(swsl::ADD_FC);
			bytecode.AddLast(ReadInt(parser));
			bytecode.AddLast(ReadFloat(parser));
		} else if (instr.Compare("sub_ff")) {
			bytecode.AddLast(swsl::SUB_FF);
			bytecode.AddLast(ReadInt(parser));
			bytecode.AddLast(ReadInt(parser));
		} else if (instr.Compare("sub_fc")) {
			bytecode.AddLast(swsl::SUB_FC);
			bytecode.AddLast(ReadInt(parser));
			bytecode.AddLast(ReadFloat(parser));
		} else if (instr.Compare("mul_ff")) {
			bytecode.AddLast(swsl::MUL_FF);
			bytecode.AddLast(ReadInt(parser));
			bytecode.AddLast(ReadInt(parser));
		} else if (instr.Compare("mul_fc")) {
			bytecode.AddLast(swsl::MUL_FC);
			bytecode.AddLast(ReadInt(parser));
			bytecode.AddLast(ReadFloat(parser));
		} else if (instr.Compare("div_ff")) {
			bytecode.AddLast(swsl::DIV_FF);
			bytecode.AddLast(ReadInt(parser));
			bytecode.AddLast(ReadInt(parser));
		} else if (instr.Compare("div_fc")) {
			bytecode.AddLast(swsl::DIV_FC);
			bytecode.AddLast(ReadInt(parser));
			bytecode.AddLast(ReadFloat(parser));
		} else if (instr.Compare("min_ff")) {
			bytecode.AddLast(swsl::MIN_FF);
			bytecode.AddLast(ReadInt(parser));
			bytecode.AddLast(ReadInt(parser));
		} else if (instr.Compare("min_fc")) {
			bytecode.AddLast(swsl::MIN_FC);
			bytecode.AddLast(ReadInt(parser));
			bytecode.AddLast(ReadFloat(parser));
		} else if (instr.Compare("max_ff")) {
			bytecode.AddLast(swsl::MAX_FF);
			bytecode.AddLast(ReadInt(parser));
			bytecode.AddLast(ReadInt(parser));
		} else if (instr.Compare("max_fc")) {
			bytecode.AddLast(swsl::MAX_FC);
			bytecode.AddLast(ReadInt(parser));
			bytecode.AddLast(ReadFloat(parser));
		} else if (instr.Compare("sqrt_ff")) {
			bytecode.AddLast(swsl::SQRT_FF);
			bytecode.AddLast(ReadInt(parser));
			bytecode.AddLast(ReadInt(parser));
		} else if (instr.Compare("sqrt_fc")) {
			bytecode.AddLast(swsl::SQRT_FC);
			bytecode.AddLast(ReadInt(parser));
			bytecode.AddLast(ReadFloat(parser));
		} else if (instr.Compare("eq_ff")) {
			bytecode.AddLast(swsl::EQ_FF);
			bytecode.AddLast(ReadInt(parser));
			bytecode.AddLast(ReadInt(parser));
		} else if (instr.Compare("eq_fc")) {
			bytecode.AddLast(swsl::EQ_FC);
			bytecode.AddLast(ReadInt(parser));
			bytecode.AddLast(ReadFloat(parser));
		} else if (instr.Compare("eq_cf")) {
			bytecode.AddLast(swsl::EQ_CF);
			bytecode.AddLast(ReadFloat(parser));
			bytecode.AddLast(ReadInt(parser));
		} else if (instr.Compare("neq_ff")) {
			bytecode.AddLast(swsl::NEQ_FF);
			bytecode.AddLast(ReadInt(parser));
			bytecode.AddLast(ReadInt(parser));
		} else if (instr.Compare("neq_fc")) {
			bytecode.AddLast(swsl::NEQ_FC);
			bytecode.AddLast(ReadInt(parser));
			bytecode.AddLast(ReadFloat(parser));
		} else if (instr.Compare("neq_cf")) {
			bytecode.AddLast(swsl::NEQ_CF);
			bytecode.AddLast(ReadFloat(parser));
			bytecode.AddLast(ReadInt(parser));
		} else if (instr.Compare("gt_ff")) {
			bytecode.AddLast(swsl::GT_FF);
			bytecode.AddLast(ReadInt(parser));
			bytecode.AddLast(ReadInt(parser));
		} else if (instr.Compare("gt_fc")) {
			bytecode.AddLast(swsl::GT_FC);
			bytecode.AddLast(ReadInt(parser));
			bytecode.AddLast(ReadFloat(parser));
		} else if (instr.Compare("gt_cf")) {
			bytecode.AddLast(swsl::GT_CF);
			bytecode.AddLast(ReadFloat(parser));
			bytecode.AddLast(ReadInt(parser));
		} else if (instr.Compare("ge_ff")) {
			bytecode.AddLast(swsl::GE_FF);
			bytecode.AddLast(ReadInt(parser));
			bytecode.AddLast(ReadInt(parser));
		} else if (instr.Compare("ge_fc")) {
			bytecode.AddLast(swsl::GE_FC);
			bytecode.AddLast(ReadInt(parser));
			bytecode.AddLast(ReadFloat(parser));
		} else if (instr.Compare("ge_cf")) {
			bytecode.AddLast(swsl::GE_CF);
			bytecode.AddLast(ReadFloat(parser));
			bytecode.AddLast(ReadInt(parser));
		} else if (instr.Compare("lt_ff")) {
			bytecode.AddLast(swsl::LT_FF);
			bytecode.AddLast(ReadInt(parser));
			bytecode.AddLast(ReadInt(parser));
		} else if (instr.Compare("lt_fc")) {
			bytecode.AddLast(swsl::LT_FC);
			bytecode.AddLast(ReadInt(parser));
			bytecode.AddLast(ReadFloat(parser));
		} else if (instr.Compare("lt_cf")) {
			bytecode.AddLast(swsl::LT_CF);
			bytecode.AddLast(ReadFloat(parser));
			bytecode.AddLast(ReadInt(parser));
		} else if (instr.Compare("le_ff")) {
			bytecode.AddLast(swsl::LE_FF);
			bytecode.AddLast(ReadInt(parser));
			bytecode.AddLast(ReadInt(parser));
		} else if (instr.Compare("le_fc")) {
			bytecode.AddLast(swsl::LE_FC);
			bytecode.AddLast(ReadInt(parser));
			bytecode.AddLast(ReadFloat(parser));
		} else if (instr.Compare("le_cf")) {
			bytecode.AddLast(swsl::LE_CF);
			bytecode.AddLast(ReadFloat(parser));
			bytecode.AddLast(ReadInt(parser));
		} else if (instr.Compare("mrg")) {
			bytecode.AddLast(swsl::MRG);
			bytecode.AddLast(ReadInt(parser));
			bytecode.AddLast(ReadInt(parser));
		} else {
			AddError("Invalid opcode", instr);
		}
	}

	if (m_errors.GetSize() == 0) {
		out.Create(bytecode.GetSize() * sizeof(swsl::instr_t));
		mtlItem<instr_t> *i = bytecode.GetFirst();
		swsl::instr_t *data = (swsl::instr_t*)&out[0];
		int counter = 0;
		while (i != NULL) {
			data[counter++] = i->GetItem();
			i = i->GetNext();
		}
	}

	return m_errors.GetSize() == 0;
}

bool swsl::Compiler::CompileByteCode(mtlArray<char> &out)
{
	// Introduce actual native compilation here
	AddError("Compilation to native binary not supported yet", "0");
	return false;
}

bool swsl::Compiler::Compile(const mtlChars &file_contents, swsl::Compiler::LanguageFormat in_fmt, swsl::Compiler::LanguageFormat out_fmt, mtlArray<char> &out)
{
	out.Free();
	m_errors.RemoveAll();

	if (in_fmt >= out_fmt) {
		AddError("Reassembly not supported", "0");
	}

	out.Create(file_contents.GetSize());
	mtlCopy((char*)&out[0], file_contents.GetChars(), out.GetSize());

	switch (in_fmt) {
	case SWSL:
		if (!CompileSWSL(out)) { return false; }
		if (out_fmt == ASM) { break; }

	case ASM:
		if (!CompileASM(out)) { return false; }
		if (out_fmt == BYTE_CODE) { break; }

	case BYTE_CODE:
		if (!CompileByteCode(out)) { return false; }
		if (out_fmt == NATIVE) { break; }

	default:
		AddError("Unknown compilation type", "0");
		break;
	}

	return m_errors.GetSize() == 0;
}
