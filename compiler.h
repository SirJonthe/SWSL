#ifndef COMPILER_H
#define COMPILER_H

#include "parser.h"
#include "shader.h"

class Compiler
{
public:
	bool Compile(const mtlChars &file, Shader &output);
};

class Disassembler
{
public:
	bool Disassemble(const Shader &shader, mtlString &output);
};

#endif // COMPILER_H
