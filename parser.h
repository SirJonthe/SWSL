#ifndef PARSER_H
#define PARSER_H

#include "MiniLib/MTL/mtlString.h"
#include "MiniLib/MTL/mtlList.h"

class Parser
{
private:
	mtlString     m_copy;
	mtlChars      m_buffer;
	int           m_reader;
	mtlList<char> m_brace_stack;
	int           m_line;
	char          m_quote_char;

public:
	enum ExpressionResult
	{
		ExpressionNotFound   = -1,
		ExpressionInputError = -2
	};

private:
	bool     IsFormat(char ch, const mtlChars &format) const;
	void     SkipWhitespaces( void );
	short    ReadChar( void );
	short    PeekChar( void ) const;
	bool     VerifyBraces(const mtlChars &str) const;
	mtlChars Read(const mtlChars &expr);
	mtlChars ReadTo(char ch);
	int      MatchSingle(const mtlChars &expr, mtlList<mtlChars> &out, mtlChars *seq = NULL);

public:
	Parser( void );

	void SetBuffer(const mtlChars &buffer);
	void CopyBuffer(const mtlChars &buffer);

	bool IsEnd( void ) const;
	bool InQuote( void ) const;
	int  GetBraceDepth( void ) const;
	int  GetBraceDepth(char brace_type) const;

	int Match(const mtlChars &expr, mtlList<mtlChars> &out, mtlChars *seq = NULL);
};

#endif // PARSER_H
