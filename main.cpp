//#define MPL_FALLBACK_SCALAR

#include <iostream>

#include <omp.h>

#include <SDL/SDL.h>

#include "MiniLib/MTL/mtlParser.h"
#include "MiniLib/MGL/mglText.h"
#include "MiniLib/MML/mmlMatrix.h"

#include "swsl.h"
#include "compiler.h"
#include "swsl_parser.h"
#include "swsl_gfx.h"
#include "swsl_aux.h"

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

#include "swsl_compiler.h"
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

bool LoadShader(const mtlChars &file_name, swsl::Shader &shader)
{
	swsl::Compiler compiler;
	mtlString file;
	if (!mtlSyntaxParser::BufferFile(mtlPath(file_name), file)) {
		std::cout << "Failed to load shader file" << std::endl;
		return false;
	}
	if (!compiler.Compile(file, shader)) {
		std::cout << "Failed to compile shader" << std::endl;
		const mtlItem<swsl::CompilerMessage> *err = shader.GetErrors();
		while (err != NULL) {
			std::cout << "  ";
			swsl::print_ch(err->GetItem().msg);
			swsl::print_ch(": ");
			swsl::print_line(err->GetItem().ref);
			err = err->GetNext();
		}
		return false;
	}
	{
		// Only a test to see if shader is valid
		// Not necessary for function

		mpl::wide_float vary[3] = {  1.0f,  2.0f,  3.0f };
		mpl::wide_float frag[3] = { -1.0f, -2.0f, -3.0f };

		swsl::Shader::InputArrays inputs = {
			{ NULL, 0 },
			{ vary, sizeof(vary)/sizeof(mpl::wide_float) },
			{ frag, sizeof(frag)/sizeof(mpl::wide_float) }
		};
		shader.SetInputArrays(inputs);
		if (!shader.IsValid() || !shader.Run(mpl::wide_float(0.0f) < mpl::wide_float(1.0f))) {
			std::cout << "Failed to execute shader" << std::endl;
			return false;
		}
	}
	{
		swsl::Disassembler disassembler;
		mtlString disassembly;
		disassembler.Disassemble(shader, disassembly);
		swsl::print_line(disassembly);
	}
	return true;
}

int InteractiveDemo( void )
{
	if (SDL_Init(SDL_INIT_VIDEO) == -1) {
		std::cout << SDL_GetError() << std::endl;
		return 1;
	}
	atexit(SDL_Quit);
	if (SDL_SetVideoMode(512, 512, 24, SDL_SWSURFACE|SDL_DOUBLEBUF) == NULL) {
		std::cout << SDL_GetError() << std::endl;
		return 1;
	}
	SDL_WM_SetCaption("SWSL test", NULL);

	const float side_len = 300.0f;
	const float base_len = side_len * sin(mmlPI / 3.0f);

	mmlVector<2> a_pos  = mmlVector<2>(side_len * 0.5f,     0.0f);
	mmlVector<2> b_pos  = mmlVector<2>(side_len,        base_len);
	mmlVector<2> c_pos  = mmlVector<2>(    0.0f,        base_len);
	mmlVector<2> center = (a_pos + b_pos + c_pos) / 3.0f;
	a_pos -= center;
	b_pos -= center;
	c_pos -= center;

	SDL_Event        event;
	swsl::Shader     shader;
	bool             quit = false;
	bool             reload_shader = true;
	bool             shader_status = false;
	swsl::Rasterizer raster;
	Printer          p;

	p.SetColor(0, 255, 0);

	raster.CreateBuffers(video->w, video->h);
	raster.SetShader(&shader);

	while (!quit) {

		Uint32 frame_start = SDL_GetTicks();

		if (reload_shader) {
			reload_shader = false;
			shader_status = LoadShader("../swsl_samples/interactive.swsl", shader);
		}

		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_KEYDOWN:
				if (event.key.keysym.sym == SDLK_r) {
					reload_shader = true;
				}
				break;
			case SDL_QUIT: quit = true; break;
			}
		}

		Vertex<3> a, b, c;

		//mmlMatrix<2,2> rmat = mml2DRotationMatrix((float)SDL_GetTicks() / 1000.0f);
		mmlMatrix<2,2> rmat = mmlMatrix<2,2>::IdentityMatrix();
		mmlVector<2>   at   = a_pos * rmat;
		mmlVector<2>   bt   = b_pos * rmat;
		mmlVector<2>   ct   = c_pos * rmat;

		a.coord.x = round(at[0]) + video->w / 2;
		a.coord.y = round(at[1]) + video->h / 2;
		b.coord.x = round(bt[0]) + video->w / 2;
		b.coord.y = round(bt[1]) + video->h / 2;
		c.coord.x = round(ct[0]) + video->w / 2;
		c.coord.y = round(ct[1]) + video->h / 2;

		a.attributes[0] = 1.0f;
		a.attributes[1] = 0.0f;
		a.attributes[2] = 0.0f;
		b.attributes[0] = 0.0f;
		b.attributes[1] = 1.0f;
		b.attributes[2] = 0.0f;
		c.attributes[0] = 0.0f;
		c.attributes[1] = 0.0f;
		c.attributes[2] = 1.0f;

		//float rgb[] = { 1.0f, 1.0f, 1.0f };
		//raster.ClearBuffers(rgb);
		raster.ClearBuffers();

		Uint32 render_start = SDL_GetTicks();
		raster.FillTriangle(a.coord, b.coord, c.coord, a.attributes, b.attributes, c.attributes);
		Uint32 render_end = SDL_GetTicks();
		Uint32 render_time = render_end - render_start;

		raster.WriteColorBuffer((mtlByte*)video->pixels, video->format->BytesPerPixel, ByteOrder());

		if (!shader_status) {
			p.SetColor(255, 0, 0);
			p.Print("Shader execution failed");
			p.Newline();
		} else {
			p.SetColor(0, 255, 0);
		}
		p.Print("Press \'r\' to reload shader");
		p.Newline();
		p.Print("Render time: ");

		p.Print((int)render_time);
		p.Print("/");
		Uint32 frame_end = SDL_GetTicks();
		Uint32 frame_time = frame_end - frame_start;
		p.Print((int)frame_time);
		p.Print(" ms");
		p.ResetCaret();

		SDL_Flip(video);
		if ((int)(25 - render_time) > 0) {
			SDL_Delay(25 - render_time);
		}

	}

	SDL_Quit();

	return 0;
}

int ParserTest( void )
{
	mtlString file_buffer;
	mtlPath file_path("../test_file.txt");
	swsl::print_ch("path=");
	swsl::print_line(file_path.GetPath());
	if (!mtlSyntaxParser::BufferFile(file_path, file_buffer)) { return 1; }
	mtlSyntaxParser parser;
	parser.SetBuffer(file_buffer);
	mtlArray<mtlChars> m;
	while (!parser.IsEnd()) {
		switch (parser.Match("%w:=%i;%|%w:=%r;%|%w:=\"%s\";%|%w[%i]%w:={%i,%i,%i};%|end;%0%|%s+%s;", m)) {
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
			swsl::print_ch(m[2]);
			swsl::print_ch("\" (type \"");
			swsl::print_ch(m[0]);
			swsl::print_ch("[");
			swsl::print_ch(m[1]);
			swsl::print_ch("]\") set to (");
			swsl::print_ch(m[3]);
			swsl::print_ch(";");
			swsl::print_ch(m[4]);
			swsl::print_ch(";");
			swsl::print_ch(m[5]);
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

int NewCompilerTest( void )
{
	ByteCodeCompiler c;
	swsl::Shader s;

	if (!c.Compile("../swsl_samples/interactive.swsl", s)) {
		const mtlItem<ByteCodeCompiler::Message> *err = c.GetError();
		while (err != NULL) {
			swsl::print_ch(err->GetItem().err);
			swsl::print_ch(": ");
			swsl::print_line(err->GetItem().msg);
			err = err->GetNext();
		}
		return 1;
	}

	mtlString output;
	swsl::Disassembler dasm;
	dasm.Disassemble(s, output);

	swsl::print_ch(output);

	return 0;
}

#include <fstream>

int CppCompilerTest( void )
{
	std::ofstream fout;
	fout.open("../swsl_samples/out.h");
	if (!fout.is_open()) {
		swsl::print_line("could not open/create out.h");
		return 1;
	}

	CppCompiler c;
	swsl::Binary bin;

	if (!c.Compile(mtlPath("../swsl_samples/interactive.swsl"), bin, "test_shader")) {
		const mtlItem<Compiler::Message> *err = c.GetError();
		while (err != NULL) {
			swsl::print_ch(err->GetItem().err);
			swsl::print_ch(": ");
			swsl::print_line(err->GetItem().msg);
			err = err->GetNext();
		}
		return 1;
	}

	fout.write(bin.GetChars(), bin.GetSize());

	return 0;
}

int main(int, char**)
{
	OutputSIMDInfo();
	//return InteractiveDemo();
	//return ParserTest();
	//return SplitTest();
	//return PathTest();
	//return NewCompilerTest();
	return CppCompilerTest();
}
