#ifndef COMPILER_H
#define COMPILER_H

#include "MiniLib/MTL/mtlString.h"
#include "MiniLib/MTL/mtlList.h"

#include "parser.h"

class Compiler;

struct CompilerMessage
{
	mtlChars msg;
	mtlChars ref;

	CompilerMessage(const mtlChars &_msg, const mtlChars &_ref) : msg(_msg), ref(_ref) {}
};

class Shader
{
	friend class Compiler;

private:
	mtlString                m_program;
	mtlList<CompilerMessage> m_errors;
	mtlList<CompilerMessage> m_warnings;

public:
	void Delete( void );
	bool IsValid( void ) const;
	int GetErrorCount( void ) const;
	int GetWarningCount( void ) const;
	const mtlItem<CompilerMessage> *GetErrors( void ) const;
	const mtlItem<CompilerMessage> *GetWarnings( void ) const;
};

class Compiler
{
public:
	bool Compile(const mtlChars &file, Shader &output);
};

#endif // COMPILER_H
