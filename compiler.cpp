#include "compiler.h"

#include "swsl_wide.h"

#include "MiniLib/MTL/mtlArray.h"
#include "MiniLib/MTL/mtlMathParser.h"

void Shader::Delete( void )
{
	m_program.Free();
	m_errors.RemoveAll();
	m_warnings.RemoveAll();
}

bool Shader::IsValid( void ) const
{
	return m_program.GetSize() > 0 && m_errors.GetSize() == 0;
}

int Shader::GetErrorCount( void ) const
{
	return m_errors.GetSize();
}

int Shader::GetWarningCount( void ) const
{
	return m_warnings.GetSize();
}

const mtlItem<CompilerMessage> *Shader::GetErrors( void ) const
{
	return m_errors.GetFirst();
}

const mtlItem<CompilerMessage> *Shader::GetWarnings( void ) const
{
	return m_warnings.GetFirst();
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
	int                      stack_ptr;
};

const TypeInfo gTypes[TYPE_COUNT] = {
	{ mtlChars("void"),   Void,   0 },
	{ mtlChars("bool"),   Bool,   1 },
	{ mtlChars("float"),  Float,  1 },
	{ mtlChars("float2"), Float2, 2 },
	{ mtlChars("float3"), Float3, 3 },
	{ mtlChars("float4"), Float4, 4 }
};

enum Instruction
{
	NOP,
	END,

	FPUSH_M,
	FPUSH_I,
	UPUSH_I,

	FPOP_M,
	UPOP_I,

	UJMP_I,

	FSET_MM,
	FSET_MI,

	FADD_MM,
	FADD_MI,

	FSUB_MM,
	FSUB_MI,

	FMUL_MM,
	FMUL_MI,

	FDIV_MM,
	FDIV_MI,

	INSTR_COUNT
};

struct InstructionInfo
{
	mtlChars    name;
	Instruction instr;
	int         params;
};

const InstructionInfo gInstr[INSTR_COUNT] = {
	{ mtlChars("nop"),     NOP,    0 },
	{ mtlChars("end"),     END,    0 },
	{ mtlChars("fpush_m"), FPUSH_M, 1 },
	{ mtlChars("fpush_i"), FPUSH_I, 1 },
	{ mtlChars("upush_i"), UPUSH_I, 1 },
	{ mtlChars("fpop_m"),  FPOP_M,  1 },
	{ mtlChars("upop_i"),  UPOP_I,  1 },
	{ mtlChars("ujmp_i"),  UJMP_I,  1 },
	{ mtlChars("fset_mm"), FSET_MM, 2 },
	{ mtlChars("fset_mi"), FSET_MI, 2 },
	{ mtlChars("fadd_mm"), FADD_MM, 2 },
	{ mtlChars("fadd_mi"), FADD_MI, 2 },
	{ mtlChars("fsub_mm"), FSUB_MM, 2 },
	{ mtlChars("fsub_mi"), FSUB_MI, 2 },
	{ mtlChars("fmul_mm"), FMUL_MM, 2 },
	{ mtlChars("fmul_mi"), FMUL_MI, 2 },
	{ mtlChars("fdiv_mm"), FDIV_MM, 2 },
	{ mtlChars("fdiv_mi"), FDIV_MI, 2 }
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
				inst.program.Append(inst.stack_ptr + stack_offset - 1);
			} else {
				return false;
			}
		}
	} else {
		inst.program.Append(type->values[lane].var_addr);
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

	inst.program.Append(UPUSH_I);
	inst.program.Append(stack_size);

	for (int addr_offset = 0; addr_offset < type->type.size; ++addr_offset) {

		Parser parser;
		parser.SetBuffer(order_str);
		mtlList<mtlChars> m;
		mtlChars seq;

		while (!parser.IsEnd()) {

			switch (parser.Match("%s+=%s%|%s-=%s%|%s*=%s%|%s/=%s%|%s=%s", m, &seq)) {
			case 0: inst.program.Append(FADD_MM); break;
			case 1: inst.program.Append(FSUB_MM); break;
			case 2: inst.program.Append(FMUL_MM); break;
			case 3: inst.program.Append(FDIV_MM); break;
			case 4: inst.program.Append(FSET_MM); break;
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

			parser.Match(";", m);
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
		def.values[addr_offset].var_addr = inst.stack_ptr + addr_offset;
	}

	inst.scopes.GetLast()->GetItem().size += type_info->size;
	inst.evaluator.SetVariable(name, 0.0f);

	inst.program.Append(UPUSH_I);
	inst.program.Append(type_info->size);
	inst.stack_ptr += type_info->size;

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
	result &= DeclareVar(inst, ret_type, name, "");
	Parser p;
	p.SetBuffer(params);
	mtlList<mtlChars> m;
	mtlChars seq;
	while (!p.IsEnd()) {
		if (p.Match("%w%w", m, &seq) == 0) {
			result &= DeclareVar(inst, m.GetFirst()->GetItem(), m.GetFirst()->GetNext()->GetItem(), "");
		} else {
			inst.errors.AddLast(CompilerMessage("Parameter syntax", seq));
			return false;
		}
		p.Match(",", m);
	}
	result &= CompileScope(inst, body);
	PopScope(inst);
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
	Scope &scope = inst.scopes.AddLast();
	scope.size = 0;
	inst.evaluator.PushScope();
	return scope;
}

void PopScope(CompileInstance &inst)
{
	if (inst.scopes.GetSize() <= 0) { return; }
	Scope &scope = inst.scopes.GetLast()->GetItem();
	inst.stack_ptr -= scope.size;
	inst.program.Append(UPOP_I);
	inst.program.Append(scope.size);
	inst.scopes.RemoveLast();
	inst.evaluator.PopScope();
}

bool Compiler::Compile(const mtlChars &input, Shader &output)
{
	CompileInstance inst;
	inst.stack_ptr = 0;
	bool result = CompileScope(inst, input);
	output.m_program.Copy(inst.program);
	output.m_errors.Copy(inst.errors);
	output.m_warnings.Copy(inst.warnings);
	return result;
}
