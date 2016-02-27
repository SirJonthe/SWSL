#include <iostream>

#include <omp.h>

#include <SDL/SDL.h>

#include "MiniLib/MTL/mtlParser.h"
#include "MiniLib/MGL/mglText.h"
#include "MiniLib/MML/mmlMatrix.h"

#include "swsl.h"
#include "parser.h"
#include "swsl_gfx.h"

// Things I should look into:
// Buffers should allocate an extra register at the edges so that screen resolutions that are not multiples of SWSL_WIDTH render properly

#define video SDL_GetVideoSurface()

void print_chars(mtlChars s)
{
	for (int i = 0; i < s.GetSize(); ++i) {
		std::cout << s.GetChars()[i];
	}
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
			mglDrawCharSmall(ch, (mtlByte*)video->pixels, video->format->BytesPerPixel, mglVideoByteOrder(), video->w, video->h, x, y, r, g, b, size);
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

bool LoadShader(const mtlChars &file_name, swsl::Shader &shader)
{
	swsl::Compiler compiler;
	mtlString file;
	if (!mtlParser::BufferFile(file_name, file)) {
		std::cout << "Failed to load shader file" << std::endl;
		return false;
	}
	if (!compiler.Compile(file, shader)) {
		std::cout << "Failed to compile shader" << std::endl;
		const mtlItem<swsl::CompilerMessage> *err = shader.GetErrors();
		while (err != NULL) {
			std::cout << "  ";
			print_chars(err->GetItem().msg);
			std::cout << ": ";
			print_chars(err->GetItem().ref);
			std::cout << std::endl;
			err = err->GetNext();
		}
		return false;
	}
	{
		swsl::wide_float vary[3] = {  1.0f,  2.0f,  3.0f };
		swsl::wide_float frag[4] = { -1.0f, -2.0f, -3.0f, -4.0f };

		swsl::Shader::InputArrays inputs = {
			{ NULL, 0 },
			{ vary, sizeof(vary)/sizeof(swsl::wide_float) },
			{ frag, sizeof(frag)/sizeof(swsl::wide_float) }
		};
		shader.SetInputArrays(inputs);
		if (!shader.IsValid() || !shader.Run(swsl::wide_float(0.0f) < swsl::wide_float(1.0f))) {
			std::cout << "Failed to execute shader" << std::endl;
			return false;
		}

		for (int i = 0; i < sizeof(frag)/sizeof(swsl::wide_float); ++i) {
			float frag_comp[SWSL_WIDTH];
			frag[i].to_scalar(frag_comp);
			std::cout << "(";
			for (int j = 0; j < SWSL_WIDTH; ++j) {
				std::cout << frag_comp[j] << ";";
			}
			std::cout << ") ";
		}
		std::cout << std::endl;
	}
	{
		swsl::Disassembler disassembler;
		mtlString disassembly;
		disassembler.Disassemble(shader, disassembly);
		print_chars(disassembly);
		std::cout << std::endl;
	}
	return true;
}

int main(int, char**)
{
	std::cout << sizeof(char*) * CHAR_BIT << " bit system" << std::endl;
	std::cout << "SIMD width: " << SWSL_WIDTH << std::endl;

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

		mmlMatrix<2,2> rmat = mml2DRotationMatrix((float)SDL_GetTicks() / 1000.0f);
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

		raster.ClearBuffers();

		Uint32 render_start = SDL_GetTicks();
		raster.FillTriangle(a.coord, b.coord, c.coord, a.attributes, b.attributes, c.attributes);
		Uint32 render_end = SDL_GetTicks();
		Uint32 render_time = render_end - render_start;

		raster.WriteColorBuffer((mtlByte*)video->pixels, video->format->BytesPerPixel, mglVideoByteOrder());

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
