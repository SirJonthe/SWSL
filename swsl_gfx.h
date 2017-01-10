#ifndef SWSL_GFX_H_INCLUDED__
#define SWSL_GFX_H_INCLUDED__

#include "swsl_buffers.h"
#include "swsl_shader.h"

#include "MiniLib/MPL/mplWide.h"
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
		typedef mpl::wide_bool  gfx_bool;
		typedef mpl::wide_float gfx_float;
		typedef mpl::wide_int   gfx_int;

		struct wide_Point2D
		{
			gfx_float x;
			gfx_float y;
		};

	private:
		swsl::Shader      *m_shader; // only temp until we compile programs natively
		swsl::FrameBuffer  m_out_buffer; // RGB + depth
		int                m_width;
		int                m_height;
		int                m_mask_x1;
		int                m_mask_y1;
		int                m_mask_x2;
		int                m_mask_y2;

	private:
		bool      IsTopLeft(const swsl::Point2D &a, const swsl::Point2D &b) const;
		int       Orient2D(const swsl::Point2D &a, const swsl::Point2D &b, const swsl::Point2D &c) const;
		gfx_float Orient2D(const swsl::Point2D &a, const swsl::Point2D &b, const wide_Point2D &c) const;
		int       GetMaskWidthStride( void ) const;
		int       CeilIndex(int i) const;
		int       FloorIndex(int i) const;

	public:
		Rasterizer( void );

		void SetShader(swsl::Shader *shader);
		void CreateBuffers(int width, int height, int components = 3);
		void SetRasterMask(int x1, int y1, int x2, int y2);
		void ResetRasterMask( void );
		void ClearBuffers( void );
		void ClearBuffers(const float *component_data);
		void WriteColorBuffer(int src_r_idx, int src_g_idx, int src_b_idx, mtlByte *dst_pixels, int dst_bytes_per_pixel, mglByteOrder32 dst_byte_order);
		void WriteColorBuffer(mtlByte *dst_pixels, int dst_bytes_per_pixel, mglByteOrder32 dst_byte_order);

		/*template < int n >
		void DrawPoint(const swsl::swsl::Point2D &a, const mmlVector<n> &a_attr);

		template < int n >
		void DrawLine(const swsl::swsl::Point2D &a, const swsl::swsl::Point2D &b, const mmlVector<n> &aa, const mmlVector<n> &ab, const mmlVector<n> &ba, const mmlVector<n> &bb);

		template < int n >
		void FillArc(int quadrant, const swsl::swsl::Point2D &a, const swsl::swsl::Point2D &b, const mmlVector<n> &aa, const mmlVector<n> &ab, const mmlVector<n> &ba, const mmlVector<n> &bb);

		template < int n >
		void FillRectangle(const swsl::swsl::Point2D &a, const swsl::swsl::Point2D &b, const mmlVector<n> &aa, const mmlVector<n> &ab, const mmlVector<n> &ba, const mmlVector<n> &bb);*/

		template < int var, int cnst >
		void FillTriangle(const swsl::Point2D &a, const swsl::Point2D &b, const swsl::Point2D &c, const mmlVector<var> &a_attr, const mmlVector<var> &b_attr, const mmlVector<var> &c_attr, const mmlVector<cnst> &const_attr);

		template < int var >
		void FillTriangle(const swsl::Point2D &a, const swsl::Point2D &b, const swsl::Point2D &c, const mmlVector<var> &a_attr, const mmlVector<var> &b_attr, const mmlVector<var> &c_attr);

		template < int cnst >
		void FillTriangle(const swsl::Point2D &a, const swsl::Point2D &b, const swsl::Point2D &c, const mmlVector<cnst> &const_attr);

		void FillTriangle(const swsl::Point2D &a, const swsl::Point2D &b, const swsl::Point2D &c);
	};



	// Reference implementation for native rasterizer

	class rasterizer
	{
	private:
		typedef mpl::wide_bool  gfx_bool;
		typedef mpl::wide_float gfx_float;
		typedef mpl::wide_int   gfx_int;

		struct wide_Point2D
		{
			gfx_float x;
			gfx_float y;
		};

	private:
		swsl::FrameBuffer  m_out_buffer; // RGB + depth
		int                m_width;
		int                m_height;
		int                m_mask_x1;
		int                m_mask_y1;
		int                m_mask_x2;
		int                m_mask_y2;

	private:
		bool      is_top_left(const swsl::Point2D &a, const swsl::Point2D &b) const;
		int       orient_2d(const swsl::Point2D &a, const swsl::Point2D &b, const swsl::Point2D &c) const;
		gfx_float orient_2d(const swsl::Point2D &a, const swsl::Point2D &b, const wide_Point2D &c) const;
		int       get_mask_width_stride( void ) const;
		int       ceil_index(int i) const;
		int       floor_index(int i) const;

	public:
		rasterizer( void );

		void create_buffers(int width, int height, int components = 3);
		void set_raster_mask(int x1, int y1, int x2, int y2);
		void reset_raster_mask( void );
		void clear_buffers( void );
		void clear_buffers(const float *component_data);
		void write_color_buffer(int src_r_idx, int src_g_idx, int src_b_idx, mtlByte *dst_pixels, int dst_bytes_per_pixel, mglByteOrder32 dst_byte_order);
		void write_color_buffer(mtlByte *dst_pixels, int dst_bytes_per_pixel, mglByteOrder32 dst_byte_order);

		template < int var, int cnst, typename shader_t >
		void fill_triangle(const swsl::Point2D &a, const swsl::Point2D &b, const swsl::Point2D &c, const mmlVector<var> &a_attr, const mmlVector<var> &b_attr, const mmlVector<var> &c_attr, const mmlVector<cnst> &const_attr, shader_t shader);

		template < int var, typename shader_t >
		void fill_triangle(const swsl::Point2D &a, const swsl::Point2D &b, const swsl::Point2D &c, const mmlVector<var> &a_attr, const mmlVector<var> &b_attr, const mmlVector<var> &c_attr, shader_t shader);

		template < int cnst, typename shader_t >
		void fill_triangle(const swsl::Point2D &a, const swsl::Point2D &b, const swsl::Point2D &c, const mmlVector<cnst> &const_attr, shader_t shader);

		template < typename shader_t >
		void fill_triangle(const swsl::Point2D &a, const swsl::Point2D &b, const swsl::Point2D &c, shader_t shader);
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
	swsl::Shader::InputArrays shader_input = {
		{ constants_arr, cnst },                // constant register
		{ varying_arr, var },                   // varying register
		{ NULL, m_out_buffer.GetPixelStride() } // fragment register
	};
	m_shader->SetInputArrays(shader_input);
	if (!m_shader->IsValid()) { return; }

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
	const int min_y = mmlMax(mmlMin(a.y, b.y, c.y), m_mask_y1);
	const int max_y = mmlMin(mmlMax(a.y, b.y, c.y), m_mask_y2 - 1);
	const int min_x = mmlMax(FloorIndex(mmlMin(a.x, b.x, c.x)), m_mask_x1); // Make sure this is snapped to a block boundry
	const int max_x = mmlMin(mmlMax(a.x, b.x, c.x), m_mask_x2 - 1);

	// Triangle setup
	const gfx_float A01 = (float)((a.y - b.y) * MPL_WIDTH);
	const gfx_float B01 = (float)(b.x - a.x);
	const gfx_float A12 = (float)((b.y - c.y) * MPL_WIDTH);
	const gfx_float B12 = (float)(c.x - b.x);
	const gfx_float A20 = (float)((c.y - a.y) * MPL_WIDTH);
	const gfx_float B20 = (float)(a.x - c.x);

	const gfx_float bias0 = IsTopLeft(b, c) ? 0.0f : -1.0f;
	const gfx_float bias1 = IsTopLeft(c, a) ? 0.0f : -1.0f;
	const gfx_float bias2 = IsTopLeft(a, b) ? 0.0f : -1.0f;

	const float x_offset[] = MPL_OFFSETS;
	// float x_offset[] = MPL_X_OFFSETS;
	// float y_offset[] = MPL_Y_OFFSETS;

	wide_Point2D p = { gfx_float(min_x) + gfx_float(x_offset), min_y };
	gfx_float w0_row = Orient2D(b, c, p) + bias0;
	gfx_float w1_row = Orient2D(c, a, p) + bias1;
	gfx_float w2_row = Orient2D(a, b, p) + bias2;
	const gfx_float sum_inv_area_x2 = (gfx_float)(1.0f) / (w0_row + w1_row + w2_row);

	gfx_float *pixel_offset = (gfx_float*)m_out_buffer.GetComponent(min_x / MPL_WIDTH, min_y, 0);
	const int  pixel_y_stride = m_out_buffer.GetScanlineStride();

	for (int y = min_y; y <= max_y; ++y) {

		gfx_float w0 = w0_row;
		gfx_float w1 = w1_row;
		gfx_float w2 = w2_row;

		shader_input.fragments.data = pixel_offset;
		for (int x = min_x; x <= max_x; x += MPL_WIDTH) {

			gfx_bool fragment_mask = (w0 | w1 | w2) >= 0.0f;

			if (!fragment_mask.all_fail()) {

				for (int i = 0; i < var; ++i) {
					varying_arr[i] = (a_reg[i] * w0 + b_reg[i] * w1 + c_reg[i] * w2) * sum_inv_area_x2;
				}

				m_shader->Run(fragment_mask);
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



// Reference implementation for native rasterizer

template < int var, int cnst, typename shader_t >
void swsl::rasterizer::fill_triangle(const swsl::Point2D &a, const swsl::Point2D &b, const swsl::Point2D &c, const mmlVector<var> &a_attr, const mmlVector<var> &b_attr, const mmlVector<var> &c_attr, const mmlVector<cnst> &const_attr, shader_t shader)
{
	gfx_float  arr[m_out_buffer.GetPixelStride() + var + cnst];
	gfx_float *frag_arr = arr;
	gfx_float *var_arr  = arr + m_out_buffer.GetPixelStride();
	gfx_float *cnst_arr = var_arr + var;

	// Copy constant data to register
	for (int i = 0; i < cnst; ++i) {
		cnst_arr[i] = const_attr[i];
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
	const int min_y = mmlMax(mmlMin(a.y, b.y, c.y), m_mask_y1);
	const int max_y = mmlMin(mmlMax(a.y, b.y, c.y), m_mask_y2 - 1);
	const int min_x = mmlMax(floor_index(mmlMin(a.x, b.x, c.x)), m_mask_x1); // Make sure this is snapped to a block boundry
	const int max_x = mmlMin(mmlMax(a.x, b.x, c.x), m_mask_x2 - 1);

	// Triangle setup
	const gfx_float A01 = (float)((a.y - b.y) * MPL_WIDTH);
	const gfx_float B01 = (float)(b.x - a.x);
	const gfx_float A12 = (float)((b.y - c.y) * MPL_WIDTH);
	const gfx_float B12 = (float)(c.x - b.x);
	const gfx_float A20 = (float)((c.y - a.y) * MPL_WIDTH);
	const gfx_float B20 = (float)(a.x - c.x);

	const gfx_float bias0 = is_top_left(b, c) ? 0.0f : -1.0f;
	const gfx_float bias1 = is_top_left(c, a) ? 0.0f : -1.0f;
	const gfx_float bias2 = is_top_left(a, b) ? 0.0f : -1.0f;

	const float x_offset[] = MPL_OFFSETS;
	// float x_offset[] = MPL_X_OFFSETS;
	// float y_offset[] = MPL_Y_OFFSETS;

	wide_Point2D p = { gfx_float(min_x) + gfx_float(x_offset), min_y };
	gfx_float w0_row = orient_2d(b, c, p) + bias0;
	gfx_float w1_row = orient_2d(c, a, p) + bias1;
	gfx_float w2_row = orient_2d(a, b, p) + bias2;
	const gfx_float sum_inv_area_x2 = (gfx_float)(1.0f) / (w0_row + w1_row + w2_row);

	gfx_float *pixel_offset = (gfx_float*)m_out_buffer.GetComponent(min_x / MPL_WIDTH, min_y, 0);
	const int  pixel_y_stride = m_out_buffer.GetScanlineStride();
	const int  pixel_x_stride = m_out_buffer.GetPixelStride();

	for (int y = min_y; y <= max_y; ++y) {

		gfx_float w0 = w0_row;
		gfx_float w1 = w1_row;
		gfx_float w2 = w2_row;

		gfx_float *pixel = pixel_offset;
		for (int x = min_x; x <= max_x; x += MPL_WIDTH) {

			gfx_bool fragment_mask = (w0 | w1 | w2) >= 0.0f;

			if (!fragment_mask.all_fail()) {

				mtlCopy(frag_arr, pixel, pixel_x_stride);
				for (int i = 0; i < var; ++i) {
					var_arr[i] = (a_reg[i] * w0 + b_reg[i] * w1 + c_reg[i] * w2) * sum_inv_area_x2;
				}

				shader(arr, fragment_mask);

				mtlCopy(pixel, frag_arr, pixel_x_stride);
			}

			w0 += A12;
			w1 += A20;
			w2 += A01;

			pixel += pixel_x_stride;
		}

		w0_row += B12;
		w1_row += B20;
		w2_row += B01;

		pixel_offset += pixel_y_stride;
	}
}

template < int var, typename shader_t >
void swsl::rasterizer::fill_triangle(const swsl::Point2D &a, const swsl::Point2D &b, const swsl::Point2D &c, const mmlVector<var> &a_attr, const mmlVector<var> &b_attr, const mmlVector<var> &c_attr, shader_t shader)
{
	mmlVector<0> const_attr;
	fill_triangle(a, b, c, a_attr, b_attr, c_attr, const_attr, shader);
}

template < int cnst, typename shader_t >
void swsl::rasterizer::fill_triangle(const swsl::Point2D &a, const swsl::Point2D &b, const swsl::Point2D &c, const mmlVector<cnst> &const_attr, shader_t shader)
{
	mmlVector<0> a_attr, b_attr, c_attr;
	fill_triangle(a, b, c, a_attr, b_attr, c_attr, const_attr, shader);
}

template < typename shader_t >
void swsl::rasterizer::fill_triangle(const swsl::Point2D &a, const swsl::Point2D &b, const swsl::Point2D &c, shader_t shader)
{
	mmlVector<0> a_attr, b_attr, c_attr, const_attr;
	FillTriangle(a, b, c, a_attr, b_attr, c_attr, const_attr, shader);
}

#endif // SWSL_GFX_H_INCLUDED__
