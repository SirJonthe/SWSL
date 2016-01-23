#include <omp.h>

#include "gfx.h"

bool Rasterizer::IsTopLeft(const Point2D &a, const Point2D &b) const
{
	// strictly connected to winding order
	return (a.x < b.x && b.y == a.y) || (a.y > b.y);
}

int Rasterizer::Orient2D(const Point2D &a, const Point2D &b, const Point2D &c) const
{
	return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

Rasterizer::gfx_float Rasterizer::Orient2D(const Point2D &a, const Point2D &b, const Rasterizer::wide_Point2D &c) const
{
	wide_Point2D A = { a.x, a.y }, B = { b.x, b.y };
	return (B.x - A.x) * (c.y - A.y) - (B.y - A.y) * (c.x - A.x);
}

Rasterizer::Rasterizer( void ) : m_width(0), m_height(0) {}

bool Rasterizer::SetShaderProgram(const mtlArray<char> &program)
{
	return m_shader.LoadProgram(program);
}

void Rasterizer::CreateBuffers(int width, int height, int components)
{
	m_width = width;
	m_height = height;
	m_out_buffer.Create(width, height, components); // RGB + depth = 4 components
}

void Rasterizer::ClearBuffers( void )
{
	mtlClear(m_out_buffer.GetComponent(0, 0, 0), m_out_buffer.GetTotalComponentCount());
}

void Rasterizer::ClearBuffers(const float *component_data)
{
#ifndef _OPENMP
	int        buffer_area   = m_out_buffer.GetWidth() * m_out_buffer.GetHeight();
	int        buffer_stride = m_out_buffer.GetYawSize();
	gfx_float *buffer_data   = m_out_buffer.GetComponent(0,0,0);
	for (int i = 0; i < buffer_area; ++i) {
		for (int n = 0; n < buffer_stride; ++n) {
			*buffer_data = component_data[n];
			++buffer_data;
		}
	}
#else
	// split into a series of scanlines per core
	// TODO: I have to be able to handle vertical resolutions that results in remainders when dividing by thread count
	const int width = m_out_buffer.GetWidth();
	const int x_stride = m_out_buffer.GetYawSize();
	#pragma omp parallel
	{
		const int y_count = m_out_buffer.GetHeight() / omp_get_num_threads(); // need to deal with remainders
		const int count = width * y_count;
		gfx_float *buffer_data = m_out_buffer.GetComponent(0, y_count * omp_get_thread_num(), 0);
		for (int i = 0; i < count; ++i) {
			for (int n = 0; n < x_stride; ++n) {
				*buffer_data = component_data[n];
				++buffer_data;
			}
		}
	}
#endif
}

void Rasterizer::WriteColorBuffer(int src_r_idx, int src_g_idx, int src_b_idx, mtlByte *dst_pixels, int dst_bytes_per_pixel, mglByteOrder32 dst_byte_order)
{
#ifndef _OPENMP
	int        buffer_area   = m_out_buffer.GetWidth() * m_out_buffer.GetHeight();
	int        buffer_stride = m_out_buffer.GetYawSize();
	gfx_float *buffer_data   = m_out_buffer.GetComponent(0,0,0);

	gfx_float scale = 255.0f;
	gfx_float rf, gf, bf;
	gfx_int   ri, gi, bi;
	int       rs[SWSL_WIDTH], gs[SWSL_WIDTH], bs[SWSL_WIDTH];
	for (int i = 0; i < buffer_area; ++i) {
		rf = *(buffer_data + src_r_idx) * scale;
		gf = *(buffer_data + src_g_idx) * scale;
		bf = *(buffer_data + src_b_idx) * scale;
		ri = gfx_int(rf);
		gi = gfx_int(gf);
		bi = gfx_int(bf);
		ri.to_scalar(rs);
		gi.to_scalar(gs);
		bi.to_scalar(bs);
		for (int n = 0; n < SWSL_WIDTH; ++n) {
			dst_pixels[dst_byte_order.index.r] = rs[n];
			dst_pixels[dst_byte_order.index.g] = gs[n];
			dst_pixels[dst_byte_order.index.b] = bs[n];
			dst_pixels += dst_bytes_per_pixel;
		}
		buffer_data += buffer_stride;
	}
#else
	// split into a series of scanlines per core
	// TODO: I have to be able to handle vertical resolutions that results in remainders when dividing by thread count
	#pragma parallel omp
	{
		const int buffer_area = m_out_buffer.GetWidth() * (m_out_buffer.GetHeight() / omp_get_num_threads());
		const int pixel_stride = m_out_buffer.GetPixelStride();
		gfx_float *buffer_data = m_out_buffer.GetComponent(0, buffer_area * omp_get_thread_num(), 0);

		gfx_float scale = 255.0f;
		gfx_float rf, gf, bf;
		gfx_int   ri, gi, bi;
		int       rs[SWSL_WIDTH], gs[SWSL_WIDTH], bs[SWSL_WIDTH];
		for (int i = 0; i < buffer_area; ++i) {
			rf = *(buffer_data + src_r_idx) * scale;
			gf = *(buffer_data + src_g_idx) * scale;
			bf = *(buffer_data + src_b_idx) * scale;
			ri = gfx_int(rf);
			gi = gfx_int(gf);
			bi = gfx_int(bf);
			ri.to_scalar(rs);
			gi.to_scalar(gs);
			bi.to_scalar(bs);
			for (int n = 0; n < SWSL_WIDTH; ++n) {
				dst_pixels[dst_byte_order.index.r] = rs[n];
				dst_pixels[dst_byte_order.index.g] = gs[n];
				dst_pixels[dst_byte_order.index.b] = bs[n];
				dst_pixels += dst_bytes_per_pixel;
			}
			buffer_data += pixel_stride;
		}
	}
#endif
}

void Rasterizer::WriteColorBuffer(mtlByte *dst_pixels, int dst_bytes_per_pixel, mglByteOrder32 dst_byte_order)
{
	WriteColorBuffer(0, 1, 2, dst_pixels, dst_bytes_per_pixel, dst_byte_order);
}

void Rasterizer::FillTriangle(const Point2D &a, const Point2D &b, const Point2D &c)
{
	mmlVector<0> a_attr, b_attr, c_attr, const_attr;
	FillTriangle(a, b, c, a_attr, b_attr, c_attr, const_attr);
}

// Triangle filling algorithm with linear interpolation - Scalar code, reference
/*
void FillTriangle(mtlByte *pixels, int bytes_per_pixel, int width, int height, const swsl::Point2D &a, const swsl::Point2D &b, const swsl::Point2D &c, const mmlVector<3> &a_attr, const mmlVector<3> &b_attr, const mmlVector<3> &c_attr)
{
	// AABB Clipping
	int min_y = mmlMax2(mmlMin3(a.y, b.y, c.y), 0);
	int max_y = mmlMin2(mmlMax3(a.y, b.y, c.y), height - 1);
	int min_x = mmlMax2(mmlMin3(a.x, b.x, c.x), 0);
	int max_x = mmlMin2(mmlMax3(a.x, b.x, c.x), width - 1);

	// Triangle setup
	int A01 = a.y - b.y;
	int B01 = b.x - a.x;
	int A12 = b.y - c.y;
	int B12 = c.x - b.x;
	int A20 = c.y - a.y;
	int B20 = a.x - c.x;

	int bias0 = IsTopLeft(b, c) ? 0 : -1;
	int bias1 = IsTopLeft(c, a) ? 0 : -1;
	int bias2 = IsTopLeft(a, b) ? 0 : -1;

	swsl::Point2D p = { min_x, min_y };
	int w0_row = Orient2D(b, c, p) + bias0;
	int w1_row = Orient2D(c, a, p) + bias1;
	int w2_row = Orient2D(a, b, p) + bias2;
	float sum_inv_area_x2 = 1.0f / (float)(w0_row + w1_row + w2_row);

	mmlVector<3> attr;

	int pitch = width * bytes_per_pixel;
	mtlByte *pixel_offset = pixels + ((min_x + min_y * width) * bytes_per_pixel);
	for (p.y = min_y; p.y <= max_y; ++p.y) { // p.y += SWSL_BLOCK_Y

		int w0 = w0_row;
		int w1 = w1_row;
		int w2 = w2_row;

		mtlByte *pixels = pixel_offset;
		for (p.x = min_x; p.x <= max_x; ++p.x) { // p.x += SWSL_BLOCK_X

			if ((w0 | w1 | w2) >= 0) {

				attr = (a_attr * w0 + b_attr * w1 + c_attr * w2) * sum_inv_area_x2;

				pixels[0] = (mtlByte)(attr[0] * 255.0f);
				pixels[1] = (mtlByte)(attr[1] * 255.0f);
				pixels[2] = (mtlByte)(attr[2] * 255.0f);
			}

			w0 += A12;
			w1 += A20;
			w2 += A01;

			pixels += bytes_per_pixel;
		}

		w0_row += B12;
		w1_row += B20;
		w2_row += B01;

		pixel_offset += pitch;
	}
}
*/

