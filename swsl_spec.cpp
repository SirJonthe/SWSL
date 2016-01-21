/*
====
SWSL
====

-----
INTRO
-----
SWSL (SoftWare Shading Language) is a shading language meant to run on the CPU while employing massively-parallel techniques.

----------
PRINCIPLES
----------

Idea
----
* Shaders are self-contained and can be executed in parallel across any number of cores and threads.
* Shader types are wide in order to utilize SIMD parallelism.

Types
-----
* Basic, non-vector types are "secretly" wide, meaning they are implemented as SIMD types of varying width depending on the capabilities of the host processor.
* The programmer has no knowledge of the width of the basic types. The code will be completely agnostic to type width.
* The programmer can not access individual scalar values.
* Vector types are implemented as an array of basic (wide) types, and as such are an expression of the SoAoS principle.
* Conditional code looks like any C-like language, further obscuring the fact that basic types are wide.

--------------
IMPLEMENTATION
--------------

Types
-----
* Basic types include: float.
* Vector types include: float2, float3, float4.
* [There is no implicit type casting between basic types. Type casting is C-styled.]
  - Will only be relevant once more basic types are implemented.
* [There is no implicit type casting between vector types. Type casting is C-styled.]
  - Only applies to vectors of same size, but different base type.
  - Use combined component names for conversion between different sized vectors of same base type (see below).
* Support for a small stack.
* No dynamic memory allocation.

Memory layout
-------------
* How the application running the code formats the input data is implementation dependent. Destination pixels could for instance be laid out in 1x1, 2x2, 4x4, or even 4x1, 4x2, 16x1 etc, or even swizzled according to some other indexing method. The point is that the code stays the same regardless of layout.

Conditional code
----------------
* Conditional code is handled by "if-else", "for", "while" and "do-while".
* As the language aims to hide the fact that the programmer is working with wide types there is no need to work with masks to implement conditional coding.
* One of the main differences between serial code and wide code is that both the code inside the "if" statement *and* the "else" statement is executed. The code inside each statement is simply compiled into a series of masked instructions.
* Conditionals should generally be avoided, since in a worst case scenario, no scalar value will be affected by code inside a conditional (for cases where the condition fails for all individual scalar values in a wide type).
* Since both code paths must be executed, "continue" and "break" must be implemented by modifying the conditional mask used to mask out what operations are affected by. "return" must allow a single pass of all connected conditionals.

Passing data to shaders
-----------------------
* Any amount of interpolated float data can be passed to shaders in the form of float or any float vector type.
* Syntax:
  void main(float2 uv, float3 color, float3 normal)
* The application running the shader is responsible to make sure errors are generated for any shader where the number of expected inputs do not match the amount of actual inputs from the rasterizer.
* Some interpolated data is globally available in all shaders and do not need to be explicitly passed
  - depth    : the depth at the current fragment coordinate (float, read/write)
  - fragment : the color of the current fragment (float3, read/write)
  - coord    : the screen-space coordinate of the current fragment (float3, read-only)
  - pos      : the world-space position of the triangle fragment being rasterized (float3, read-only)

Assignment
----------
* Basic types are assigned using a single value and the appropriate suffix:
  float a = 1.0f;
* Vector types are assigned using aggregate construction:
  float2 a = { 1.0f, 0.0f };

Component access
----------------
* Using a float3 as example.
* Vector elements can be accessed in a number of ways:
  - As individual basic types using component name, e.g.
    f.x, f.y, f.z (returns a float)
  - As vector types using a combination of component names, e.g:
    f.xy, f.xz, f.yz (returns a float2), or f.xyz (returns float3)
  - As a shuffled vector using a combination of component names, e.g:
    f.xz, f.zx, f.yz, f.yx (returns a float2)
* Regardless of method, access supports both read and write (as long as types match), e.g.
  - Shuffle:
    a.yz = a.zy;


Math operations
---------------
* Basic types implement basic math operations (+,-,*,/,%,&,|,~,^, etc.).
* Supports a minimal C-styled math library for basic types.
* Implements basic vector operations (+,-,*,/). Uses function calls for dot product, cross product, etc..

Notes
-----
* When writing to fragment and depth a mask has to be applied in order to avoid that pixels included in the input fragment block are not written to if they lie outside the triangle.

------------
FUTURE PLANS
------------
* Passing any number of variable-type read-write fragments from buffers into the shaders.
* Support basic type "int" and associated vector types. No value suffix, no decimals.
* Support basic type "fixed" and associated vector types. Value suffix is "i".
* Passing non-interpolated data . Syntax in global scope: "static float2 data;" (static denotes that it does not change, error when initialized)
* Functions.
* Structs.
* "texture" type. Textures as input data. Uses 2d vectors for sampling.

*/

void main(float3 color, float3 normal)
{
	if (color.x < 0.0f || color.y < 0.0f) {
		color.x = abs(color.x);
	} else {
		color.x = 0.0f;
	}

	return;
}