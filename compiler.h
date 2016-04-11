#ifndef COMPILER_H
#define COMPILER_H

#include "MiniLib/MTL/mtlStringMap.h"
#include "swsl_shader.h"

bool Compile(const mtlChars &filename, swsl::Shader &output);

#endif // COMPILER_H
