#ifndef SWSL_SIMD_H
#define SWSL_SIMD_H

#define SWSL_COMPILER_UNKNOWN 0
#define SWSL_COMPILER_MSVC    1
#define SWSL_COMPILER_GCC     2

#define SWSL_SIMD_NONE   0
#define SWSL_SIMD_SSE    1
#define SWSL_SIMD_AVX256 2
#define SWSL_SIMD_AVX512 3
#define SWSL_SIMD_NEON   4

// add support for AVX-256 and AVX-512

// Detect instruction set
#if defined(__GNUC__)
	#define SWSL_COMPILER SWSL_COMPILER_GCC
	#if defined(__AVX__)
		#define SWSL_SIMD SWSL_SIMD_AVX256
	#elif defined(__SSE__)
		#define SWSL_SIMD  SWSL_SIMD_SSE
	#elif defined(__ARM_NEON__)
		// Enable on Raspberry Pi 2 using these compiler flags:
			// -mcpu=cortex-a7
			// -mfloat-abi=hard
			// -mfpu=neon-vfpv4
		#define SWSL_SIMD SWSL_SIMD_NEON
	#else
		#define SWSL_SIMD SWSL_SIMD_NONE
		#warning No SIMD support, falling back to scalar
	#endif
#elif defined(_MSC_VER)
	#define SWSL_COMPILER SWSL_COMPILER_MSVC
	#ifndef _M_CEE_PURE
		#define SWSL_SIMD SWSL_SSE
	#endif
#else
	#define SWSL_COMPILER SWSL_COMPILER_UNKNOWN
	#define SWSL_SIMD     SWSL_SIMD_NONE
	#warning No SIMD support, falling back to scalar
#endif

// SIMD instruction definitions
#if SWSL_SIMD == SWSL_SIMD_SSE
	#define SWSL_WIDTH         4
	#define SWSL_ALIGN         16
	#define SWSL_BLOCK_X       2
	#define SWSL_BLOCK_Y       2
	#define SWSL_OFFSETS       { 0, 1, 2, 3 }
	#define SWSL_X_OFFSETS     { 0, 1 }
	#define SWSL_Y_OFFSETS     SWSL_X_OFFSETS
#elif SWSL_SIMD == SWSL_SIMD_NEON
	#define SWSL_WIDTH         4
	#define SWSL_ALIGN         4
	#define SWSL_BLOCK_X       2
	#define SWSL_BLOCK_Y       2
	#define SWSL_OFFSETS       { 0, 1, 2, 3 }
	#define SWSL_X_OFFSETS     { 0, 1 }
	#define SWSL_Y_OFFSETS     SWSL_X_OFFSETS
#elif SWSL_SIMD == SWSL_SIMD_AVX256
	#define SWSL_WIDTH         8
	#define SWSL_ALIGN         32
	#define SWSL_BLOCK_X       4
	#define SWSL_BLOCK_Y       2
	#define SWSL_OFFSETS       { 0, 1, 2, 3, 4, 5, 6, 7 }
	#define SWSL_X_OFFSETS     { 0, 1, 2, 3 }
	#define SWSL_Y_OFFSETS     { 0, 1 }
#elif SWSL_SIMD == SWSL_SIMD_AVX512
	#define SWSL_WIDTH         16
	#define SWSL_ALIGN         64
	#define SWSL_BLOCK_X       4
	#define SWSL_BLOCK_Y       4
	#define SWSL_OFFSETS       { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 }
	#define SWSL_X_OFFSETS     { 0, 1, 2, 3 }
	#define SWSL_Y_OFFSETS     SWSL_X_OFFSETS
#elif SWSL_SIMD == SWSL_SIMD_NONE
	#define SWSL_WIDTH         1
	#define SWSL_ALIGN         1
	#define SWSL_BLOCK_X       1
	#define SWSL_BLOCK_Y       1
	#define SWSL_OFFSETS       { 0 }
	#define SWSL_X_OFFSETS     SWSL_OFFSETS
	#define SWSL_Y_OFFSETS     SWSL_OFFSETS
#endif

// Common defines
#define SWSL_SCALAR_SIZE     sizeof(float)
#define SWSL_WIDTH_MASK      (SWSL_WIDTH - 1)
#define SWSL_WIDTH_INVMASK   (~SWSL_WIDTH_MASK)
#define SWSL_BLOCK_X_MASK    (SWSL_BLOCK_X - 1)
#define SWSL_BLOCK_Y_MASK    (SWSL_BLOCK_Y - 1)
#define SWSL_BLOCK_X_INVMASK (~SWSL_BLOCK_X_MASK)
#define SWSL_BLOCK_Y_INVMASK (~SWSL_BLOCK_Y_MASK)
#define SWSL_WIDE_WORD_SIZE  (SWSL_SCALAR_SIZE*SWSL_WIDTH)

// Include appropriate headers
#if SWSL_SIMD == SWSL_SIMD_AVX256 || SWSL_SIMD == SWSL_SIMD_AVX512
	#include <immintrin.h>
#elif SWSL_SIMD == SWSL_SIMD_SSE
	#include <xmmintrin.h>
#elif SWSL_SIMD == SWSL_SIMD_NEON
	#include <arm_neon.h>
#endif

// Include header for scalar math
#include "MiniLib/MML/mmlMath.h"


#endif // SWSL_SIMD_H
