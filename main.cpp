#include <iostream>

#include <omp.h>
#include <arm_neon.h>

#include <SDL/SDL.h>

#include "MiniLib/MGL/mglText.h"
#include "MiniLib/MML/mmlMatrix.h"

#include "swsl.h"
#include "parser.h"

// Things I should look into:
// Buffers should allocate an extra register at the edges so that screen resolutions that are not multiples of SWSL_WIDTH render properly

#define video SDL_GetVideoSurface()

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
	line_height = mmlMax2(line_height, size);
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

/*int main(int, char**)
{
	std::cout << "Cores: " << omp_get_num_procs() << std::endl;
	#pragma omp parallel
	{
		std::cout << omp_get_thread_num();
	}
	std::cout << std::endl;

	if (SDL_Init(SDL_INIT_VIDEO) == -1) {
		std::cout << SDL_GetError() << std::endl;
		return 1;
	}
	atexit(SDL_Quit);
	if (SDL_SetVideoMode(512, 512, 24, SDL_SWSURFACE|SDL_DOUBLEBUF) == NULL) {
		std::cout << SDL_GetError() << std::endl;
		return 1;
	}

	mtlString file_contents;
	mtlParser parser;
	if (!parser.BufferFile("../shader.swsl", file_contents)) {
		std::cout << "File not found." << std::endl;
		return 1;
	}

	mtlArray<char> program;
	swsl::Compiler compiler;
	if (!compiler.Compile(file_contents, swsl::Compiler::SWSL, swsl::Compiler::ASM, program)) {
		std::cout << "Failed to compile SWSL to ASM." << std::endl;
		const mtlItem<mtlString> *err = compiler.GetErrors();
		while (err != NULL) {
			std::cout << "  ";
			for (int i = 0; i < err->GetItem().GetSize(); ++i) {
				std::cout << err->GetItem().GetChars()[i];
			}
			std::cout << std::endl;
			err = err->GetNext();
		}
		return 1;
	}

	file_contents.Copy(mtlChars::FromDynamic(program, program.GetSize()));
	if (!compiler.Compile(file_contents, swsl::Compiler::ASM, swsl::Compiler::BYTE_CODE, program)) {
		std::cout << "Failed to compile ASM to BYTE_CODE." << std::endl;
		const mtlItem<mtlString> *err = compiler.GetErrors();
		while (err != NULL) {
			std::cout << "  ";
			for (int i = 0; i < err->GetItem().GetSize(); ++i) {
				std::cout << err->GetItem().GetChars()[i];
			}
			std::cout << std::endl;
			err = err->GetNext();
		}
		return 1;
	}

	swsl::Rasterizer rasterizer;
	if (!rasterizer.SetShaderProgram(program)) {
		std::cout << "Failed to set program" << std::endl;
		return 1;
	}
	rasterizer.CreateBuffers(video->w, video->h);
	rasterizer.ClearBuffers();

	mmlVector<2> a_pos = mmlVector<2>(   0.0f, -111.0f);
	mmlVector<2> b_pos = mmlVector<2>( 128.0f,  111.0f);
	mmlVector<2> c_pos = mmlVector<2>(-128.0f,  111.0f);

	float time = 0.0f;

	Printer print;
	print.SetSize(3);

	SDL_Event event;
	bool quit = false;
	while (!quit) {

		Uint32 frame_start = SDL_GetTicks();

		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT: quit = true; break;
			}
		}

		Vertex<3> a, b, c;

		mmlMatrix<2,2> rmat = mml2DRotationMatrix(time);
		mmlVector<2> at = a_pos * rmat;
		mmlVector<2> bt = b_pos * rmat;
		mmlVector<2> ct = c_pos * rmat;

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

		Uint32 render_start = SDL_GetTicks();
		rasterizer.FillTriangle(a.coord, b.coord, c.coord, a.attributes, b.attributes, c.attributes);
		Uint32 render_end = SDL_GetTicks();
		Uint32 render_time = render_end - render_start;

		Uint32 convert_start = SDL_GetTicks();
		rasterizer.WriteColorBuffer((mtlByte*)video->pixels, video->format->BytesPerPixel, mglVideoByteOrder());
		float clear_color[] = { 1.0f, 0.0f, 1.0f, 0.0f };
		rasterizer.ClearBuffers(clear_color);
		Uint32 convert_end = SDL_GetTicks();
		Uint32 convert_time = convert_end - convert_start;

		print.ResetCaret();
		print.Print("Render:  ");
		print.Print((int)render_time);
		print.Print(" ms");
		print.Newline();
		print.Print("Convert: ");
		print.Print((int)convert_time);
		print.Print(" ms");

		SDL_Flip(video);

		Uint32 frame_end = SDL_GetTicks();
		time += (float)(frame_end - frame_start) / 1000.0f;
	}

	SDL_Quit();

	return 0;
}*/

#include "compiler.h"

int main(int, char**)
{
	Compiler compiler;
	Shader shader;

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

	mtlString file;
	if (!mtlParser::BufferFile("../shader.swsl", file)) {
		Printer p;
		p.SetColor(255, 0, 0);
		p.Print("Could not open file ../shader.swsl");
	} else {
		if (!compiler.Compile(file, shader)) {
			Printer p;
			const mtlItem<CompilerMessage> *msg = shader.GetErrors();
			p.SetColor(255, 0, 0);
			while (msg != NULL) {
				p.Print(msg->GetItem().msg);
				p.Print(": ");
				p.Print(msg->GetItem().ref);
				p.Newline();
				msg = msg->GetNext();
			}
			p.SetColor(0, 255, 255);
			msg = shader.GetWarnings();
			while (msg != NULL) {
				p.Print(msg->GetItem().msg);
				p.Print(": ");
				p.Print(msg->GetItem().ref);
				p.Newline();
				msg = msg->GetNext();
			}
		} else {
			Printer p;
			p.SetColor(0, 255, 0);
			p.Print("Success");
		}
	}

	SDL_Flip(video);

	SDL_Event event;
	while (SDL_WaitEvent(&event) && event.type != SDL_QUIT) {}

	SDL_Quit();

	return 0;
}
