//#define MPL_FALLBACK_SCALAR

#include <iostream>
#include <fstream>

#include <SDL/SDL.h>

#include "MiniLib/MTL/mtlParser.h"
#include "MiniLib/MGL/mglText.h"
#include "MiniLib/MML/mmlMatrix.h"
#include "MiniLib/MPL/mplAlloc.h"

#include "swsl.h"
#include "swsl_gfx.h"
#include "swsl_aux.h"
#include "swsl_astgen.h"
#include "swsl_cpptrans.h"

// Things I should look into:
// Buffers should allocate an extra register at the edges so that screen resolutions that are not multiples of SWSL_WIDTH render properly

// http://forum.devmaster.net/t/rasterizing-with-sse/18590/11

#define video SDL_GetVideoSurface()

mglByteOrder32 ByteOrder( void )
{
	static const mglByteOrder32 byte_order = { 0x00000102 };
	return byte_order;
}

template < int n >
struct Vertex
{
	swsl::Point2D coord;
	mmlVector<n>  attributes;
};

class Printer
{
private:
	int x, y;
	int x_margin, y_margin;
	int line_height;
	int size;
	unsigned char r, g, b;

public:
	Printer( void );
	void ResetCaret( void );
	void SetCaret(int _x, int _y);
	void Newline( void );
	void Print(const mtlChars &str);
	void Print(int i);
	void Print(float f);
	void SetColor(unsigned char _r, unsigned char _g, unsigned char _b);
	void SetSize(int _size);
};

Printer::Printer( void ) : x(3), y(3), x_margin(3), y_margin(3), line_height(1), size(1), r(255), g(255), b(255) {}

void Printer::ResetCaret( void )
{
	SetCaret(x_margin, y_margin);
}

void Printer::SetCaret(int _x, int _y)
{
	x_margin = x = _x;
	y_margin = y = _y;
	line_height = size;
}

void Printer::Newline( void )
{
	x = x_margin;
	y += line_height * mglFontSmall_CharHeightPx;
	line_height = size;
}

void Printer::Print(const mtlChars &str)
{
	line_height = mmlMax(line_height, size);
	for (int i = 0; i < str.GetSize(); ++i) {

		char ch = str[i];

		if (mtlChars::IsNewline(ch)) {
			y += line_height;
			line_height = size;
			x = x_margin;
			continue;
		}

		if (!mtlChars::IsWhitespace(ch)) {
			mglDrawCharSmall(ch, (mtlByte*)video->pixels, video->format->BytesPerPixel, ByteOrder(), video->w, video->h, x, y, r, g, b, size);
		}

		x += mglFontSmall_CharWidthPx * size;
	}
}

void Printer::Print(int i)
{
	mtlString i_to_s;
	i_to_s.FromInt(i);
	Print(i_to_s);
}

void Printer::Print(float f)
{
	mtlString f_to_s;
	f_to_s.FromFloat(f);
	Print(f_to_s);
}

void Printer::SetSize(int _size)
{
	size = _size;
}

void Printer::SetColor(unsigned char _r, unsigned char _g, unsigned char _b)
{
	r = _r;
	g = _g;
	b = _b;
}

#include <limits>

void OutputSIMDInfo( void )
{
	std::cout << sizeof(char*) * CHAR_BIT << " bit binary" << std::endl;
	std::cout <<
			 #if MPL_SIMD == MPL_SIMD_NONE
				 "Scalar"
			 #elif MPL_SIMD == MPL_SIMD_SSE
				"SSE"
			 #elif MPL_SIMD == MPL_SIMD_AVX256
				"AVX256"
			 #elif MPL_SIMD == MPL_SIMD_AVX512
				"AVX512"
			 #elif MPL_SIMD == MPL_SIMD_NEON
				 "NEON"
			 #endif
				 << " @ " << MPL_WIDTH << " wide" << std::endl;
}

int PathTest( void )
{
	//mtlPath p1("darp/parp/karp.larp", mtlPath::File);
	//mtlPath p2("../narp/tarp", mtlPath::Directory);

	mtlPath p1("../a.b", mtlPath::File);
	mtlPath p2("../c.d", mtlPath::File);

	swsl::print_line(p1.GetPath());
	swsl::print_line(p2.GetPath());

	mtlPath p3 = p1 + p2;

	swsl::print_line(p3.GetPath());

	return 0;
}

int SplitTest( void )
{
	mtlChars str1 = "a, b, c, d";
	mtlChars str2 = "a, , c, d,";

	mtlList<mtlChars> split;
	str1.SplitByChar(split, ',');

	mtlItem<mtlChars> *i = split.GetFirst();
	while (i != NULL) {
		swsl::print_ch("\"");
		swsl::print_ch(i->GetItem());
		swsl::print_line("\"");
		i = i->GetNext();
	}

	std::cout << std::endl;

	str2.SplitByChar(split, ',');

	i = split.GetFirst();
	while (i != NULL) {
		swsl::print_ch("\"");
		swsl::print_ch(i->GetItem());
		swsl::print_line("\"");
		i = i->GetNext();
	}

	return 0;
}

int CppTranslatorTest( void )
{
	swsl::SyntaxTreeGenerator gen;
	std::cout << "Generating tree..." << std::flush;
	swsl::SyntaxTree *t = gen.Generate("../swsl_samples/test.swsl");
	std::cout << "done" << std::endl;
	swsl::CppTranslator c;
	swsl::Binary bin;
	std::cout << "Compiling tree..." << std::endl;
	if (!c.Compile(t, "wide", bin)) {
		std::cout << "failed" << std::endl;
		return 1;
	}
	std::cout << "done" << std::endl;
	std::cout << "Discarding tree..." << std::flush;
	delete t;
	std::cout << "done" << std::endl;
	std::cout << "Writing output..." << std::flush;
	std::ofstream fout("../swsl_samples/out.h", std::ios::binary);
	if (!fout.is_open()) {
		std::cout << "failed" << std::endl;
		return 1;
	}
	fout.write(bin.GetChars(), bin.GetSize());
	std::cout << "done" << std::endl;
	return 0;
}

float scalar_max(const float &a, const float &b, bool)
{
	if (a < b) { return b; }
	return a;
}

#include "tmp_out.h"
int CodeCorrectnessTest( void )
{
	std::cout << "testing correctness (find max value)..." << std::endl;

	srand(time(0));

	const int size = MPL_WIDTH;
	float a[size];
	float b[size];
	for (int i = 0; i < size; ++i) {
		a[i] = rand() % 90 + 10;
		b[i] = rand() % 90 + 10;
		std::cout << "  " << a[i] << "  " << b[i] << std::endl;
	}

	float scal[size];
	for (int i = 0; i < size; ++i) {
		scal[i] = scalar_max(a[i], b[i], true);
	}
	float wide[size];
	for (int i = 0; i < size; i+=MPL_WIDTH) {
		*(mpl::wide_float*)(wide + i) = wide_max(*(mpl::wide_float*)(a + i), *(mpl::wide_float*)(b + i), true);
	}

	std::cout << "  scal:";
	for (int i = 0; i < size; ++i) { std::cout << "  " << scal[i]; }
	std::cout << std::endl;
	std::cout << "  wide:";
	for (int i = 0; i < size; ++i) { std::cout << "  " << wide[i]; }
	std::cout << std::endl;

	std::cout << "done" << std::endl;
	return 0;
}

void print_cmpct_ch(const mtlChars &ch)
{
	int spaces = 0;
	for (int i = 0; i < ch.GetSize(); ++i) {
		if (ch[i] == ' ' || ch[i] == '\t') {
			if (spaces == 0) {
				std::cout << " ";
				++spaces;
			} else {
				continue;
			}
		} else {
			spaces = 0;
			if (ch[i] == '\n' || ch[i] == '\r') {
				std::cout << "|";
			} else {
				std::cout << ch[i];
			}
		}
	}
}

void print_ch(const mtlChars &ch)
{
	for (int i = 0; i < ch.GetSize(); ++i) {
		std::cout << ch[i];
	}
}

int ParserTest( void )
{
	std::cout << "testing parser..." << std::flush;

	mtlString file;
	const mtlChars test_buffer = "../test_buffer.txt";
	if (!mtlSyntaxParser::BufferFile(test_buffer, file)) {
		std::cout << "failed to read input file" << std::endl;
		return 1;
	}

	std::cout << std::endl;

	mtlSyntaxParser p;
	p.SetBuffer(file);
	p.EnableDiagnostics();
	mtlArray<mtlChars> m;
	while (!p.IsEnd()) {

		switch (p.Match("#include \"%S\" %| struct%w{%s} %| %w%?(&)%w(%s){%s} %| %s", m)) {

		case 0:
			std::cout << "  include: \"";
			print_ch(m[0]);
			std::cout << "\"" << std::endl;
			break;

		case 1:
			std::cout << "  struct: \"";
			print_ch(m[0]);
			std::cout << "\", \"";
			print_cmpct_ch(m[1]);
			std::cout << "\"" << std::endl;
			break;

		case 2:
			std::cout << "  func: \"";
			print_ch(m[0]);
			std::cout << "\", \"";
			print_ch(m[1]);
			std::cout << "\", \"";
			print_ch(m[2]);
			std::cout << "\", \"";
			print_cmpct_ch(m[3]);
			std::cout << "\", \"";
			print_cmpct_ch(m[4]);
			std::cout << "\"" << std::endl;
			break;

		default:
			std::cout << "  unknown: \"";
			print_cmpct_ch(m[0]);
			std::cout << "\"" << std::endl << "failed" << std::endl;
			print_ch(p.GetDiagnostics());
			return 1;
		}
		p.Match(";");
	}

	p.SetBuffer("+1.0");
	p.EnableDiagnostics();
	p.SetHyphenators("");
	if (p.Match("%r%0", m) < 0) {
		std::cout << "  this is a bug" << std::endl;
		print_ch(p.GetDiagnostics());
	} else {
		std::cout << "  bug averted" << std::endl << "  ";
		print_ch(m[0]);
		std::cout << std::endl;
	}

	p.SetBuffer("abc =   123");
	if (p.Match("%S=%S", m) < 0) {
		std::cout << "  this is a bug" << std::endl;
		print_ch(p.GetDiagnostics());
	} else {
		std::cout << "  bug averted: \"";
		print_ch(m[0]);
		std::cout << "\" \"";
		print_ch(m[1]);
		std::cout << "\"" << std::endl;
	}

	std::cout << "done" << std::endl;
	return 0;
}

int main(int, char**)
{
	OutputSIMDInfo();
	//return SplitTest();
	//return PathTest();
	return CppTranslatorTest();
	//return CodeCorrectnessTest();
	//return CodeLoopTest();
	//return CodePerformanceTest();
	//return ParserTest();
}
