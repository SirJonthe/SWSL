#include <iostream>
#include "compiler.h"
#include "swsl_instr.h"
#include "swsl_aux.h"

void Compiler::PushScope( void )
{
	if (m_scopes.GetSize() > 0) {
		Scope *prev = &m_scopes.GetLast()->GetItem();
		Scope *next = &m_scopes.AddLast();
		next->scope_depth = prev->scope_depth + 1;
		next->mask_depth = m_mask_depth;
		next->scope_size = 0;

	} else {
		Scope *next = &m_scopes.AddLast();
		next->scope_depth = 1;
		next->mask_depth = m_mask_depth;
		next->scope_size = 0;
	}
	EmitPushScope();
}

void Compiler::PopScope( void )
{
	EmitPopScope();
	if (m_scopes.GetSize() > 0) {
		m_scopes.RemoveLast();
	}
}

void Compiler::AddError(const mtlChars &err, const mtlChars &msg)
{
	Message *m = &m_errors.AddLast();
	m->err.Copy(err);
	m->msg.Copy(msg);
}

Compiler::File *Compiler::AddFile(const mtlPath &filename)
{
	mtlItem<Compiler::File> *i = m_files.GetFirst();
	while (i != NULL) {
		if (i->GetItem().m_path.GetPath().Compare(filename.GetPath())) { return NULL; }
	}

	m_files.AddLast().m_path = filename;
	m_file_stack.AddLast(&m_files.GetLast()->GetItem());

	return m_file_stack.GetLast()->GetItem();
}

bool Compiler::Success( void ) const
{
	return m_errors.GetSize() == 0;
}

void Compiler::InitializeBaseCompilerState(swsl::Binary &output, const mtlChars &out_name)
{
	output.Free();
	m_errors.RemoveAll();
	m_files.RemoveAll();
	m_file_stack.RemoveAll();
	m_mask_depth = 0;
	m_global_scope = true;
	m_out_name = out_name.GetTrimmed(); // TODO: clean the name of whitespaces and special characters
}

void Compiler::CompileFile(const mtlPath &filename)
{
	File *f = AddFile(filename);
	if (f == NULL) { return; }
	LoadFile(filename, f->m_contents);
	CompileCode(f->m_contents);
	m_files.RemoveLast();
}

void Compiler::LoadFile(const mtlPath &filename, mtlString &file_contents)
{
	file_contents.Free();
	if (!mtlSyntaxParser::BufferFile(filename, file_contents)) {
		AddError("Could not open file", filename.GetPath());
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

void Compiler::CompileScope(const mtlChars &code)
{
	PushScope();
	CompileCode(code);
	PopScope();
}

void Compiler::CompileCodeUnit(mtlSyntaxParser &parser)
{
	if (!IsGlobalScope()) {
		CompileLocalCodeUnit(parser);
	} else {
		CompileGlobalCodeUnit(parser);
	}
}

bool Compiler::IsGlobalScope( void ) const
{
	return m_global_scope;
}

void Compiler::CompileLocalCodeUnit(mtlSyntaxParser &parser)
{
	mtlArray<mtlChars> params;
	mtlChars seq;
	switch (parser.Match("{%s}  %|  if(%S){%s}  %| %w(%s); %|  %s;  %|  %s", params, &seq)) {

	case 0:
		CompileScope(params[0]);
		break;

	case 1:
		CompileConditional(params[0], params[1], parser);
		break;

	case 2:
		EmitFunctionCall(params[0], params[1]);
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

void Compiler::CompileConditional(const mtlChars &condition, const mtlChars &body, mtlSyntaxParser &parser)
{
	++m_mask_depth;

	PushScope();
	EmitIf(condition);
	CompileScope(body);

	bool done = false;
	mtlArray<mtlChars> params;
	while (!done) {

		switch (parser.Match("else if(%S){%s} %| else{%s}", params)) {
		case 0:
			EmitElse();
			CompileConditional(params[0], params[1], parser);
			break;

		case 1:
			EmitElse();
			CompileScope(params[0]);

		default:
			done = true;
		}

	}

	PopScope();

	--m_mask_depth;
}

void Compiler::CompileStatement(const mtlChars &statement)
{
	EmitStatement(statement);
}

void Compiler::CompileGlobalCodeUnit(mtlSyntaxParser &parser)
{
	mtlArray<mtlChars> params;
	mtlChars seq;
	switch (parser.Match("import\"%s\"  %|  export struct %w{%s};  %|  struct %w{%s};  %|  export %w%w(%s){%s}  %|  %w%w(%s){%s}  %|  export %w%w(%s);  %|  %w%w(%s);  %|  %s", params, &seq)) {
	case 0:
		CompileFile(params[0]); // TODO: The path needs to be relative to the current location of the file that is being compiled
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

void Compiler::CompileFunction(const mtlChars &ret_type, const mtlChars &func_name, const mtlChars &params, const mtlChars &scope)
{
	m_global_scope = false;
	EmitFunctionSignature(ret_type, func_name, params);
	CompileScope(scope);
	m_global_scope = true;
}

void Compiler::DeclareFunction(const mtlChars &ret_type, const mtlChars &func_name, const mtlChars &params)
{
	EmitFunctionSignature(ret_type, func_name, params);
}

unsigned int Compiler::GetMaskDepth( void ) const
{
	return m_mask_depth;
}

const mtlChars &Compiler::GetProgramName( void ) const
{
	return m_out_name;
}

const mtlItem<Compiler::Message> *Compiler::GetError( void ) const
{
	return m_errors.GetFirst();
}

bool Compiler::Compile(const mtlPath &filename, swsl::Binary &output, const mtlChars &out_name)
{
	InitializeBaseCompilerState(output, out_name);
	InitializeCompilerState(output);
	CompileFile(filename);
	ProgramErrorCheck();
	ConvertToOutput(output);
	return Success();
}
