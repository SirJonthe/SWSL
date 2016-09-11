#ifndef SWSL_BUFFERS_H
#define SWSL_BUFFERS_H

#include "MiniLib/MPL/mplWide.h"
#include "MiniLib/MTL/mtlArray.h"
#include "MiniLib/MTL/mtlBits.h"

namespace swsl
{

	// Linear buffers
	// 1) Accessed in sequence
	// 2) No compression
	// 3) Modified by shaders

	class FrameBuffer
	{
	private:
		mtlArray<mpl::wide_float> m_data;
		int                       m_width;
		int                       m_height;
		int                       m_components;

	public:
		FrameBuffer( void ) : m_data(), m_width(0), m_height(0), m_components(0) {}

		void Create(int width, int height, int components);
		void Destroy( void );
		void Clear( void );

		int GetPackedWidth( void )         const { return m_width; }
		int GetHeight( void )              const { return m_height; }
		int GetPixelStride( void )         const { return m_components; }
		int GetScanlineStride( void )      const { return m_components * m_width; }
		int GetTotalComponentCount( void ) const { return m_data.GetSize(); }

		mpl::wide_float       *GetComponent(int x, int y, int c = 0)       { return m_data + (x + y * m_width) * m_components + c; }
		const mpl::wide_float *GetComponent(int x, int y, int c = 0) const { return m_data + (x + y * m_width) * m_components + c; }
	};

	// Non-linear buffers
	// 1) Can be sampled at several locations at once
	// 2) Compression and swizzling makes cache misses less likely
	// 3) Are passed to the shader as parameters

	class TextureBuffer
	{
	private:
		mtlArray<mtlByte> m_data; // not a SIMD register because textures are not accessed in blocks of pixels
		int               m_width;
		int               m_height;
	};

}

#endif // SWSL_BUFFERS_H
