#ifndef COMPILER_H
#define COMPILER_H

#include "parser.h"
#include "shader.h"

class Compiler
{
public:
	bool Compile(const mtlChars &file, Shader &output);
};

#endif // COMPILER_H
