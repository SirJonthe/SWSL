#ifndef SWSL_GFX_H
#define SWSL_GFX_H

#include <omp.h>

#include "swsl_wide.h"
#include "swsl_buffers.h"
#include "swsl_vmach.h"

#include "MiniLib/MML/mmlVector.h"
#include "MiniLib/MML/mmlMath.h"
#include "MiniLib/MTL/mtlBits.h"
#include "MiniLib/MGL/mglPixel.h"

namespace swsl
{
	struct Point2D
	{
		int x;
		int y;
	};

	// A suggested implementation of a rasterizer.
	class Rasterizer
	{
	private:
		typedef swsl::wide_cmpmask gfx_cmpmask;
		typedef swsl::wide_float   gfx_float;
		typedef swsl::wide_int     gfx_int;

		struct wide_Point2D
		{
			gfx_float x;
			gfx_float y;
		};

	private:
		swsl::ShaderProgram m_shader; // only temp until we compile programs natively
		swsl::FrameBuffer   m_out_buffer; // RGB + depth
		int                 m_width;
		int                 m_height;

	private:
		bool      IsTopLeft(const swsl::Point2D &a, const swsl::Point2D &b) const;
		int       Orient2D(const swsl::Point2D &a, const swsl::Point2D &b, const swsl::Point2D &c) const;
		gfx_float Orient2D(const swsl::Point2D &a, const swsl::Point2D &b, const wide_Point2D &c) const;

	public:
		Rasterizer( void );

		bool SetShaderProgram(const mtlArray<char> &program);
		void CreateBuffers(int width, int height, int components = 4);
		void ClearBuffers( void );
		void ClearBuffers(const float *component_data);
		void WriteColorBuffer(int src_r_idx, int src_g_idx, int src_b_idx, mtlByte *dst_pixels, int dst_bytes_per_pixel, mglByteOrder32 dst_byte_order);
		void WriteColorBuffer(mtlByte *dst_pixels, int dst_bytes_per_pixel, mglByteOrder32 dst_byte_order);

		/*template < int n >
		void DrawPoint(const swsl::Point2D &a, const mmlVector<n> &a_attr);

		template < int n >
		void DrawLine(const swsl::Point2D &a, const swsl::Point2D &b, const mmlVector<n> &aa, const mmlVector<n> &ab, const mmlVector<n> &ba, const mmlVector<n> &bb);

		template < int n >
		void FillArc(int quadrant, const swsl::Point2D &a, const swsl::Point2D &b, const mmlVector<n> &aa, const mmlVector<n> &ab, const mmlVector<n> &ba, const mmlVector<n> &bb);

		template < int n >
		void FillRectangle(const swsl::Point2D &a, const swsl::Point2D &b, const mmlVector<n> &aa, const mmlVector<n> &ab, const mmlVector<n> &ba, const mmlVector<n> &bb);*/

		template < int var, int cnst >
		void FillTriangle(const swsl::Point2D &a, const swsl::Point2D &b, const swsl::Point2D &c, const mmlVector<var> &a_attr, const mmlVector<var> &b_attr, const mmlVector<var> &c_attr, const mmlVector<cnst> &const_attr);

		template < int var >
		void FillTriangle(const swsl::Point2D &a, const swsl::Point2D &b, const swsl::Point2D &c, const mmlVector<var> &a_attr, const mmlVector<var> &b_attr, const mmlVector<var> &c_attr);

		template < int cnst >
		void FillTriangle(const swsl::Point2D &a, const swsl::Point2D &b, const swsl::Point2D &c, const mmlVector<cnst> &const_attr);

		void FillTriangle(const swsl::Point2D &a, const swsl::Point2D &b, const swsl::Point2D &c);
	};
}

template < int var, int cnst >
void swsl::Rasterizer::FillTriangle(const swsl::Point2D &a, const swsl::Point2D &b, const swsl::Point2D &c, const mmlVector<var> &a_attr, const mmlVector<var> &b_attr, const mmlVector<var> &c_attr, const mmlVector<cnst> &const_attr)
{
	// TODO
	// 3) Perspective correction

	// ISSUES
	// Interpolated values seem to overflow at the edges

	gfx_float varying_arr[var];
	gfx_float constants_arr[cnst];
	swsl::ShaderInput shader_input = {
		{ constants_arr, cnst },            // constant register
		{ varying_arr, var },               // varying register
		{ NULL, m_out_buffer.GetYawSize() } // fragment register
	};
	m_shader.SetInputRegisters(shader_input);
	if (!m_shader.Verify()) { return; }

	// Copy constant data to register
	for (int i = 0; i < cnst; ++i) {
		constants_arr[i] = const_attr[i];
	}

	// Pump data for X number of pixels into these
	gfx_float a_reg[var];
	gfx_float b_reg[var];
	gfx_float c_reg[var];
	for (int i = 0; i < var; ++i) {
		a_reg[i] = a_attr[i];
		b_reg[i] = b_attr[i];
		c_reg[i] = c_attr[i];
	}

	// AABB Clipping
	const int min_y = mmlMax2(mmlMin3(a.y, b.y, c.y), 0);
	const int max_y = mmlMin2(mmlMax3(a.y, b.y, c.y), m_height - 1);
	const int min_x = mmlMax2(mmlMin3(a.x, b.x, c.x) & SWSL_WIDTH_INVMASK, 0); // Make sure this is snapped to a block boundry
	const int max_x = mmlMin2(mmlMax3(a.x, b.x, c.x), m_width - 1);

	// Triangle setup
	const gfx_float A01 = (float)((a.y - b.y) * SWSL_WIDTH);
	const gfx_float B01 = (float)(b.x - a.x);
	const gfx_float A12 = (float)((b.y - c.y) * SWSL_WIDTH);
	const gfx_float B12 = (float)(c.x - b.x);
	const gfx_float A20 = (float)((c.y - a.y) * SWSL_WIDTH);
	const gfx_float B20 = (float)(a.x - c.x);

	const gfx_float bias0 = IsTopLeft(b, c) ? 0.0f : -1.0f;
	const gfx_float bias1 = IsTopLeft(c, a) ? 0.0f : -1.0f;
	const gfx_float bias2 = IsTopLeft(a, b) ? 0.0f : -1.0f;

	const float x_offset[] = SWSL_OFFSETS;
	// float x_offset[] = SWSL_X_OFFSETS;
	// float y_offset[] = SWSL_Y_OFFSETS;

	wide_Point2D p = { gfx_float(min_x) + gfx_float(x_offset), min_y };
	gfx_float w0_row = Orient2D(b, c, p) + bias0;
	gfx_float w1_row = Orient2D(c, a, p) + bias1;
	gfx_float w2_row = Orient2D(a, b, p) + bias2;
	const gfx_float sum_inv_area_x2 = (gfx_float)(1.0f) / (w0_row + w1_row + w2_row);

	gfx_float *pixel_offset = (gfx_float*)m_out_buffer.GetComponent(min_x / SWSL_WIDTH, min_y, 0);
	const int  pixel_y_stride = m_out_buffer.GetPitchSize();

	for (int y = min_y; y <= max_y; ++y) {

		gfx_float w0 = w0_row;
		gfx_float w1 = w1_row;
		gfx_float w2 = w2_row;

		shader_input.fragments.data = pixel_offset;
		for (int x = min_x; x <= max_x; x += SWSL_WIDTH) {

			swsl::wide_cmpmask fragment_mask = (w0 | w1 | w2) >= 0.0f;

			if (!fragment_mask.all_fail()) {

				for (int i = 0; i < var; ++i) {
					varying_arr[i] = (a_reg[i] * w0 + b_reg[i] * w1 + c_reg[i] * w2) * sum_inv_area_x2;
				}

				m_shader.ExecProgram(fragment_mask);
			}

			w0 += A12;
			w1 += A20;
			w2 += A01;

			shader_input.fragments.data += shader_input.fragments.count;
		}

		w0_row += B12;
		w1_row += B20;
		w2_row += B01;

		pixel_offset += pixel_y_stride;
	}
}

template < int var >
void swsl::Rasterizer::FillTriangle(const swsl::Point2D &a, const swsl::Point2D &b, const swsl::Point2D &c, const mmlVector<var> &a_attr, const mmlVector<var> &b_attr, const mmlVector<var> &c_attr)
{
	mmlVector<0> const_attr;
	FillTriangle(a, b, c, a_attr, b_attr, c_attr, const_attr);
}

template < int cnst >
void swsl::Rasterizer::FillTriangle(const swsl::Point2D &a, const swsl::Point2D &b, const swsl::Point2D &c, const mmlVector<cnst> &const_attr)
{
	mmlVector<0> a_attr, b_attr, c_attr;
	FillTriangle(a, b, c, a_attr, b_attr, c_attr, const_attr);
}

#endif // SWSL_GFX_H
