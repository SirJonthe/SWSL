#include "swsl_buffers.h"

#include "MiniLib/MML/mmlMath.h"

void swsl::FrameBuffer::Create(int width, int height, int components)
{
	width  = mmlMax(0, MPL_CEIL(width)) / MPL_WIDTH;
	height = mmlMax(0, height);

	if (width * height == 0) {
		Destroy();
	} else if (width * height * components > m_width * m_height * m_components) {
		m_data.Create(width * height * components);
	}
	m_width      = width;
	m_height     = height;
	m_components = components;
}

void swsl::FrameBuffer::Destroy( void )
{
	m_width  = 0;
	m_height = 0;
}

void swsl::FrameBuffer::Clear( void )
{
	mtlClear(&m_data[0], m_width * m_height * m_components * sizeof(mpl::wide_float));
}
