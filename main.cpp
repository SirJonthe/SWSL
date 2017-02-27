//#define MPL_FALLBACK_SCALAR

#include <iostream>
#include <fstream>
#include <omp.h>

#include <SDL/SDL.h>

#include "MiniLib/MTL/mtlParser.h"
#include "MiniLib/MGL/mglText.h"
#include "MiniLib/MML/mmlMatrix.h"

#include "swsl.h"
#include "swsl_gfx.h"
#include "swsl_aux.h"
#include "swsl_astgen.h"
#include "swsl_cppcompiler.h"

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
#include <pthread.h>

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
				"AVX512
			 #elif MPL_SIMD == MPL_SIMD_NEON
				 "NEON"
			 #endif
				 << " @ " << MPL_WIDTH << " wide" << std::endl;
}

int ParserTest( void )
{
	mtlString file_buffer;
	mtlPath file_path("../swsl_samples/test_file.txt");
	swsl::print_ch("path=");
	swsl::print_line(file_path.GetPath());
	if (!mtlSyntaxParser::BufferFile(file_path, file_buffer)) {
		std::cout << "failed to open specified file" << std::endl;
		return 1;
	}
	mtlSyntaxParser parser;
	parser.SetBuffer(file_buffer);
	mtlArray<mtlChars> m;
	while (!parser.IsEnd()) {
		switch (parser.Match("%w:=%i;%|%w:=%r;%|%w:=\"%s\";%|%w[%i]:={%i,%i,%i};%|end;%0%|%s+%s;", m)) {
		case 0:
			swsl::print_ch("\"");
			swsl::print_ch(m[0]);
			swsl::print_ch("\" set to int \"");
			swsl::print_ch(m[1]);
			swsl::print_line("\"");
			break;
		case 1:
			swsl::print_ch("\"");
			swsl::print_ch(m[0]);
			swsl::print_ch("\" set to real \"");
			swsl::print_ch(m[1]);
			swsl::print_line("\"");
			break;
		case 2:
			swsl::print_ch("\"");
			swsl::print_ch(m[0]);
			swsl::print_ch("\" set to str \"");
			swsl::print_ch(m[1]);
			swsl::print_line("\"");
			break;
		case 3:
			swsl::print_ch("\"");
			swsl::print_ch(m[0]);
			swsl::print_ch("[");
			swsl::print_ch(m[1]);
			swsl::print_ch("]\" set to arr (");
			swsl::print_ch(m[2]);
			swsl::print_ch(";");
			swsl::print_ch(m[3]);
			swsl::print_ch(";");
			swsl::print_ch(m[4]);
			swsl::print_line(")");
			break;
		case 4:
			swsl::print_line("Goodbye");
			break;
		case 5:
			swsl::print_ch("term \"");
			swsl::print_ch(m[0]);
			swsl::print_ch("\" plus term \"");
			swsl::print_ch(m[1]);
			swsl::print_line("\"");
			break;
		default:
			swsl::print_line("Error");
			return 1;
		}
	}
	return 0;
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

int CppCompilerTest( void )
{
	swsl::SyntaxTreeGenerator gen;
	std::cout << "Generating tree..." << std::flush;
	swsl::SyntaxTree *t = gen.Generate("../swsl_samples/test.swsl");
	std::cout << "done" << std::endl;
	swsl::CppCompiler c;
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

#include "tmp_out.h"
int CppGenTest( void )
{
	mpl::wide_float f[10];
	//wide_main(f, MPL_TRUE);
}

int main(int, char**)
{
	OutputSIMDInfo();
	//return SplitTest();
	//return PathTest();
	return CppCompilerTest();
}
