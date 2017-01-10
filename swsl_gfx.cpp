#include <omp.h>

#include "swsl_gfx.h"

bool swsl::Rasterizer::IsTopLeft(const swsl::Point2D &a, const swsl::Point2D &b) const
{
	// strictly connected to winding order
	return (a.x < b.x && b.y == a.y) || (a.y > b.y);
}

int swsl::Rasterizer::Orient2D(const swsl::Point2D &a, const swsl::Point2D &b, const swsl::Point2D &c) const
{
	return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

swsl::Rasterizer::gfx_float swsl::Rasterizer::Orient2D(const swsl::Point2D &a, const swsl::Point2D &b, const swsl::Rasterizer::wide_Point2D &c) const
{
	wide_Point2D A = { a.x, a.y }, B = { b.x, b.y };
	return (B.x - A.x) * (c.y - A.y) - (B.y - A.y) * (c.x - A.x);
}

int swsl::Rasterizer::GetMaskWidthStride( void ) const
{
	return ((m_mask_x2 - m_mask_x1) / MPL_WIDTH) * m_out_buffer.GetPixelStride();
}

int swsl::Rasterizer::CeilIndex(int i) const
{
	return (i + MPL_WIDTH_MASK) & MPL_WIDTH_INVMASK;
}

int swsl::Rasterizer::FloorIndex(int i) const
{
	return i & MPL_WIDTH_INVMASK;
}

swsl::Rasterizer::Rasterizer( void ) : m_shader(NULL), m_width(0), m_height(0), m_mask_x1(0), m_mask_y1(0), m_mask_x2(0), m_mask_y2(0) {}

void swsl::Rasterizer::SetShader(swsl::Shader *shader)
{
	m_shader = shader;
}

void swsl::Rasterizer::CreateBuffers(int width, int height, int components)
{
	m_width = width;
	m_height = height;
	m_out_buffer.Create(width, height, components); // RGB + depth = 4 components
	ResetRasterMask();
}

void swsl::Rasterizer::SetRasterMask(int x1, int y1, int x2, int y2)
{
	// raster mask does not necessarily respect the exact boundry specified
	// due to the memory layout of the frame buffer

	m_mask_x1 = mmlMax(FloorIndex(x1), 0);
	m_mask_y1 = mmlMax(y1, 0);
	m_mask_x2 = mmlMin(CeilIndex(x2), m_width);
	m_mask_y2 = mmlMin(y2, m_height);
}

void swsl::Rasterizer::ResetRasterMask( void )
{
	m_mask_x1 = 0;
	m_mask_y1 = 0;
	m_mask_x2 = m_width;
	m_mask_y2 = m_height;
}

void swsl::Rasterizer::ClearBuffers( void )
{
	int scanline_stride    = m_out_buffer.GetScanlineStride();
	int mask_stride        = GetMaskWidthStride();
	gfx_float *buffer_data = m_out_buffer.GetComponent(m_mask_x1 / MPL_WIDTH, m_mask_y1);

	for (int y = m_mask_y1; y < m_mask_y2; ++y) {
		mtlClear(buffer_data, mask_stride);
		buffer_data += scanline_stride;
	}
}

void swsl::Rasterizer::ClearBuffers(const float *component_data)
{
	/*int        buffer_area   = m_out_buffer.GetWidth() * m_out_buffer.GetHeight();
	int        buffer_stride = m_out_buffer.GetPixelStride();
	gfx_float *buffer_data   = m_out_buffer.GetComponent(0,0);
	for (int i = 0; i < buffer_area; ++i) {
		for (int n = 0; n < buffer_stride; ++n) {
			*buffer_data = component_data[n];
			++buffer_data;
		}
	}*/

	int        scanline_stride = m_out_buffer.GetScanlineStride();
	int        pixel_stride    = m_out_buffer.GetPixelStride();
	int        mask_stride     = GetMaskWidthStride();
	gfx_float *buffer_data     = m_out_buffer.GetComponent(m_mask_x1 / MPL_WIDTH, m_mask_y1);

	for (int y = m_mask_y1; y < m_mask_y2; ++y) {
		for (int x = 0; x < mask_stride;) {
			for (int n = 0; n < pixel_stride; ++n, ++x) {
				buffer_data[x] = component_data[n];
			}
		}
		buffer_data += scanline_stride;
	}
}

void swsl::Rasterizer::WriteColorBuffer(int src_r_idx, int src_g_idx, int src_b_idx, mtlByte *dst_pixels, int dst_bytes_per_pixel, mglByteOrder32 dst_byte_order)
{
	const int        src_scanline_stride = m_out_buffer.GetScanlineStride();
	const int        dst_scanline_stride = dst_bytes_per_pixel * m_width;
	const int        src_pixel_stride = m_out_buffer.GetPixelStride();
	const int        x1 = m_mask_x1 / MPL_WIDTH;
	const int        x2 = m_mask_x2 / MPL_WIDTH;
	const gfx_float *src_pixels = m_out_buffer.GetComponent(x1, m_mask_y1);
	const gfx_float  scale = 255.0f;
	gfx_float        rf, gf, bf;
	gfx_int          ri, gi, bi;
	int              rs[MPL_WIDTH], gs[MPL_WIDTH], bs[MPL_WIDTH];

	dst_pixels += (m_mask_x1 + m_mask_y1 * m_width) * dst_bytes_per_pixel;

	for (int y = m_mask_y1; y < m_mask_y2; ++y) {
		mtlByte         *dst_pixel = dst_pixels;
		const gfx_float *src_pixel = src_pixels;
		for (int x = x1; x < x2; ++x) {
			rf = *(src_pixel + src_r_idx) * scale;
			gf = *(src_pixel + src_g_idx) * scale;
			bf = *(src_pixel + src_b_idx) * scale;
			ri = gfx_int(rf);
			gi = gfx_int(gf);
			bi = gfx_int(bf);
			ri.to_scalar(rs);
			gi.to_scalar(gs);
			bi.to_scalar(bs);
			for (int n = 0; n < MPL_WIDTH; ++n) {
				dst_pixel[dst_byte_order.index.r] = rs[n];
				dst_pixel[dst_byte_order.index.g] = gs[n];
				dst_pixel[dst_byte_order.index.b] = bs[n];
				dst_pixel += dst_bytes_per_pixel;
			}
			src_pixel += src_pixel_stride;
		}
		dst_pixels += dst_scanline_stride;
		src_pixels += src_scanline_stride;
	}
}

/*void swsl::Rasterizer::WriteColorBuffer(int src_r_idx, int src_g_idx, int src_b_idx, mtlByte *dst_pixels, int dst_bytes_per_pixel, mglByteOrder32 dst_byte_order)
{
	const int  buffer_area   = m_out_buffer.GetPackedWidth() * m_out_buffer.GetHeight();
	const int  buffer_stride = m_out_buffer.GetPixelStride();
	gfx_float *buffer_data   = m_out_buffer.GetComponent(0,0);

	const gfx_float scale = 255.0f;
	gfx_float       rf, gf, bf;
	gfx_int         ri, gi, bi;
	int             rs[MPL_WIDTH], gs[MPL_WIDTH], bs[MPL_WIDTH];
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
		for (int n = 0; n < MPL_WIDTH; ++n) {
			dst_pixels[dst_byte_order.index.r] = rs[n];
			dst_pixels[dst_byte_order.index.g] = gs[n];
			dst_pixels[dst_byte_order.index.b] = bs[n];
			dst_pixels += dst_bytes_per_pixel;
		}
		buffer_data += buffer_stride;
	}
}*/

void swsl::Rasterizer::WriteColorBuffer(mtlByte *dst_pixels, int dst_bytes_per_pixel, mglByteOrder32 dst_byte_order)
{
	WriteColorBuffer(0, 1, 2, dst_pixels, dst_bytes_per_pixel, dst_byte_order);
}

void swsl::Rasterizer::FillTriangle(const swsl::Point2D &a, const swsl::Point2D &b, const swsl::Point2D &c)
{
	mmlVector<0> a_attr, b_attr, c_attr, const_attr;
	FillTriangle(a, b, c, a_attr, b_attr, c_attr, const_attr);
}

// Triangle filling algorithm with linear interpolation - Scalar code, reference
/*
void FillTriangle(mtlByte *pixels, int bytes_per_pixel, int width, int height, const swsl::swsl::Point2D &a, const swsl::swsl::Point2D &b, const swsl::swsl::Point2D &c, const mmlVector<3> &a_attr, const mmlVector<3> &b_attr, const mmlVector<3> &c_attr)
{
	// AABB Clipping
	int min_y = mmlMax2(mmlMin3(a.y, b.y, c.y), 0);
	int max_y = mmlMin(mmlMax3(a.y, b.y, c.y), height - 1);
	int min_x = mmlMax2(mmlMin3(a.x, b.x, c.x), 0);
	int max_x = mmlMin(mmlMax3(a.x, b.x, c.x), width - 1);

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

	swsl::swsl::Point2D p = { min_x, min_y };
	int w0_row = Orient2D(b, c, p) + bias0;
	int w1_row = Orient2D(c, a, p) + bias1;
	int w2_row = Orient2D(a, b, p) + bias2;
	float sum_inv_area_x2 = 1.0f / (float)(w0_row + w1_row + w2_row);

	mmlVector<3> attr;

	int pitch = width * bytes_per_pixel;
	mtlByte *pixel_offset = pixels + ((min_x + min_y * width) * bytes_per_pixel);
	for (p.y = min_y; p.y <= max_y; ++p.y) { // p.y += MPL_BLOCK_Y

		int w0 = w0_row;
		int w1 = w1_row;
		int w2 = w2_row;

		mtlByte *pixels = pixel_offset;
		for (p.x = min_x; p.x <= max_x; ++p.x) { // p.x += MPL_BLOCK_X

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



// Reference implementation of native rasterizer

bool swsl::rasterizer::is_top_left(const swsl::Point2D &a, const swsl::Point2D &b) const
{
	// strictly connected to winding order
	return (a.x < b.x && b.y == a.y) || (a.y > b.y);
}

int swsl::rasterizer::orient_2d(const swsl::Point2D &a, const swsl::Point2D &b, const swsl::Point2D &c) const
{
	return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

swsl::rasterizer::gfx_float swsl::rasterizer::orient_2d(const swsl::Point2D &a, const swsl::Point2D &b, const swsl::rasterizer::wide_Point2D &c) const
{
	wide_Point2D A = { a.x, a.y }, B = { b.x, b.y };
	return (B.x - A.x) * (c.y - A.y) - (B.y - A.y) * (c.x - A.x);
}

int swsl::rasterizer::get_mask_width_stride( void ) const
{
	return ((m_mask_x2 - m_mask_x1) / MPL_WIDTH) * m_out_buffer.GetPixelStride();
}

int swsl::rasterizer::ceil_index(int i) const
{
	return (i + MPL_WIDTH_MASK) & MPL_WIDTH_INVMASK;
}

int swsl::rasterizer::floor_index(int i) const
{
	return i & MPL_WIDTH_INVMASK;
}

swsl::rasterizer::rasterizer( void ) : m_width(0), m_height(0), m_mask_x1(0), m_mask_y1(0), m_mask_x2(0), m_mask_y2(0) {}


void swsl::rasterizer::create_buffers(int width, int height, int components)
{
	m_width = width;
	m_height = height;
	m_out_buffer.Create(width, height, components); // RGB + depth = 4 components
	reset_raster_mask();
}

void swsl::rasterizer::set_raster_mask(int x1, int y1, int x2, int y2)
{
	m_mask_x1 = mmlMax(floor_index(x1), 0);
	m_mask_y1 = mmlMax(y1, 0);
	m_mask_x2 = mmlMin(ceil_index(x2), m_width);
	m_mask_y2 = mmlMin(y2, m_height);
}

void swsl::rasterizer::reset_raster_mask( void )
{
	m_mask_x1 = 0;
	m_mask_y1 = 0;
	m_mask_x2 = m_width;
	m_mask_y2 = m_height;
}

void swsl::rasterizer::clear_buffers( void )
{
	const int  scanline_stride = m_out_buffer.GetScanlineStride();
	const int  mask_stride     = get_mask_width_stride();
	gfx_float *buffer_data     = m_out_buffer.GetComponent(m_mask_x1 / MPL_WIDTH, m_mask_y1);

	for (int y = m_mask_y1; y < m_mask_y2; ++y) {
		mtlClear(buffer_data, mask_stride);
		buffer_data += scanline_stride;
	}
}

void swsl::rasterizer::clear_buffers(const float *component_data)
{
	const int  scanline_stride = m_out_buffer.GetScanlineStride();
	const int  pixel_stride    = m_out_buffer.GetPixelStride();
	const int  mask_stride     = get_mask_width_stride();
	gfx_float *buffer_data     = m_out_buffer.GetComponent(m_mask_x1 / MPL_WIDTH, m_mask_y1);

	for (int y = m_mask_y1; y < m_mask_y2; ++y) {
		for (int x = 0; x < mask_stride;) {
			for (int n = 0; n < pixel_stride; ++n, ++x) {
				buffer_data[x] = component_data[n];
			}
		}
		buffer_data += scanline_stride;
	}
}

void swsl::rasterizer::write_color_buffer(int src_r_idx, int src_g_idx, int src_b_idx, mtlByte *dst_pixels, int dst_bytes_per_pixel, mglByteOrder32 dst_byte_order)
{
	const int        src_scanline_stride = m_out_buffer.GetScanlineStride();
	const int        dst_scanline_stride = dst_bytes_per_pixel * m_width;
	const int        src_pixel_stride = m_out_buffer.GetPixelStride();
	const int        x1 = m_mask_x1 / MPL_WIDTH;
	const int        x2 = m_mask_x2 / MPL_WIDTH;
	const gfx_float *src_pixels = m_out_buffer.GetComponent(x1, m_mask_y1);
	const gfx_float  scale = 255.0f;
	gfx_float        rf, gf, bf;
	gfx_int          ri, gi, bi;
	int              rs[MPL_WIDTH], gs[MPL_WIDTH], bs[MPL_WIDTH];

	dst_pixels += (m_mask_x1 + m_mask_y1 * m_width) * dst_bytes_per_pixel;

	for (int y = m_mask_y1; y < m_mask_y2; ++y) {
		mtlByte         *dst_pixel = dst_pixels;
		const gfx_float *src_pixel = src_pixels;
		for (int x = x1; x < x2; ++x) {
			rf = *(src_pixel + src_r_idx) * scale;
			gf = *(src_pixel + src_g_idx) * scale;
			bf = *(src_pixel + src_b_idx) * scale;
			ri = gfx_int(rf);
			gi = gfx_int(gf);
			bi = gfx_int(bf);
			ri.to_scalar(rs);
			gi.to_scalar(gs);
			bi.to_scalar(bs);
			for (int n = 0; n < MPL_WIDTH; ++n) {
				dst_pixel[dst_byte_order.index.r] = rs[n];
				dst_pixel[dst_byte_order.index.g] = gs[n];
				dst_pixel[dst_byte_order.index.b] = bs[n];
				dst_pixel += dst_bytes_per_pixel;
			}
			src_pixel += src_pixel_stride;
		}
		dst_pixels += dst_scanline_stride;
		src_pixels += src_scanline_stride;
	}
}

void swsl::rasterizer::write_color_buffer(mtlByte *dst_pixels, int dst_bytes_per_pixel, mglByteOrder32 dst_byte_order)
{
	write_color_buffer(0, 1, 2, dst_pixels, dst_bytes_per_pixel, dst_byte_order);
}
