#include <iostream>
#include "parser.h"
#include "compiler.h"

void LoadFile(const mtlChars &filename, mtlString &file_contents, swsl::Shader &output)
{
	file_contents.Free();
	file_contents.Reserve(4096);
	if (!Parser::BufferFile(filename, file_contents)) {
		AddError(output, "Could not open file", filename);
	}
}

mtlChars GetItem(const mtlList<mtlChars> &items, int index)
{
	mtlItem<mtlChars> *item = items.GetFirst();
	for (int i = 0; i < index; ++i) {
		item = item->GetNext()
	}
	return item->GetItem();
}

void CompileStatement(Parser &parser, swsl::Shader &output)
{
	mtlList<mtlChars> params;
	switch (parser.Match("{%s}", params)) {

	case 0:
		CompileScope(GetItem(params, 0), output);
		break;

	default:
		AddError(output, "Unknown statement", );
		break;

	}
}

void CompileScope(const mtlChars &scope, swsl::Shader &output)
{
	Parser parser;
	parser.SetBuffer(scope);
	while (!parser.IsEnd()) {
		CompileStatement(scope, output);
	}
}

void CompileFile(const mtlChars &filename, swsl::Shader &output)
{
	mtlString file_contents;
	LoadFile(filename, file_contents, output);
	CompileScope(file_contents, output);
}

bool Compile(const mtlChars &filename, swsl::Shader &output)
{
	output.Delete();
	CompileFile(filename, output);
	return output.GetErrorCount() == 0;
}
