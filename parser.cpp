#include "parser.h"

#define OpenBraces   "([{"
#define ClosedBraces ")]}"
#define Quotes       "\'\""
#define Newlines     "\n\r"
#define Whitespaces  " \t\n\r"
#define EndOfStream  256
#define Escape       '\\'

bool Parser::IsFormat(char ch, const mtlChars &format) const
{
	Parser parser;
	parser.SetBuffer(format);
	bool match = false;
	char fmt_char;

	while (!parser.IsEnd() && !match) {

		fmt_char = (char)parser.ReadChar();

		if (fmt_char == '%') {

			char type = parser.ReadChar();

			switch (type) {
			case 'a': { // alphabetic
				match = mtlChars::IsAlpha(ch);
				break;
			}
			case 'd': { // decimal
				match = mtlChars::IsNumeric(ch);
				break;
			}
			case 'f': { // float
				match = IsFormat(ch, "%d.f");
				break;
			}
			case 'b': { // binary
				match = mtlChars::IsBin(ch);
				break;
			}
			case 'h': { // hexadecimal
				match = mtlChars::IsHex(ch);
				break;
			}
			default:
				return false;
			}

		} else {
			if (fmt_char == Escape) { fmt_char = (char)parser.ReadChar(); }
			match = mtlChars::ToLower(ch) == mtlChars::ToLower(fmt_char);
		}
	}

	return match;
}

void Parser::SkipWhitespaces( void )
{
	while (!IsEnd() && mtlChars::SameAsAny(m_buffer[m_reader], Whitespaces, sizeof(Whitespaces))) {
		if (mtlChars::SameAsAny(m_buffer[m_reader], Newlines, sizeof(Newlines))) {
			++m_line;
		}
		++m_reader;
	}
}

short Parser::ReadChar( void )
{
	if (IsEnd()) { return EndOfStream; }

	char ch = m_buffer[m_reader++];

	int quote_index = -1;
	if (mtlChars::SameAsAny(ch, Whitespaces, sizeof(Whitespaces))) {
		ch = ' ';
		SkipWhitespaces();
	} else if ((quote_index = mtlChars::SameAsWhich(ch, Quotes, sizeof(Quotes))) != -1) {
		if (!InQuote()) {
			m_quote_char = Quotes[quote_index];
		} else if (Quotes[quote_index] == m_quote_char) {
			m_quote_char = 0;
		}
	} else if (!InQuote()) {
		int open_index = -1;
		int closed_index = -1;
		if ((open_index = mtlChars::SameAsWhich(ch, OpenBraces, sizeof(OpenBraces))) != -1) {
			m_brace_stack.AddLast(OpenBraces[open_index]);
		} else if ((closed_index = mtlChars::SameAsWhich(ch, ClosedBraces, sizeof(ClosedBraces))) != -1) {
			open_index = mtlChars::SameAsWhich(m_brace_stack.GetLast()->GetItem(), OpenBraces, sizeof(OpenBraces));
			if (open_index == closed_index) {
				m_brace_stack.RemoveLast();
			}
		}
	}

	return (short)ch;
}

short Parser::PeekChar( void ) const
{
	return IsEnd() ? EndOfStream : (short)m_buffer[m_reader];
}

bool Parser::VerifyBraces(const mtlChars &str) const
{
	Parser parser;
	parser.SetBuffer(str);
	while (!parser.IsEnd()) {
		parser.ReadChar();
	}
	return parser.GetBraceDepth() == 0;
}

mtlChars Parser::Read(const mtlChars &format)
{
	if (IsEnd()) { return mtlChars(); }

	const int start = m_reader;

	if (format.GetSize() > 0) {
		while (!IsEnd()) {
			const char next_ch = (char)PeekChar();
			if (!IsFormat(next_ch, format)) { break; }
			ReadChar();
		}
	} else {
		ReadChar();
	}

	const int end = m_reader;
	SkipWhitespaces();
	return m_buffer.GetSubstring(start, end);
}

mtlChars Parser::ReadTo(char ch)
{
	int brace_depth = GetBraceDepth();
	int start = m_reader;

	while (!IsEnd()) {
		if (GetBraceDepth() == brace_depth && (char)PeekChar() == ch) { break; }
		ReadChar();
	}

	mtlChars str = m_buffer.GetSubstring(start, m_reader).GetTrimmed();
	SkipWhitespaces();
	return str;
}

int Parser::MatchSingle(const mtlChars &expr, mtlList<mtlChars> &out, mtlChars *seq)
{
	//m_brace_stack.RemoveAll();

	mtlItem<char> *brace_item = m_brace_stack.GetLast();

	if (!VerifyBraces(expr)) {
		return (int)ExpressionInputError;
	}

	out.RemoveAll();
	Parser ep;
	ep.SetBuffer(expr);
	int result = 1;
	int start = m_reader;
	int brace_depth = GetBraceDepth();

	while (!ep.IsEnd() && result == 1) {

		if (IsEnd()) {
			result = (int)ExpressionNotFound;
			continue;
		}

		mtlChars b = ep.Read("");

		if (b.Compare("%") && !ep.IsEnd()) {
			b = ep.Read("");
			switch (b[0]) {
			case 'c':
				out.AddLast(Read(""));
				break;
			case 'a':
				out.AddLast(Read("%a"));
				break;
			case 'f': {
					mtlChars sf = Read("%d.f");
					if (!sf.IsFloat()) {
						result = (int)ExpressionNotFound;
					} else {
						out.AddLast(sf);
					}
				}
				break;
			case 'i':
				out.AddLast(Read("%d"));
				break;
			case 'r':
			case 'd':
				out.AddLast(Read("%d."));
				break;
			case 'w':
				out.AddLast(Read("%a%d_"));
				break;
			case 's':
				out.AddLast(ReadTo((char)ep.PeekChar()));
				break;
			case '0':
				if (ep.PeekChar() != EndOfStream) { result = (int)ExpressionNotFound; }
				break;
			default: {
					/*mtlList<mtlChars> m;
					mtlChars s;
					switch (ep.Match("(%s)%|<%s>%|[%c-%c]", m, &s)) {
					case 0:
						out.AddLast(Read(m.GetFirst()->GetItem()));
						break;
					case 1:
						out.AddLast(Read(s));
						break;
					case 2:
						out.AddLast(Read(s));
						break;
					default:
						result = (int)ExpressionInputError;
						break;
					}*/
					result = (int)ExpressionInputError;
				}
				break;
			}
			if (out.GetLast()->GetItem().GetSize() < 1) {
				result = (int)ExpressionNotFound;
			}
		} else if (!b.Compare(Read(""))) {
			result = (int)ExpressionNotFound;
			continue;
		}
	}
	if (result == 1 && GetBraceDepth() != brace_depth) {
		result = (int)ExpressionNotFound;
	}
	if (result != 1) {
		out.RemoveAll();
		m_reader = start;
	}
	if (seq != NULL) {
		*seq = this->m_buffer.GetSubstring(start, m_reader);
		seq->Trim();
	}

	while (m_brace_stack.GetLast() != brace_item && m_brace_stack.GetSize() > 0) {
		m_brace_stack.RemoveLast();
	}

	return result;
}

Parser::Parser( void ) : m_copy(), m_buffer(), m_reader(0), m_brace_stack(), m_line(0), m_quote_char(0)
{}

void Parser::SetBuffer(const mtlChars &buffer)
{
	m_copy.Free();
	m_buffer = buffer;
	m_reader = 0;
	m_brace_stack.RemoveAll();
	SkipWhitespaces();
}

void Parser::CopyBuffer(const mtlChars &buffer)
{
	m_copy.Copy(buffer);
	m_buffer = m_copy;
	m_reader = 0;
	m_brace_stack.RemoveAll();
	SkipWhitespaces();
}

bool Parser::BufferFile(const mtlDirectory &p_file, mtlString &p_buffer)
{
	std::ifstream fin(p_file.GetDirectory().GetChars(), std::ios::ate|std::ios::binary);
	if (!fin.is_open()) { return false; }
	p_buffer.SetSize((int)fin.tellg());
	fin.seekg(0, std::ios::beg);
	return !fin.read(p_buffer.GetChars(), p_buffer.GetSize()).bad();
}

bool Parser::IsEnd( void ) const
{
	return m_reader >= m_buffer.GetSize();
}

bool Parser::InQuote( void ) const
{
	return m_quote_char != 0;
}

int Parser::GetBraceDepth( void ) const
{
	return m_brace_stack.GetSize();
}

int Parser::GetBraceDepth(char brace_type) const
{
	const mtlItem<char> *iter = m_brace_stack.GetFirst();
	int depth = 0;
	while (iter != NULL) {
		if (iter->GetItem() == brace_type) {
			++depth;
		}
		iter = iter->GetNext();
	}
	return depth;
}

int Parser::Match(const mtlChars &expr, mtlList<mtlChars> &out, mtlChars *seq)
{
	mtlList<mtlChars> exprs;
	expr.SplitByString(exprs, "%|");
	mtlItem<mtlChars> *expr_iter = exprs.GetFirst();
	int i = 0;
	while (expr_iter != NULL) {
		int code = MatchSingle(expr_iter->GetItem(), out, seq);
		switch (code) {
		case 1:                    return i;
		case ExpressionInputError: return (int)ExpressionInputError;
		default:                   break;
		}
		++i;
		expr_iter = expr_iter->GetNext();
	}
	return (int)ExpressionNotFound;
}
