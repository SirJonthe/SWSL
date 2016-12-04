#ifndef SWSL_COMPILER_H
#define SWSL_COMPILER_H

#include "swsl_parser.h"
#include "swsl_shader.h"

namespace swsl
{

class Compiler
{
public:
	bool Compile(const mtlChars &file, swsl::Shader &output);
};

class Disassembler
{
public:
	bool Disassemble(const swsl::Shader &shader, mtlString &output);
};

}

#endif // COMPILER_H
