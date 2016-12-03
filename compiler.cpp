#include <iostream>
#include "compiler.h"
#include "swsl_instr.h"
#include "swsl_aux.h"

// preparser
	// remove comments
	// unroll loops
	// expand expression lanes
	// concatenate file contents (don't require include guards)

// math parser
	// calculates depth of expression
	// splits expression into an order of basic operations

// Instead of analyzing the conditional scope
	// All variables not declared in the scope will be modified with compounded comparison mask
	// optimization: when getting a variable not declared in the current scope, declare a new variable that shadows global with current mask, merge local with global at end of scope

// Note
	// Return instruction directly modifies mask
	// If mask is all zeroes, returns actually causes an instruction jump

const mtlChars Keywords[] = {
	"if", "else", "for", "while",
	"continue", "break", "return",
	"const", "mutable",
	"import", "export",
	"struct",
	"void",
	"bool", "true", "false",
	"int",   "int2",   "int3",   "int4",
	"fixed", "fixed2", "fixed3", "fixed4",
	"unit",  "unit2",  "unit3",  "unit4",
	"float", "float2", "float3", "float4"
};

void Compiler::AddError(const mtlChars &err, const mtlChars &msg)
{
	Message *m = &m_errors.AddLast();
	m->err.Copy(err);
	m->msg.Copy(msg);
}

Compiler::File *Compiler::AddFile(const mtlChars &filename)
{
	mtlItem<Compiler::File> *i = m_files.GetFirst();
	while (i != NULL) {
		if (i->GetItem().m_path.GetPath().Compare(filename)) { return NULL; }
	}

	m_files.AddLast().m_path = filename;
	m_file_stack.AddLast(&m_files.GetLast()->GetItem());

	return m_file_stack.GetLast()->GetItem();
}

void Compiler::GenerateExpressionTree(Compiler::ExprNode *&node, const mtlChars &expr)
{
	mtlSyntaxParser p;
	p.SetBuffer(expr);
	mtlArray<mtlChars> m;
	NodeType node_type = (NodeType)p.Match("%s+%S  %|  %s-%S  %|  %S*%S  %|  %S/%S  %|  %S==%S  %|  %S!=%S  %|  %S<%S  %|  %S<=%S  %|  %S>%S  %|  %S>=%S  %|  %S&&%S  %|  %S||%S  %|  (%S)%0  %|  %S(%s)%0  %|  %S", m);
	switch (node_type) {

	case NodeType_Add:
	case NodeType_Sub:
	case NodeType_Mul:
	case NodeType_Div: {
		ExprBranch *expr_branch = new ExprBranch;
		node = expr_branch;
		GenerateExpressionTree(expr_branch->left, m[0].GetSize() > 0 ? m[0] : "0");
		GenerateExpressionTree(expr_branch->right, m[1]);
		break;
	}

	case NodeType_Eq:
	case NodeType_Neq:
	case NodeType_Less:
	case NodeType_LessEq:
	case NodeType_Greater:
	case NodeType_GreaterEq:
	case NodeType_And:
	case NodeType_Or: {
		ExprBranch *expr_branch = new ExprBranch;
		node = expr_branch;
		GenerateExpressionTree(expr_branch->left, m[0]);
		GenerateExpressionTree(expr_branch->right, m[1]);
		break;
	}

	case NodeType_Paranthesis:
		GenerateExpressionTree(node, m[0]);
		break;

	case NodeType_Func:
	case NodeType_Term:
		node = new ExprTerm;
		break;
	}

	if (node_type != NodeType_Paranthesis) {
		node->str = expr;
		node->type = node_type;
	}
}

void Compiler::SimplifyExpressionTree(Compiler::ExprNode *&node)
{
	// Simplifies an expression by evaluating constants nodes
}

Compiler::TypeInfo Compiler::ClassifyType(const mtlChars &type_name) const
{
	TypeInfo v;
	v.name = type_name;

	if (v.name.Compare(Keywords[Token_Void], true)) {
		v.type = VariableType_Void;
		v.size = 0;
	}
	// else if (v.name.Compare(Keywords[Token_Bool], true)) {
	//	v.type = VariableType_Bool;
	//	v.size = 1;
	//} else if (v.name.Compare(Keywords[Token_Int], true)) {
	//	v.type = VariableType_Int;
	//	v.size = 1;
	//} else if (v.name.Compare(Keywords[Token_Int2], true)) {
	//	v.type = VariableType_Int;
	//	v.size = 2;
	//} else if (v.name.Compare(Keywords[Token_Int3], true)) {
	//	v.type = VariableType_Int;
	//	v.size = 3;
	//} else if (v.name.Compare(Keywords[Token_Int4], true)) {
	//	v.type = VariableType_Int;
	//	v.size = 4;
	//} else if (v.name.Compare(Keywords[Token_Fixed], true)) {
	//	v.type = VariableType_Fixed;
	//	v.size = 1;
	//} else if (v.name.Compare(Keywords[Token_Fixed2], true)) {
	//	v.type = VariableType_Fixed;
	//	v.size = 2;
	//} else if (v.name.Compare(Keywords[Token_Fixed3], true)) {
	//	v.type = VariableType_Fixed;
	//	v.size = 3;
	//} else if (v.name.Compare(Keywords[Token_Fixed4], true)) {
	//	v.type = VariableType_Fixed;
	//	v.size = 4;
	//} else if (v.name.Compare(Keywords[Token_Unit], true)) {
	//	v.type = VariableType_Unit;
	//	v.size = 1;
	//} else if (v.name.Compare(Keywords[Token_Unit2], true)) {
	//	v.type = VariableType_Unit;
	//	v.size = 2;
	//} else if (v.name.Compare(Keywords[Token_Unit3], true)) {
	//	v.type = VariableType_Unit;
	//	v.size = 3;
	//} else if (v.name.Compare(Keywords[Token_Unit4], true)) {
	//	v.type = VariableType_Unit;
	//	v.size = 4;
	//}
	else if (v.name.Compare(Keywords[Token_Float], true)) {
		v.type = VariableType_Float;
		v.size = 1;
	} else if (v.name.Compare(Keywords[Token_Float2], true)) {
		v.type = VariableType_Float;
		v.size = 2;
	} else if (v.name.Compare(Keywords[Token_Float3], true)) {
		v.type = VariableType_Float;
		v.size = 3;
	} else if (v.name.Compare(Keywords[Token_Float4], true)) {
		v.type = VariableType_Float;
		v.size = 4;
	} else {
		// FIXME
			// check custom types
		v.type = VariableType_Unknown;
		v.size = 0;
	}

	return v;
}

bool Compiler::IsKeyword(const mtlChars &str) const
{
	for (int i = 0; i < Token_Count; ++i) {
		if (Keywords[i].Compare(str, false)) { return true; }
	}
	return false;
}

bool Compiler::IsValidName(const mtlChars &name) const
{
	if (name.GetSize() <= 0)                           { return false; }
	if (IsKeyword(name))                               { return false; }
	if (mtlChars::IsNumeric(name[0]))                  { return false; }
	if (!mtlChars::IsAlpha(name[0]) && name[0] != '_') { return false; }
	for (int i = 1; i < name.GetSize(); ++i) {
		if (!mtlChars::IsAlphanumeric(name[i]) && name[i] != '_') { return false; }
	}
	return true;
}

bool Compiler::NameConflict(const mtlChars &sym_name) const
{
	return m_sym.GetLast()->GetItem().GetEntry(sym_name) != NULL || m_sym.GetFirst()->GetItem().GetEntry(sym_name);
}

Compiler::SymbolType *Compiler::GetSymbol(const mtlChars &sym_name)
{
	mtlItem< mtlStringMap<SymbolType> > *sym_iter = m_sym.GetLast();
	while (sym_iter != NULL) {
		SymbolType *sym = sym_iter->GetItem().GetEntry(sym_name);
		if (sym != NULL) { return sym; }
		sym_iter = sym_iter->GetPrev();
	}
	return NULL;
}

void Compiler::DeclareSymbol(const mtlChars &sym_name, Compiler::SymbolType type)
{
	if (!IsValidName(sym_name)) { AddError("Invalid naming", sym_name); }
	SymbolType *sym_type = GetSymbol(sym_name);
	if (NameConflict(sym_name) || (sym_type != NULL && *sym_type == SymbolType_Struct)) {
		AddError("Redeclaration", sym_name);
	}

	*m_sym.GetLast()->GetItem().CreateEntry(sym_name) = type;
}

void Compiler::DeclareVariable(const mtlChars &var_decl)
{
	mtlSyntaxParser p;
	p.SetBuffer(var_decl);
	mtlArray<mtlChars> params;
	switch (p.Match("const %w%w%0  %|  mutable %w%w%0  %|  %w%w%0", params)) {
	case 0:
	case 1:
	case 2:
	{
		Variable v;
		v.type_info = ClassifyType(params[0]);
		v.name = params[1];
		if (v.type_info.type == VariableType_Unknown) { AddError("Unknown type", params[0]); }
		v.addr = (swsl::addr_t)m_top->stack_offset;
		*m_top->var.CreateEntry(v.name) = v;
		DeclareSymbol(params[1], SymbolType_Variable);
		ModifyStack(v.type_info.size);
		break;
	}

	default:
		AddError("Invalid declaration", "");
		break;
	}
}

void Compiler::DeclareParameters(const mtlList<mtlChars> &param_list)
{
	const mtlItem<mtlChars> *var_iter = param_list.GetFirst();
	while (var_iter != NULL) {
		DeclareVariable(var_iter->GetItem());
		var_iter = var_iter->GetNext();
	}
}

void Compiler::SplitParameters(const mtlChars &param_decl, mtlList<mtlChars> &out_params)
{
	out_params.RemoveAll();
	if (param_decl.Compare(Keywords[Token_Void], true) || param_decl.IsBlank()) { return; }
	param_decl.SplitByChar(out_params, ',');
}

void Compiler::ClassifyParameters(const mtlChars &param_decl, mtlList<Compiler::TypeInfo> &out_params)
{
	out_params.RemoveAll();
	mtlList<mtlChars> params;
	SplitParameters(param_decl, params);
	mtlItem<mtlChars> *param_iter = params.GetFirst();
	while (param_iter != NULL) {
		out_params.AddLast(ClassifyParameter(param_iter->GetItem()));
		param_iter = param_iter->GetNext();
	}
}

Compiler::TypeInfo Compiler::ClassifyParameter(const mtlChars &param_decl)
{
	mtlSyntaxParser p;
	p.SetBuffer(param_decl);
	TypeInfo type_info;
	mtlArray<mtlChars> m;
	switch (p.Match("const %w%0  %| const %w%w%0  %|  mutable %w%0  %|  mutable %w%w%0  %|  %w%0  %|  %w%w%0", m)) {
	case 0:
	case 1:
		type_info = ClassifyType(m[0]);
		type_info.access = VariableAccess_Const;
		break;

	case 2:
	case 3:
		type_info = ClassifyType(m[0]);
		type_info.access = VariableAccess_Mutable;
		break;

	case 4:
	case 5:
		type_info = ClassifyType(m[0]);
		type_info.access = VariableAccess_Mutable;
		break;

	default:
		type_info = ClassifyType(param_decl);
		type_info.access = VariableAccess_Const;
		break;
	}
	return type_info;
}

Compiler::Function *Compiler::GetFunction(const mtlChars &func_name)
{
	return m_funcs.GetEntry(func_name);
}

Compiler::Function *Compiler::DeclareFunction(const mtlChars &ret_type, const mtlChars &func_name, const mtlChars &params)
{
	Function *func = GetFunction(func_name);

	if (func != NULL) {

		AddError("Redeclaration", "");
		return NULL;

	} else {

		func = m_funcs.CreateEntry(func_name);

		func->name = func_name;
		ClassifyParameters(params, func->param_type);

		func->jmp_addr = m_out.GetSize();
		EmitInstruction(swsl::UNS_POP_I);
		EmitAddress(func->ret_type.size);
		EmitInstruction(swsl::UNS_JMP_I);
		EmitAddress(0); // this will be modified when the body is declared
		if (ClassifyType(ret_type).type == VariableType_Unknown) { AddError("Unknown type", ret_type); }
		DeclareSymbol(func_name, SymbolType_Function);

	}

	return func;
}

void Compiler::LoadFile(const mtlChars &filename, mtlString &file_contents)
{
	file_contents.Free();
	if (!mtlSyntaxParser::BufferFile(filename, file_contents)) {
		AddError("Could not open file", filename);
	}
}

void Compiler::CompileCodeUnit(mtlSyntaxParser &parser)
{
	if (!IsGlobalScope()) {
		CompileLocalCodeUnit(parser);
	} else {
		CompileGlobalCodeUnit(parser);
	}
}

void Compiler::CompileLocalCodeUnit(mtlSyntaxParser &parser)
{
	mtlArray<mtlChars> params;
	mtlChars seq;
	switch (parser.Match("{%s}  %|  if(%s){%s}else{%s}  %|  if(%s){%s}  %|  %s;  %|  %s", params, &seq)) {

	case 0:
		CompileScope(params[0]);
		break;

	case 1:
		CompileIfElse(params[0], params[1], params[2]);
		break;

	case 2:
		CompileIf(params[0], params[1]);
		break;

	case 3:
		CompileStatement(params[0]);
		break;

	case 4:
	default:
		AddError("Unknown local code unit", parser.GetBuffer());
		break;

	}
}

void Compiler::CompileGlobalCodeUnit(mtlSyntaxParser &parser)
{
	mtlString rem_num;
	rem_num.FromInt(parser.GetBufferRemaining().GetSize());
	swsl::print_ch(rem_num);
	swsl::print_ch(", \"");
	swsl::print_ch(parser.GetBufferRemaining());
	swsl::print_ch("\"\n");

	mtlArray<mtlChars> params;
	mtlChars seq;
	switch (parser.Match("import\"%s\"  %|  export struct %w{%s};  %|  struct %w{%s};  %|  export %w%w(%s){%s}  %|  %w%w(%s){%s}  %|  export %w%w(%s);  %|  %w%w(%s);  %|  %s", params, &seq)) {
	case 0:
		CompileFile(params[0]); // FIXME: The path needs to be relative to the current location of the file that is being compiled
		break;

	//case 1:
	//case 2:
		//	DeclareStruct(params[0], params[1]);
		//	break;

	case 3:
	case 4:
		CompileFunction(params[0], params[1], params[2], params[3]);
		break;

	case 5:
	case 6:
		DeclareFunction(params[0], params[1], params[2]);
		break;

	case 7:
	default:
		AddError("Unknown global code unit", parser.GetBufferRemaining());
		break;
	}
}

void Compiler::CompileStatement(const mtlChars &statement)
{
	mtlSyntaxParser p;
	p.SetBuffer(statement);
	mtlArray<mtlChars> params;
	switch (p.Match("%s=%s%|%s", params)) {
	case 0:
		CompileDeclarationAndExpression(params[0], params[1]);
		break;

	case 1:
		DeclareVariable(params[0]);
		break;

	default: // this will never happen
		break;
	}
}

void Compiler::CompileCode(const mtlChars &code)
{
	mtlSyntaxParser parser;
	parser.SetBuffer(code);
	while (!parser.IsEnd()) {
		CompileCodeUnit(parser);
	}
}

void Compiler::CompileScope(const mtlChars &scope)
{
	PushScope();
	CompileCode(scope);
	PopScope();
}

void Compiler::CompileCondition(const mtlChars &condition)
{
	// FIXME
}

void Compiler::CompileIfElse(const mtlChars &condition, const mtlChars &if_code, const mtlChars &else_code)
{
	CompileCondition(condition);
	EmitInstruction(swsl::TST_PUSH);
	CompileScope(if_code);
	EmitInstruction(swsl::TST_INV);
	CompileScope(else_code);
	EmitInstruction(swsl::TST_POP);
}

void Compiler::CompileIf(const mtlChars &condition, const mtlChars &code)
{
	CompileCondition(condition);
	EmitInstruction(swsl::TST_PUSH);
	CompileScope(code);
	EmitInstruction(swsl::TST_POP);
}

void Compiler::CompileFunction(const mtlChars &ret_type, const mtlChars &func_name, const mtlChars &params, const mtlChars &scope)
{
	if (func_name.Compare("main", true)) {
		if (!ret_type.Compare(Keywords[Token_Void], true)) { AddError("main return type not void", ""); }
		++m_has_main;
	}
	DefineFunction(ret_type, func_name, params, scope);
	PushScope();
	m_top->stack_offset = m_stack_pointer;
	mtlList<mtlChars> param_list;
	SplitParameters(params, param_list);
	DeclareParameters(param_list);
	CompileCode(scope);
	PopScope();
	EmitInstruction(swsl::RETURN);
}

void Compiler::DefineFunction(const mtlChars &ret_type, const mtlChars &func_name, const mtlChars &params, const mtlChars &body)
{
	Function *f = GetFunction(func_name);
	if (f != NULL) {
		if (f->body.GetSize() > 0) {
			AddError("Redefinition", "");
		}
		CompareFunctionSignature(ret_type, func_name, params);
	} else {
		f = DeclareFunction(ret_type, func_name, params);
		f->body = body;
	}
}

void Compiler::CompareFunctionSignature(const mtlChars &ret_type, const mtlChars &func_name, const mtlChars &params)
{
	Function *f = GetFunction(func_name);
	if (f != NULL) {
		if (!f->ret_type.name.Compare(ret_type, true)) {
			AddError("Function signature mismatch", "Return type mismatch");
		} else {
			mtlList<TypeInfo> param_type;
			ClassifyParameters(params, param_type);
			if (param_type.GetSize() != f->param_type.GetSize()) {
				AddError("Function signature mismatch", "Parameter count mismatch");
			} else {
				mtlItem<TypeInfo> *i = param_type.GetFirst(), *j = f->param_type.GetFirst();
				while (i != NULL && j != NULL) {
					if (i->GetItem().access != j->GetItem().access || i->GetItem().name.Compare(j->GetItem().name, true)) {
						AddError("Function signature mismatch", "Parameter type mismatch");
					}
					i = i->GetNext();
					j = j->GetNext();
				}
			}
		}
	} else {
		AddError("Undeclared function", func_name);
	}
}

void Compiler::CompileFile(const mtlChars &filename)
{
	File *f = AddFile(filename);
	if (f == NULL) { return; } // early exit to avoid infinite recursion
	LoadFile(filename, f->m_contents);
	CompileCode(f->m_contents);
	m_files.RemoveLast();
}

void Compiler::CompileDeclarationAndExpression(const mtlChars &decl, const mtlChars &expr)
{
	// FIXME
}

void Compiler::InitializeCompilerState(swsl::Shader &output)
{
	output.Delete();
	output.m_program.SetCapacity(512);
	m_top = NULL;
	m_out.RemoveAll();
	m_out.AddLast().u_addr = 0; // Input count
	m_out.AddLast().u_addr = 2; // Entry index
	m_scopes.RemoveAll();
	m_sym.RemoveAll();
	m_funcs.RemoveAll();
	m_errors.RemoveAll();
	m_stack_pointer = 0;
	m_has_main = 0;
	m_deferred_stack_manip = 0;
	EmitAddress(0);
	EmitAddress(2);
	PushScope();
}

void Compiler::DeclareIntrinsics( void )
{
	// FIXME
	// Declare built-in math functions that compile to a single instruction
		// sqrt, min, max, abs, floor, ceil, round, trunc, sin, cos, tan etc.
	// declare built-in math functions that do not compile to a single function
		// dot, cross etc.
}

bool Compiler::Success( void ) const
{
	return m_errors.GetSize() == 0;
}

void Compiler::PushScope( void )
{
	m_scopes.AddLast();
	m_sym.AddLast();
	m_top = &m_scopes.GetLast()->GetItem();
	m_top->stack_offset = 0;
	m_top->stack_size = 0;
	ModifyStack(1);
	EmitInstruction(swsl::TST_PUSH);
}

void Compiler::PopScope( void )
{
	if (m_scopes.GetSize() > 0) {
		EmitInstruction(swsl::TST_POP);
		ModifyStack(-m_top->stack_size);
		m_scopes.RemoveLast();
		m_top = &m_scopes.GetLast()->GetItem();
		m_sym.RemoveLast();
	}
}

void Compiler::ModifyStack(int stack_add)
{
	if (m_top != NULL) {
		m_stack_pointer        += stack_add;
		m_deferred_stack_manip += stack_add;
		m_top->stack_size      += stack_add;
	}
}

void Compiler::EmitInstruction(swsl::InstructionSet instruction)
{
	EmitStackMod();
	m_out.AddLast().instr = instruction;
}

void Compiler::EmitAddress(swsl::addr_t address)
{
	EmitStackMod();
	m_out.AddLast().u_addr = address;
}

void Compiler::EmitImmutable(float immutable)
{
	EmitStackMod();
	m_out.AddLast().fl_imm = immutable;
}

void Compiler::EmitStackMod( void )
{
	if (m_deferred_stack_manip < 0) {
		m_deferred_stack_manip = 0;
		EmitInstruction(swsl::UNS_POP_I);
		EmitAddress((swsl::addr_t)-m_deferred_stack_manip);
	} else if (m_deferred_stack_manip > 0) {
		m_deferred_stack_manip = 0;
		EmitInstruction(swsl::UNS_PUSH_I);
		EmitAddress((swsl::addr_t)m_deferred_stack_manip);
	}
}

bool Compiler::IsGlobalScope( void ) const
{
	return m_scopes.GetSize() == 1;
}

void Compiler::ProgramErrorCheck( void )
{
	if (m_has_main <= 0) { AddError("No program entry point", ""); }

	mtlList<const Function*> func_list;
	m_funcs.ToList(func_list);
	mtlItem<const Function*> *func_iter = func_list.GetFirst();
	while (func_iter != NULL) {
		if (func_iter->GetItem()->body.GetSize() <= 0) { AddError("Missing function definition (body)", ""); }
		func_iter = func_iter->GetNext();
	}
}

void Compiler::ConvertToShader(swsl::Shader &output) const
{
	output.m_program.Resize(m_out.GetSize());
	const mtlItem<swsl::Instruction> *out_iter = m_out.GetFirst();
	int instr_index = 0;
	while (out_iter != NULL) {
		output.m_program[instr_index] = out_iter->GetItem();
		out_iter = out_iter->GetNext();
		++instr_index;
	}
}

const mtlItem<Compiler::Message> *Compiler::GetError( void ) const
{
	return m_errors.GetFirst();
}

bool Compiler::Compile(const mtlChars &filename, swsl::Shader &output)
{
	InitializeCompilerState(output);
	DeclareIntrinsics();
	CompileFile(filename);
	ProgramErrorCheck();
	ConvertToShader(output);
	return Success();
}
