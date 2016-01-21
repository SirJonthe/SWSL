#ifndef SWSL_BUFFERS_H
#define SWSL_BUFFERS_H

#include "swsl_wide.h"

#include "MiniLib/MTL/mtlArray.h"

namespace swsl
{

	// Linear buffers
	// 1) Accessed in sequence
	// 2) No compression
	// 3) Modified by shaders

	class FrameBuffer
	{
	private:
		mtlArray<swsl::wide_float> m_data;
		int                        m_width;
		int                        m_height;
		int                        m_components;

	public:
		FrameBuffer( void ) : m_data(), m_width(0), m_height(0), m_components(0) {}

		void Create(int width, int height, int components);
		void Destroy( void );
		void Clear( void );

		int GetWidth( void )               const { return m_width; }
		int GetHeight( void )              const { return m_height; }
		int GetYawSize( void )             const { return m_components; }
		int GetPixelStride( void )         const { return m_components; }
		int GetPitchSize( void )           const { return m_components * m_width; }
		int GetScanlineStride( void )      const { return m_components * m_width; }
		int GetTotalComponentCount( void ) const { return m_data.GetSize(); }

		swsl::wide_float       *GetComponent(int x, int y, int c)       { return &m_data[(x + y * m_width) * m_components] + c; }
		const swsl::wide_float *GetComponent(int x, int y, int c) const { return &m_data[(x + y * m_width) * m_components] + c; }
	};

	// Non-linear buffers
	// 1) Can be sampled at several locations at once
	// 2) Compression and swizzling makes cache misses less likely
	// 3) Are passed to the shader as parameters

	class TextureBuffer
	{
	private:
		mtlArray<unsigned char> m_data; // not a SIMD register because textures are not accessed in blocks of pixels
		int                     m_width;
		int                     m_height;
	};

}

#endif // SWSL_BUFFERS_H
