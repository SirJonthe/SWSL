#include "swsl_compiler.h"
#include "swsl_instr.h"

#include "swsl_wide.h"

#include "MiniLib/MTL/mtlArray.h"
#include "MiniLib/MTL/mtlMathParser.h"

#include <iostream>
void print_ch(const mtlChars &str)
{
	for (int i = 0; i < str.GetSize(); ++i) {
		std::cout << str.GetChars()[i];
	}
}

enum Type
{
	Void,
	Bool,
	Float,
	Float2,
	Float3,
	Float4,

	TYPE_COUNT
};

enum Mutability
{
	Immutable,
	Readonly,
	Mutable
};

union Value
{
	swsl::addr_t var_addr;
	float        imm_value;
};

struct TypeInfo
{
	mtlChars name;
	Type     type;
	int      size;
};

struct Definition
{
	mtlString  name;
	TypeInfo   type;
	Mutability mut;
	Value      value;
	int        scope_level;
};

struct Scope
{
	mtlList< Definition > defs;
	Parser                parser;
	int                   size;
	int                   rel_sptr;
	int                   nested_branch;
};

struct CompileInstance
{
	mtlList<swsl::Instruction>      program;
	swsl::addr_t                   *prog_entry;
	swsl::addr_t                   *prog_inputs;
	mtlList<swsl::CompilerMessage>  errors;
	mtlList<swsl::CompilerMessage>  warnings;
	mtlList<Scope>                  scopes;
	int                             stack_manip;
	int                             main;
};

static const TypeInfo gTypes[TYPE_COUNT] = {
	{ mtlChars("void"),   Void,   0 },
	{ mtlChars("bool"),   Bool,   1 },
	{ mtlChars("float"),  Float,  1 },
	{ mtlChars("float2"), Float2, 2 },
	{ mtlChars("float3"), Float3, 3 },
	{ mtlChars("float4"), Float4, 4 }
};

static const TypeInfo gSubTypes[TYPE_COUNT] = {
	{ mtlChars("void"),  Void,  0 },
	{ mtlChars("bool"),  Bool,  1 },
	{ mtlChars("float"), Float, 1 },
	{ mtlChars("float"), Float, 1 },
	{ mtlChars("float"), Float, 1 },
	{ mtlChars("float"), Float, 1 }
};

static const mtlChars Members = "xyzw";
static const char     Accessor = '.';

struct ExpressionNode
{
	void GetLane(const mtlChars &val, int lane, mtlString &out) const;

	virtual      ~ExpressionNode( void ) {}
	virtual int   Evaluate(const mtlChars &dst, mtlString &out, int lane, int depth) = 0;
	virtual bool  IsLeaf( void ) const = 0;
	virtual void  AppendValue(mtlString &out) = 0;
	virtual bool  IsConstant( void ) const = 0;
};

struct OperationNode : public ExpressionNode
{
	char            operation;
	ExpressionNode *left;
	ExpressionNode *right;

		  OperationNode( void ) : left(NULL), right(NULL) {}
		 ~OperationNode( void ) { delete left; delete right; }
	int   Evaluate(const mtlChars &dst, mtlString &out, int lane, int depth);
	bool  IsLeaf( void ) const { return false; }
	void  AppendValue(mtlString &out);
	bool  IsConstant( void ) const { return left->IsConstant() && right->IsConstant(); }
};

struct ValueNode : public ExpressionNode
{
	mtlString term;
	int       size;

	int  Evaluate(const mtlChars &dst, mtlString &out, int lane, int depth);
	bool IsLeaf( void ) const { return true; }
	void AppendValue(mtlString &out);
	bool IsConstant( void ) const { return term.IsFloat(); }
};

void ExpressionNode::GetLane(const mtlChars &val, int lane, mtlString &out) const
{
	out.Free();
	if (val.IsFloat()) {
		out.Copy(val);
	} else {
		int index = val.FindLastChar(Accessor);
		if (index != -1) {
			out.Append(mtlChars(val, 0, index));
			out.Append(Accessor);
			int size = val.GetSize() - (index + 1);
			out.Append(lane < size ? mtlChars(val, index+1, val.GetSize())[lane] : '0');
		} else {
			out.Append(val);
			out.Append(Accessor);
			out.Append(lane < Members.GetSize() ? Members[lane] : '0');
		}
	}
}

int OperationNode::Evaluate(const mtlChars &dst, mtlString &out, int lane, int depth)
{
	int ldepth = left->Evaluate(dst, out, lane, depth);
	int rdepth = right->IsLeaf() ? 0 : right->Evaluate(dst, out, lane, depth+1);
	int mdepth = 0;

	mtlString addr_str;
	if (depth > 0 || dst.GetSize() == 0) {
		out.Append('[');
		addr_str.FromInt(depth);
		out.Append(addr_str);
		out.Append(']');
		mdepth = depth;
	} else {
		mtlString temp;
		GetLane(dst, lane, temp);
		out.Append(temp);
		//out.Append(dst);
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

	return mmlMax(ldepth, mdepth, rdepth);
}

void OperationNode::AppendValue(mtlString &out)
{
	out.Append(operation);
}

int ValueNode::Evaluate(const mtlChars &dst, mtlString &out, int lane, int depth)
{
	int out_depth = 0;
	mtlString temp;
	if (depth > 0 || dst.GetSize() == 0) {
		out.Append('[');
		temp.FromInt(depth);
		out.Append(temp);
		out.Append(']');
		out_depth = depth;
	} else {
		//out.Append(dst);
		GetLane(dst, lane, temp);
		out.Append(temp);
		out_depth = 0;
	}
	out.Append("=");
	//AppendValue(out);
	GetLane(term, lane, temp);
	out.Append(temp);
	out.Append(';');

	return out_depth;
}

void ValueNode::AppendValue(mtlString &out)
{
	out.Append(term);
}

bool IsBraceBalanced(const mtlChars &expr)
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

int FindOperation(const mtlChars &operations, const mtlChars &expression)
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

bool GenerateTree(ExpressionNode *&node, mtlChars expr)
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

		retval = GenerateTree(op_node->left, lexpr) && GenerateTree(op_node->right, rexpr);

		node = op_node;
	} else { // STOPPING CONDITION

		ValueNode *val_node = new ValueNode;
		val_node->term.Copy(expr);

		node = val_node;
	}
	return retval;
}

ExpressionNode *GenerateTree(const mtlChars &expr)
{
	ExpressionNode *tree = NULL;
	if (!GenerateTree(tree, expr)) {
		if (tree != NULL) {
			delete tree;
			tree = NULL;
		}
	}
	return tree;
}

Definition                  *GetType(CompileInstance &inst, const mtlChars &name);
bool                         IsValidName(const mtlChars &name);
const TypeInfo              *GetTypeInfo(const mtlChars &type);
bool                         EmitOperand(CompileInstance &inst, const mtlChars &operand);
bool                         AssignVar(CompileInstance &inst, const mtlChars &name, const mtlChars &expr);
bool                         DeclareVar(CompileInstance &inst, const mtlChars &type, const mtlChars &name, const mtlChars &expr);
bool                         CompileScope(CompileInstance &inst, const mtlChars &input, bool branch);
bool                         CompileFunction(CompileInstance &inst, const mtlChars &ret_type, const mtlChars &name, const mtlChars &params, const mtlChars &body);
bool                         CompileCondition(CompileInstance &inst, const mtlChars &cond, const mtlChars &if_body, const mtlChars &else_body);
bool                         CompileStatement(CompileInstance &inst, const mtlChars &statement);
Scope                       &PushScope(CompileInstance &inst, bool branch);
void                         PopScope(CompileInstance &inst);
void                         EmitInstruction(CompileInstance &inst, swsl::InstructionSet instr);
void                         EmitInstruction(CompileInstance &inst, swsl::InstructionSet instr, mtlItem<swsl::Instruction> *&at);
void                         EmitImmutable(CompileInstance &inst, float fl);
void                         EmitImmutable(CompileInstance &inst, float fl, mtlItem<swsl::Instruction> *&at);
void                         EmitAddress(CompileInstance &inst, swsl::addr_t addr);
void                         EmitAddress(CompileInstance &inst, swsl::addr_t addr, mtlItem<swsl::Instruction> *&at);
void                         PushStack(CompileInstance &inst, int size);
void                         PopStack(CompileInstance &inst, int size);
const swsl::InstructionInfo *GetInstructionInfo(swsl::InstructionSet instr);
const swsl::InstructionInfo *GetInstructionInfo(const mtlChars &instr);
bool                         IsGlobal(const CompileInstance &inst);
swsl::addr_t                 GetRelAddress(const CompileInstance &inst, swsl::addr_t addr);
void                         AddError(CompileInstance &inst, const mtlChars &msg, const mtlChars &ref);
void                         CopyCompilerMessages(const mtlList<swsl::CompilerMessage> &a, mtlList<swsl::CompilerMessage> &b);
mtlChars                     GetBaseName(const mtlChars &name);
mtlChars                     GetBaseMembers(const mtlChars &name);

Definition *GetType(CompileInstance &inst, const mtlChars &name)
{
	mtlItem<Scope> *scope = inst.scopes.GetLast();
	while (scope != NULL) {
		mtlItem<Definition> *def = scope->GetItem().defs.GetFirst();
		while (def != NULL) {
			if (def->GetItem().name.Compare(name, true)) {
				return &def->GetItem();
			}
			def = def->GetNext();
		}
		scope = scope->GetPrev();
	}
	return NULL;
}

bool IsValidName(const mtlChars &name)
{
	if (name.GetSize() > 0 && GetTypeInfo(name) == NULL) {
		if (!mtlChars::IsAlpha(name.GetChars()[0]) && name.GetChars()[0] != '_') {
			return false;
		}
		for (int i = 1; i < name.GetSize(); ++i) {
			char ch = name.GetChars()[i];
			if (!mtlChars::IsAlphanumeric(ch) && ch != '_') {
				return false;
			}
		}
		return true;
	}
	return false;
}

const TypeInfo *GetTypeInfo(const mtlChars &type)
{
	for (int i = 0; i < TYPE_COUNT; ++i) {
		if (gTypes[i].name.Compare(type, true)) {
			return gTypes+i;
		}
	}
	return NULL;
}

bool EmitOperand(CompileInstance &inst, const mtlChars &operand)
{
	const Definition *type = GetType(inst, operand);
	if (type == NULL) {
		float fl_val;
		if (operand.ToFloat(fl_val)) {
			EmitImmutable(inst, fl_val);
		} else {
			Parser p;
			p.SetBuffer(operand);
			mtlList<mtlChars> m;
			if (p.Match("[%i]", m) == 0) {
				int stack_offset;
				m.GetFirst()->GetItem().ToInt(stack_offset);
				EmitAddress(inst, inst.scopes.GetLast()->GetItem().rel_sptr - stack_offset);
			} else {
				AddError(inst, "Unknown operand", operand);
				return false;
			}
		}
	} else {
		EmitAddress(inst, GetRelAddress(inst, type->value.var_addr));
	}
	return true;
}

bool AssignVar(CompileInstance &inst, const mtlChars &name, const mtlChars &expr)
{
	mtlChars base_name = GetBaseName(name);
	Definition *type = GetType(inst, base_name);
	if (type == NULL) {
		AddError(inst, "Undeclared variable", name);
		return false;
	}
	if (type->mut != Mutable) {
		AddError(inst, "Modifying a constant", name);
		return false;
	}

	ExpressionNode *tree = GenerateTree(expr);
	if (tree == NULL) {
		AddError(inst, "Malformed expression", expr);
		return false;
	}


	mtlChars          base_mem = GetBaseMembers(name);
	bool              result = true;
	Parser            parser;
	mtlList<mtlChars> ops;
	mtlList<mtlChars> m;
	mtlString         order_str;
	const int         num_lanes = (base_mem.GetSize() > 0) ? base_mem.GetSize() : type->type.size;

	for (int lane = 0; lane < num_lanes; ++lane) {

		order_str.Free();
		const int stack_size = tree->Evaluate(name, order_str, lane, 0);

		PushStack(inst, stack_size);

		order_str.SplitByChar(ops, ';');
		mtlItem<mtlChars> *op = ops.GetFirst();

		while (op != NULL && op->GetItem().GetSize() > 0) {

			parser.SetBuffer(op->GetItem());

			switch (parser.Match("%s+=%s%|%s-=%s%|%s*=%s%|%s/=%s%|%s=%s", m, NULL)) {
			case 0: EmitInstruction(inst, swsl::FADD_MM); break;
			case 1: EmitInstruction(inst, swsl::FSUB_MM); break;
			case 2: EmitInstruction(inst, swsl::FMUL_MM); break;
			case 3: EmitInstruction(inst, swsl::FDIV_MM); break;
			case 4: EmitInstruction(inst, swsl::FSET_MM); break;
			default:
				AddError(inst, "Invalid syntax", op->GetItem());
				return false;
				break;
			}

			mtlItem<swsl::Instruction> *instr_item = inst.program.GetLast();

			const mtlChars dst = m.GetFirst()->GetItem();
			const mtlChars src = m.GetFirst()->GetNext()->GetItem();

			EmitOperand(inst, dst);
			if (src.IsFloat()) {
				*((int*)(&instr_item->GetItem().instr)) += 1;
			}
			EmitOperand(inst, src);

			op = op->GetNext();
		}

		PopStack(inst, stack_size);
	}

	delete tree;

	return result;
}

bool DeclareVar(CompileInstance &inst, const mtlChars &type, const mtlChars &name, const mtlChars &expr)
{
	const TypeInfo *type_info = GetTypeInfo(type);
	if (type_info == NULL) {
		AddError(inst, "Unknown type", type);
		return false;
	}

	if (!IsValidName(name)) {
		AddError(inst, "Invalid name", name);
		return false;
	}

	const Definition *prev_def = GetType(inst, name);
	if (prev_def != NULL) {
		AddError(inst, "Redeclaration", name);
		return false;
	}

	const int rel_sptr = inst.scopes.GetLast()->GetItem().rel_sptr;

	Definition &def = inst.scopes.GetLast()->GetItem().defs.AddLast();
	def.name.Copy(name);
	def.mut            = Mutable;
	def.type           = *type_info;
	def.scope_level    = inst.scopes.GetSize() - 1;
	def.value.var_addr = rel_sptr;

	//def.values.Create(type_info->size);
	//for (int addr_offset = 0; addr_offset < type_info->size; ++addr_offset) {
	//	def.values[addr_offset].var_addr = rel_sptr + addr_offset;
	//}

	for (int i = 0; i < type_info->size; ++i) {
		Definition &def = inst.scopes.GetLast()->GetItem().defs.AddLast();
		def.name.Copy(name);
		def.name.Append(Accessor);
		def.name.Append(Members[i]);
		def.mut            = Mutable;
		def.type           = gSubTypes[type_info->type];
		def.scope_level    = inst.scopes.GetSize() - 1;
		def.value.var_addr = rel_sptr + i;
	}

	PushStack(inst, type_info->size);

	return (expr.GetSize() > 0) ? AssignVar(inst, name, expr) : true;
}

bool CompileScope(CompileInstance &inst, const mtlChars &input, bool branch)
{
	bool result = true;
	Scope &scope = PushScope(inst, branch);
	scope.parser.SetBuffer(input);
	mtlList<mtlChars> m;
	while (result && !scope.parser.IsEnd()) {
		switch (scope.parser.Match("{%s}%|%w%w(%s){%s}%|if(%s){%s}else{%s}%|if(%s){%s}%|%s;", m, NULL)) {
		case 0: // SCOPE
			result = CompileScope(inst, m.GetFirst()->GetItem(), false);
			break;
		case 1: // FUNCTION
			result = CompileFunction(inst, m.GetFirst()->GetItem(), m.GetFirst()->GetNext()->GetItem(), m.GetFirst()->GetNext()->GetNext()->GetItem(), m.GetFirst()->GetNext()->GetNext()->GetNext()->GetItem());
			break;
		case 2: // CONDITION
			result = CompileCondition(inst, m.GetFirst()->GetNext()->GetItem(), m.GetFirst()->GetNext()->GetNext()->GetItem(), "");
			break;
		case 3: // CONDITION
			result = CompileCondition(inst, m.GetFirst()->GetNext()->GetItem(), m.GetFirst()->GetNext()->GetNext()->GetItem(), m.GetFirst()->GetNext()->GetNext()->GetNext()->GetItem());
			break;
		case 4: // STATEMENT
			result = CompileStatement(inst, m.GetFirst()->GetItem());
			break;
		default:
			AddError(inst, "Malformed sequence", input);
			return false;
			break;
		}
	}
	PopScope(inst);
	return result;
}

bool CompileFunction(CompileInstance &inst, const mtlChars &ret_type, const mtlChars &name, const mtlChars &params, const mtlChars &body)
{
	if (!IsGlobal(inst)) { // TODO: I want to support this eventually, but I need to emit jump instructions
		AddError(inst, "Nested functions not allowed", name);
		return false;
	}

	PushScope(inst, false);
	inst.scopes.GetLast()->GetItem().rel_sptr = 0; // reset the relative stack pointer
	bool result = true;
	bool is_main = false;
	result &= DeclareVar(inst, ret_type, name, "");

	if (name.Compare("main", true)) {
		++inst.main;
		if (inst.main > 1) {
			AddError(inst, "Multiple entry points", "");
			return false;
		}
		*inst.prog_entry = (char)inst.program.GetSize();
		is_main = true;
	}

	Parser p;
	p.SetBuffer(params);
	mtlList<mtlChars> m;
	int stack_start = inst.scopes.GetLast()->GetItem().size;
	while (!p.IsEnd()) {
		if (p.Match("%w%w", m, NULL) == 0) {
			result &= DeclareVar(inst, m.GetFirst()->GetItem(), m.GetFirst()->GetNext()->GetItem(), "");
		} else {
			AddError(inst, "Parameter syntax", params);
			return false;
		}
		p.Match(",", m);
	}
	int stack_end = inst.scopes.GetLast()->GetItem().size;
	result &= CompileScope(inst, body, false);
	PopScope(inst);
	if (is_main) {
		*inst.prog_inputs = stack_end - stack_start;
		EmitInstruction(inst, swsl::END);
	} else {
		// FIX: This is a placeholder to prevent the compiler from optimizing stack manipulations
		EmitInstruction(inst, swsl::NOP); // this should emit a SET instruction (set the value of the return value)
	}
	return result;
}

bool CompileCondition(CompileInstance &inst, const mtlChars &cond, const mtlChars &if_body, const mtlChars &else_body)
{
	// emit test instructions
	// emit condition mask

	bool result = CompileScope(inst, if_body, true) && CompileScope(inst, else_body, true);

	// output merges
	return result;
}

bool CompileStatement(CompileInstance &inst, const mtlChars &statement)
{
	if (IsGlobal(inst)) {
		AddError(inst, "Global statements not allowed", statement);
		return false;
	}

	Parser parser;
	parser.SetBuffer(statement);
	mtlList<mtlChars> m;
	mtlChars seq;
	bool result = false;
	switch (parser.Match("%w%w=%s%|%s=%s%|%w(%s)%|%w%w", m, &seq)) {
	case 0:
		result = DeclareVar(inst, m.GetFirst()->GetItem(), m.GetFirst()->GetNext()->GetItem(), m.GetFirst()->GetNext()->GetNext()->GetItem());
		break;
	case 1:
		result = AssignVar(inst, m.GetFirst()->GetItem(), m.GetFirst()->GetNext()->GetItem());
		break;
	case 2:
		// TODO: implement function calling
		// store result in temp
		result = false;
		AddError(inst, "Calling functions is not supported yet", seq);
		break;
	case 3:
		result = DeclareVar(inst, m.GetFirst()->GetItem(), m.GetFirst()->GetNext()->GetItem(), "");
		break;
	default:
		result = false;
		AddError(inst, "Malformed statement", statement);
		break;
	}
	return result;
}

Scope &PushScope(CompileInstance &inst, bool branch)
{
	int nested_branch = 0;
	if (inst.scopes.GetSize() > 0) {
		nested_branch = inst.scopes.GetLast()->GetItem().nested_branch;
	}
	Scope &scope = inst.scopes.AddLast();
	scope.size = 0;
	scope.nested_branch = nested_branch + (int)branch;
	if (inst.scopes.GetLast()->GetPrev() != NULL) {
		scope.rel_sptr = inst.scopes.GetLast()->GetPrev()->GetItem().rel_sptr;
	} else {
		scope.rel_sptr = 0;
	}
	return scope;
}

void PopScope(CompileInstance &inst)
{
	if (inst.scopes.GetSize() <= 0) { return; }
	Scope &scope = inst.scopes.GetLast()->GetItem();
	PopStack(inst, scope.size);
	inst.scopes.RemoveLast();
}

void EmitInstruction(CompileInstance &inst, swsl::InstructionSet instr)
{
	swsl::Instruction i;
	if (instr != swsl::END) {
		if (inst.stack_manip < 0) {
			i.instr = swsl::UPOP_I;
			inst.program.AddLast(i);
			EmitAddress(inst, (swsl::addr_t)(-inst.stack_manip));
			inst.stack_manip = 0;
		} else if (inst.stack_manip > 0) {
			i.instr = swsl::UPUSH_I;
			inst.program.AddLast(i);
			EmitAddress(inst, (swsl::addr_t)inst.stack_manip);
			inst.stack_manip = 0;
		}
	} else {
		inst.stack_manip = 0;
	}
	i.instr = instr;
	inst.program.AddLast(i);
}

void EmitInstruction(CompileInstance &inst, swsl::InstructionSet instr, mtlItem<swsl::Instruction> *&at)
{
	swsl::Instruction i;
	i.instr = instr;
	at = inst.program.Insert(i, at);
}

void EmitImmutable(CompileInstance &inst, float fl)
{
	swsl::Instruction i;
	i.fl_imm = fl;
	inst.program.AddLast(i);
}

void EmitImmutable(CompileInstance &inst, float fl, mtlItem<swsl::Instruction> *&at)
{
	swsl::Instruction i;
	i.fl_imm = fl;
	at = inst.program.Insert(i, at);
}

void EmitAddress(CompileInstance &inst, swsl::addr_t addr)
{
	swsl::Instruction i;
	i.u_addr = addr;
	inst.program.AddLast(i);
}

void EmitAddress(CompileInstance &inst, swsl::addr_t addr, mtlItem<swsl::Instruction> *&at)
{
	swsl::Instruction i;
	i.u_addr = addr;
	at = inst.program.Insert(i, at);
}

void PushStack(CompileInstance &inst, int size)
{
	inst.stack_manip += size;
	inst.scopes.GetLast()->GetItem().size += size;
	inst.scopes.GetLast()->GetItem().rel_sptr += size;
}

void PopStack(CompileInstance &inst, int size)
{
	inst.stack_manip -= size;
	inst.scopes.GetLast()->GetItem().size -= size;
	inst.scopes.GetLast()->GetItem().rel_sptr -= size;
}

const swsl::InstructionInfo *GetInstructionInfo(swsl::InstructionSet instr)
{
	return ((int)instr < swsl::INSTR_COUNT) ? swsl::gInstr + (int)instr : NULL;
}

const swsl::InstructionInfo *GetInstructionInfo(const mtlChars &instr)
{
	for (int i = 0; i < swsl::INSTR_COUNT; ++i) {
		if (swsl::gInstr[i].name.Compare(instr, true)) {
			return swsl::gInstr + i;
		}
	}
	return NULL;
}

bool IsGlobal(const CompileInstance &inst)
{
	return inst.scopes.GetSize() <= 1;
}

swsl::addr_t GetRelAddress(const CompileInstance &inst, swsl::addr_t addr)
{
	return inst.scopes.GetLast()->GetItem().rel_sptr - addr;
}

void AddError(CompileInstance &inst, const mtlChars &msg, const mtlChars &ref)
{
	swsl::CompilerMessage &m = inst.errors.AddLast();
	m.msg.Copy(msg);
	m.ref.Copy(ref);
}

void CopyCompilerMessages(const mtlList<swsl::CompilerMessage> &a, mtlList<swsl::CompilerMessage> &b)
{
	const mtlItem<swsl::CompilerMessage> *i = a.GetFirst();
	while (i != NULL) {
		swsl::CompilerMessage &m = b.AddLast();
		m.msg.Copy(i->GetItem().msg);
		m.ref.Copy(i->GetItem().ref);
		i = i->GetNext();
	}
}

mtlChars GetBaseName(const mtlChars &name)
{
	const int mem_index = name.FindLastChar(Accessor);
	mtlChars ret_val = (mem_index < 0) ? name : mtlChars(name, 0, mem_index);
	return ret_val;
}

mtlChars GetBaseMembers(const mtlChars &name)
{
	const int mem_index = name.FindLastChar(Accessor);
	mtlChars ret_val = (mem_index < 0) ? "" : mtlChars(name, mem_index + 1, name.GetSize());
	return ret_val;
}

bool swsl::Compiler::Compile(const mtlChars &input, swsl::Shader &output)
{
	output.Delete();
	CompileInstance inst;
	inst.main = 0;
	inst.stack_manip = 0;
	EmitAddress(inst, 0); // number of inputs
	inst.prog_inputs = &(inst.program.GetLast()->GetItem().u_addr);
	EmitAddress(inst, 2); // entry point (only a guess, is modified later)
	inst.prog_entry = &(inst.program.GetLast()->GetItem().u_addr);
	bool result = CompileScope(inst, input, false);
	if (inst.main == 0) {
		AddError(inst, "Missing \'main\'", "");
	}
	output.m_program.Create(inst.program.GetSize());
	mtlItem<Instruction> *j = inst.program.GetFirst();
	for (int i = 0; i < output.m_program.GetSize(); ++i) {
		output.m_program[i] = j->GetItem();
		j = j->GetNext();
	}
	CopyCompilerMessages(inst.errors, output.m_errors);
	CopyCompilerMessages(inst.warnings, output.m_warnings);
	return result && output.m_errors.GetSize() == 0;
}

bool swsl::Disassembler::Disassemble(const swsl::Shader &shader, mtlString &output)
{
	output.Free();
	output.Reserve(4096);
	mtlString num;

	output.Append("0     inputs   ");
	num.FromInt(shader.m_program[0].u_addr);
	output.Append(num);
	output.Append("\n");
	output.Append("1     entry    ");
	num.FromInt(shader.m_program[1].u_addr);
	output.Append(num);
	output.Append("\n");

	for (int iptr = 2; iptr < shader.m_program.GetSize(); ) {
		num.FromInt(iptr);
		output.Append(num);
		for (int j = 0; j < (6 - num.GetSize()); ++j) {
			output.Append(' ');
		}
		const InstructionInfo *instr = GetInstructionInfo((swsl::InstructionSet)shader.m_program[iptr++].instr);
		if (instr == NULL) {
			output.Append("<<Unknown instruction. Abort.>>");
			return false;
		}
		output.Append(instr->name);
		for (int i = 0; i < (9 - instr->name.GetSize()); ++i) {
			output.Append(' ');
		}
		if (iptr + instr->params - 1 >= shader.m_program.GetSize()) {
			output.Append("<<Parameter corruption. Abort.>>");
			return false;
		}
		for (int i = 0; i < instr->params; ++i) {
			if (i == instr->params - 1 && instr->const_float_src) {
				num.FromFloat(shader.m_program[iptr++].fl_imm);
				output.Append(num);
			} else {
				num.FromInt(shader.m_program[iptr++].u_addr);
				output.Append(num);
			}
			for (int j = 0; j < (6 - num.GetSize()); ++j) {
				output.Append(' ');
			}
		}
		output.Append("\n");
	}
	return shader.GetErrorCount() == 0;
}
