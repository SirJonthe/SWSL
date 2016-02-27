#ifndef COMPILER_H
#define COMPILER_H

#include "swsl_shader.h"

class Compiler
{
public:
	bool Compile(const mtlChars &input, swsl::Shader &output);
};

#endif // COMPILER_H
