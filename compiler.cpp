#include <iostream>
#include "compiler.h"
#include "swsl_instr.h"
#include "swsl_aux.h"

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

void Compiler::InitializeCompilerState( void )
{
	m_errors.RemoveAll();
	m_files.RemoveAll();
	m_file_stack.RemoveAll();
	m_global_scope = true;
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

void Compiler::CompileIfElse(const mtlChars &condition, const mtlChars &if_code, const mtlChars &else_code)
{
	CompileIf(condition, if_code);
	EmitElse();
	CompileScope(else_code);
}

void Compiler::CompileIf(const mtlChars &condition, const mtlChars &body)
{
	EmitIf(condition);
	CompileScope(body);
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

const mtlItem<Compiler::Message> *Compiler::GetError( void ) const
{
	return m_errors.GetFirst();
}

bool Compiler::Compile(const mtlPath &filename, swsl::Binary &output)
{
	InitializeCompilerState();
	CompileFile(filename);
	ProgramErrorCheck();
	ConvertToOutput(output);
	return Success();
}
