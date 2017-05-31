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

int CodeLoopTest( void )
{
	std::cout << "testing iteration in loops..." << std::endl;
	int xs[] = { 0, 1, 2, 3 };
	mpl::wide_int x = mpl::wide_int(xs);
	int lims[] = { 2, 5, 2, 10 };
	mpl::wide_int lim = mpl::wide_int(lims);
	wide_iterate_to(x, lim, mpl::wide_bool(true));
	std::cout << "  ";
	for (int i = 0; i < MPL_WIDTH; ++i) {
		std::cout << ((int*)&x)[i] << " ";
	}
	std::cout << std::endl;

	std::cout << "done" << std::endl;
	return 0;
}

inline void scalar_mat_mult(
		  float &x,         float &y,         float &z,
	const float &m00, const float &m10, const float &m20,
	const float &m01, const float &m11, const float &m21,
	const float &m02, const float &m12, const float &m22)
{
	float a = x*m00 + y*m10 + z*m20;
	float b = x*m01 + y*m11 + z*m21;
	float c = x*m02 + y*m12 + z*m22;
	x = a;
	y = b;
	z = c;
}

int CodePerformanceTest( void )
{
	std::cout << "testing performance (matrix mult)..." << std::endl;

	const size_t size = MPL_WIDTH*1000000;
	const size_t byte_size = size * sizeof(float);
	const size_t total_size = byte_size * 12;

	srand(time(0));

	time_t start, end;
	mpl::aligned mem;
	mem.alloc(total_size);

	float *x   = mem.ptr<float>() + size*0;
	float *y   = mem.ptr<float>() + size*1;
	float *z   = mem.ptr<float>() + size*2;
	float *m00 = mem.ptr<float>() + size*3, *m10 = mem.ptr<float>() + size* 4, *m20 = mem.ptr<float>() + size* 5,
		  *m01 = mem.ptr<float>() + size*6, *m11 = mem.ptr<float>() + size* 7, *m21 = mem.ptr<float>() + size* 8,
		  *m02 = mem.ptr<float>() + size*9, *m12 = mem.ptr<float>() + size*10, *m22 = mem.ptr<float>() + size*11;
	for (size_t i = 0; i < size; ++i) {
		x[i]   = rand();
		y[i]   = rand();
		z[i]   = rand();
		m00[i] = rand();
		m10[i] = rand();
		m20[i] = rand();
		m01[i] = rand();
		m11[i] = rand();
		m21[i] = rand();
		m02[i] = rand();
		m12[i] = rand();
		m22[i] = rand();
	}

	start = clock();
	for (size_t i = 0; i < size; ++i) {
		scalar_mat_mult(x[i], y[i], z[i],
						m00[i], m10[i], m20[i],
						m01[i], m11[i], m21[i],
						m02[i], m12[i], m22[i]
		);
	}
	end = clock();
	std::cout << "  scalar finished in " << end - start << " clocks" << std::endl;

	start = clock();
	for (size_t i = 0; i < size; i+=MPL_WIDTH) {
		wide_mat_mult(*((mpl::wide_float*)(x + i)), *((mpl::wide_float*)(y + i)), *((mpl::wide_float*)(z + i)),
					  *((mpl::wide_float*)(m00 + i)), *((mpl::wide_float*)(m10 + i)), *((mpl::wide_float*)(m20 + i)),
					  *((mpl::wide_float*)(m01 + i)), *((mpl::wide_float*)(m11 + i)), *((mpl::wide_float*)(m21 + i)),
					  *((mpl::wide_float*)(m02 + i)), *((mpl::wide_float*)(m12 + i)), *((mpl::wide_float*)(m22 + i))
		);
	}
	end = clock();
	std::cout << "  wide1 finished in  " << end - start << " clocks" << std::endl;

#if MPL_SIMD == MPL_SIMD_SSE
	start = clock();
	for (size_t i = 0; i < size; i+=MPL_WIDTH) {
		wide_mat_mult_raw(*((__m128*)(x + i)), *((__m128*)(y + i)), *((__m128*)(z + i)),
						  *((__m128*)(m00 + i)), *((__m128*)(m10 + i)), *((__m128*)(m20 + i)),
						  *((__m128*)(m01 + i)), *((__m128*)(m11 + i)), *((__m128*)(m21 + i)),
						  *((__m128*)(m02 + i)), *((__m128*)(m12 + i)), *((__m128*)(m22 + i))
		);
	}
	end = clock();
	std::cout << "  wide2 finished in  " << end - start << " clocks" << std::endl;
#endif

	mem.free();

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
