#include "compiler.h"
#include "instr.h"

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
	unsigned short var_addr;
	float          imm_value;
};

struct TypeInfo
{
	mtlChars name;
	Type     type;
	int      size;
};

struct Definition
{
	mtlChars        name;
	TypeInfo        type;
	Mutability      mut;
	mtlArray<Value> values;
};

struct Scope
{
	mtlList< Definition > defs;
	Parser                parser;
	int                   size;
};

struct CompileInstance
{
	mtlString                program;
	mtlList<CompilerMessage> errors;
	mtlList<CompilerMessage> warnings;
	mtlList<Scope>           scopes;
	mtlMathParser            evaluator;
	int                      base_sptr;
	int                      main;
};

const TypeInfo gTypes[TYPE_COUNT] = {
	{ mtlChars("void"),   Void,   0 },
	{ mtlChars("bool"),   Bool,   1 },
	{ mtlChars("float"),  Float,  1 },
	{ mtlChars("float2"), Float2, 2 },
	{ mtlChars("float3"), Float3, 3 },
	{ mtlChars("float4"), Float4, 4 }
};

Definition *GetType(CompileInstance &inst, const mtlChars &name);
bool IsValidName(const mtlChars &name);
const TypeInfo *GetTypeInfo(const mtlChars &type);
bool EmitOperand(CompileInstance &inst, const mtlChars &operand, int lane);
bool AssignVar(CompileInstance &inst, const mtlChars &name, const mtlChars &expr);
//bool DeclareConst(CompileInstance &inst, const mtlChars &type, const mtlChars &name, const mtlChars &expr);
bool DeclareVar(CompileInstance &inst, const mtlChars &type, const mtlChars &name, const mtlChars &expr);
bool CompileScope(CompileInstance &inst, const mtlChars &input);
bool CompileFunction(CompileInstance &inst, const mtlChars &ret_type, const mtlChars &name, const mtlChars &params, const mtlChars &body);
bool CompileCondition(CompileInstance &inst, const mtlChars &cond, const mtlChars &body);
bool CompileStatement(CompileInstance &inst, const mtlChars &statement);
Scope &PushScope(CompileInstance &inst);
void PopScope(CompileInstance &inst);

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
	if (name.GetSize() > 0) {
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

bool EmitOperand(CompileInstance &inst, const mtlChars &operand, int lane)
{
	const Definition *type = GetType(inst, operand);
	if (type == NULL) {
		float fl_val;
		if (operand.ToFloat(fl_val)) {
			for (int i = 0; i < sizeof(fl_val); ++i) {
				inst.program.Append(((char*)(&fl_val))[i]);
			}
		} else {
			Parser p;
			p.SetBuffer(operand);
			mtlList<mtlChars> m;
			if (p.Match("[%i]", m) == 0) {
				int stack_offset;
				m.GetFirst()->GetItem().ToInt(stack_offset);
				inst.program.Append(inst.scopes.GetLast()->GetItem().size - stack_offset);
			} else {
				return false;
			}
		}
	} else {
		inst.program.Append(inst.base_sptr - type->values[lane].var_addr); // this is wrong
	}
	return true;
}



bool AssignVar(CompileInstance &inst, const mtlChars &name, const mtlChars &expr)
{
	Definition *type = GetType(inst, name);
	if (type == NULL) {
		inst.errors.AddLast(CompilerMessage("Undeclared variable", name));
		return false;
	}
	if (type->mut != Mutable) {
		inst.errors.AddLast(CompilerMessage("Modifying a constant", name));
		return false;
	}

	bool result = true;

	mtlString order_str;
	const int stack_size = inst.evaluator.GetOrderOfOperations(expr, order_str, true);

	std::cout << "Order of ops: ";
	print_ch(order_str);
	std::cout << std::endl;

	inst.program.Append(UPUSH_I);
	inst.program.Append(stack_size);

	Parser parser;
	mtlList<mtlChars> m;
	mtlChars seq;

	for (int addr_offset = 0; addr_offset < type->type.size; ++addr_offset) {

		parser.SetBuffer(order_str);

		while (!parser.IsEnd()) {

			switch (parser.Match("%s+=%s;%|%s-=%s;%|%s*=%s;%|%s/=%s;%|%s=%s;", m, &seq)) {
			case 0: inst.program.Append(FADD_MM); print_ch(seq); std::cout << std::endl; break;
			case 1: inst.program.Append(FSUB_MM); print_ch(seq); std::cout << std::endl; break;
			case 2: inst.program.Append(FMUL_MM); print_ch(seq); std::cout << std::endl; break;
			case 3: inst.program.Append(FDIV_MM); print_ch(seq); std::cout << std::endl; break;
			case 4: inst.program.Append(FSET_MM); print_ch(seq); std::cout << std::endl; break;
			default:
				inst.errors.AddLast(CompilerMessage("Invalid syntax", seq));
				return false;
				break;
			}

			mtlChars dst = m.GetFirst()->GetItem();
			mtlChars src = m.GetFirst()->GetNext()->GetItem();

			EmitOperand(inst, dst, addr_offset);
			if (src.IsFloat()) {
				inst.program.GetChars()[inst.program.GetSize() - 1] += 1;
			}
			EmitOperand(inst, src, addr_offset);
		}
	}

	inst.program.Append(UPOP_I);
	inst.program.Append(stack_size);

	return result;
}

/*bool DeclareConst(CompileInstance &inst, const mtlChars &type, const mtlChars &name, const mtlChars &expr)
{
	// determine if true imm, or readonly
	// if true imm, eval answer in a VM

	const TypeInfo *type_info = GetTypeInfo(type);
	if (type_info == NULL) {
		inst.errors.AddLast(CompilerMessage("Unknown type", type));
		return false;
	}

	if (!IsValidName(name)) {
		inst.errors.AddLast(CompilerMessage("Invalid name", name));
		return false;
	}

	Definition &def = inst.scopes.GetLast()->GetItem().defs.AddLast();
	def.mut = Mutable; // set mutable so AssignVar can emit necessary instructions
	def.name = name;
	def.type = *type_info;
	def.values.Create(type_info->size);
	for (int addr_offset = 0; addr_offset < type_info->size; ++addr_offset) {
		def.values[addr_offset].var_addr = inst.stack_ptr + addr_offset;
	}

	inst.scopes.GetLast()->GetItem().size += type_info->size;
	inst.evaluator.SetVariable(name, 0.0f);

	inst.program.Append(UPUSH_I);
	inst.program.Append(type_info->size);
	inst.stack_ptr += type_info->size;

	bool result = (expr.GetSize() > 0) ? AssignVar(inst, name, expr) : true;

	def.mut = Readonly; // set to readonly so no subsequent calls to AssignVar succeeds

	return result;
}*/

bool DeclareVar(CompileInstance &inst, const mtlChars &type, const mtlChars &name, const mtlChars &expr)
{
	const TypeInfo *type_info = GetTypeInfo(type);
	if (type_info == NULL) {
		inst.errors.AddLast(CompilerMessage("Unknown type", type));
		return false;
	}

	if (!IsValidName(name)) {
		inst.errors.AddLast(CompilerMessage("Invalid name", name));
		return false;
	}

	Definition &def = inst.scopes.GetLast()->GetItem().defs.AddLast();
	def.mut = Mutable;
	def.name = name;
	def.type = *type_info;
	def.values.Create(type_info->size);
	for (int addr_offset = 0; addr_offset < type_info->size; ++addr_offset) {
		def.values[addr_offset].var_addr = inst.base_sptr + addr_offset;
	}

	inst.scopes.GetLast()->GetItem().size += type_info->size;
	inst.evaluator.SetVariable(name, 0.0f);

	inst.program.Append(UPUSH_I);
	inst.program.Append(type_info->size);

	return (expr.GetSize() > 0) ? AssignVar(inst, name, expr) : true;
}

bool CompileScope(CompileInstance &inst, const mtlChars &input)
{
	bool result = true;
	Scope &scope = PushScope(inst);
	scope.parser.SetBuffer(input);
	mtlList<mtlChars> m;
	mtlChars seq;
	while (result && !scope.parser.IsEnd()) {
		switch (scope.parser.Match("%w%w(%s){%s}%|if(%s){%s}%|%s;", m, &seq)) {
		case 0: // FUNCTION
			result = CompileFunction(inst, m.GetFirst()->GetItem(), m.GetFirst()->GetNext()->GetItem(), m.GetFirst()->GetNext()->GetNext()->GetItem(), m.GetFirst()->GetNext()->GetNext()->GetNext()->GetItem());
			break;
		case 1: // CONDITION
			result = CompileCondition(inst, m.GetFirst()->GetNext()->GetItem(), m.GetFirst()->GetNext()->GetNext()->GetItem());
			break;
		case 2: // STATEMENT
			result = CompileStatement(inst, m.GetFirst()->GetItem());
			break;
		default:
			inst.errors.AddLast(CompilerMessage("Malformed statement", seq));
			return false;
			break;
		}
	}
	PopScope(inst);
	return result;
}

bool CompileFunction(CompileInstance &inst, const mtlChars &ret_type, const mtlChars &name, const mtlChars &params, const mtlChars &body)
{
	PushScope(inst);
	bool result = true;
	bool is_main = false;
	result &= DeclareVar(inst, ret_type, name, "");

	if (name.Compare("main", true)) {
		++inst.main;
		if (inst.main > 1) {
			inst.errors.AddLast(CompilerMessage("Multiple entry points", ""));
			return false;
		}
		inst.program.GetChars()[gMetaData_EntryIndex] = (char)inst.program.GetSize();
		is_main = true;
	}

	Parser p;
	p.SetBuffer(params);
	mtlList<mtlChars> m;
	mtlChars seq;
	int stack_start = inst.scopes.GetLast()->GetItem().size;
	while (!p.IsEnd()) {
		if (p.Match("%w%w", m, &seq) == 0) {
			result &= DeclareVar(inst, m.GetFirst()->GetItem(), m.GetFirst()->GetNext()->GetItem(), "");
		} else {
			inst.errors.AddLast(CompilerMessage("Parameter syntax", seq));
			return false;
		}
		p.Match(",", m);
	}
	int stack_end = inst.scopes.GetLast()->GetItem().size;
	result &= CompileScope(inst, body);
	PopScope(inst);
	if (is_main) {
		inst.program.GetChars()[gMetaData_InputIndex] = stack_end - stack_start;
		inst.program.Append(END);
	}
	return result;
}

bool CompileCondition(CompileInstance &inst, const mtlChars &cond, const mtlChars &body)
{
	// output forks (based on cond)
	bool result = CompileScope(inst, body);
	// output merges
	return result;
}

bool CompileStatement(CompileInstance &inst, const mtlChars &statement)
{
	Parser parser;
	parser.SetBuffer(statement);
	mtlList<mtlChars> m;
	mtlChars seq;
	bool result = false;
	//switch (parser.Match("const %w%s=%s%|%w%s=%s%|%s=%s", m, &seq)) {
	//case 0: // const decl - determine readonly/immutable
	//	result = DeclareConst(inst, m.GetFirst()->GetItem(), m.GetFirst()->GetNext()->GetItem(), m.GetFirst()->GetNext()->GetNext()->GetItem());
	//	break;
	//case 1: // var decl
	switch (parser.Match("%w%s=%s%|%s=%s", m, &seq)) {
	case 0:
		result = DeclareVar(inst, m.GetFirst()->GetItem(), m.GetFirst()->GetNext()->GetItem(), m.GetFirst()->GetNext()->GetNext()->GetItem());
		break;
	//case 2: // reassignment (throw error if const)
	case 1:
		result = AssignVar(inst, m.GetFirst()->GetItem(), m.GetFirst()->GetNext()->GetItem());
		break;
	default:
		result = false;
		inst.errors.AddLast(CompilerMessage("Malformed statement", seq));
		break;
	}
	return result;
}

Scope &PushScope(CompileInstance &inst)
{
	if (inst.scopes.GetLast() != NULL) {
		inst.base_sptr += inst.scopes.GetLast()->GetItem().size;
	}
	Scope &scope = inst.scopes.AddLast();
	scope.size = 0;
	inst.evaluator.PushScope();
	return scope;
}

void PopScope(CompileInstance &inst)
{
	if (inst.scopes.GetSize() <= 0) { return; }
	Scope &scope = inst.scopes.GetLast()->GetItem();
	inst.program.Append(UPOP_I);
	inst.program.Append(scope.size);
	inst.scopes.RemoveLast();
	inst.evaluator.PopScope();
	if (inst.scopes.GetLast() != NULL) {
		inst.base_sptr -= inst.scopes.GetLast()->GetItem().size;
	}
}

bool Compiler::Compile(const mtlChars &input, Shader &output)
{
	CompileInstance inst;
	inst.base_sptr = 0;
	inst.main = 0;
	inst.program.Append(0); // number of inputs
	inst.program.Append(2); // entry point (only a guess, is modified later)
	bool result = CompileScope(inst, input);
	if (inst.main == 0) {
		inst.errors.AddLast(CompilerMessage("Missing \'main\'", ""));
	}
	inst.program.Append(END); // precautionary END
	output.m_program.Copy(inst.program);
	output.m_errors.Copy(inst.errors);
	output.m_warnings.Copy(inst.warnings);
	return result;
}

const InstructionInfo *GetInstructionInfo(Instruction instr)
{
	for (int i = 0; i < INSTR_COUNT; ++i) {
		if (gInstr[i].instr == instr) {
			return gInstr+i;
		}
	}
	return NULL;
}

bool Disassembler::Disassemble(const Shader &shader, mtlString &output)
{
	output.Free();
	output.Reserve(4096);
	mtlString num;

	output.Append("inputs   ");
	num.FromInt(shader.m_program.GetChars()[0]);
	output.Append(num);
	output.Append("\n");
	output.Append("entry    ");
	num.FromInt(shader.m_program.GetChars()[1]);
	output.Append(num);
	output.Append("\n");

	for (int iptr = 2; iptr < shader.m_program.GetSize(); ) {
		const InstructionInfo *instr = GetInstructionInfo((Instruction)shader.m_program.GetChars()[iptr++]);
		if (instr != NULL) {
			output.Append(instr->name);
			for (int i =0; i < (9 - instr->name.GetSize()); ++i) {
				output.Append(" ");
			}
			if (iptr + instr->params - 1 >= shader.m_program.GetSize()) {
				output.Append("<<Parameter corruption. Abort.>>");
				return false;
			}
			for (int i = 0; i < instr->params; ++i) {
				if (i == instr->params - 1 && instr->const_float_src) {
					const float *val = (const float*)(shader.m_program.GetChars() + iptr);
					num.FromFloat(*val);
					output.Append(num);
					iptr += sizeof(float);
				} else {
					num.FromInt(shader.m_program[iptr++]);
					output.Append(num);
				}
				output.Append("  ");
			}
			output.Append("\n");
		} else {
			output.Append("<<Unknown instruction. Abort.>>");
			return false;
		}
	}
	return shader.GetErrorCount() == 0;
}
