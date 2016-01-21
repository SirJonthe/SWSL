#include "swsl_buffers.h"

#include "MiniLib/MML/mmlMath.h"

void swsl::FrameBuffer::Create(int width, int height, int components)
{
	width = mmlMax2(0, width);
	height = mmlMax2(0, height);
	float frac_width = (float)width / (float)SWSL_WIDTH;
	width = (int)frac_width;
	if (frac_width > (float)width) {
		width += 1;
	}
	if (width * height == 0) {
		Destroy();
	} else if (width * height * components > m_width * m_height * m_components) {
		m_data.Create(width * height * components);
	}
	m_width = width;
	m_height = height;
	m_components = components;
}

void swsl::FrameBuffer::Destroy( void )
{
	m_width = 0;
	m_height = 0;
}

void swsl::FrameBuffer::Clear( void )
{
	mtlClear(&m_data[0], m_width * m_height * m_components * sizeof(swsl::wide_float));
}
