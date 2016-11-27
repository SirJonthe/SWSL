TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

QMAKE_CXXFLAGS += \
#	-mcpu=cortex-a7 \
#	-mfloat-abi=hard \
#	-mfpu=neon-vfpv4 \
	-fopenmp \
	-pthread

QMAKE_LFLAGS += -fopenmp -pthread

LIBS += -fopenmp -pthread

SOURCES += \
	main.cpp \
    swsl_compiler.cpp \
    swsl_gfx.cpp \
    swsl_buffers.cpp \
    MiniLib/MGL/mglCamera.cpp \
    MiniLib/MGL/mglGraphics.cpp \
    MiniLib/MGL/mglImage.cpp \
    MiniLib/MGL/mglModel.cpp \
    MiniLib/MGL/mglNoise.cpp \
    MiniLib/MGL/mglParticleDynamics2D.cpp \
    MiniLib/MGL/mglPlane.cpp \
    MiniLib/MGL/mglRasterizer.cpp \
    MiniLib/MGL/mglRay.cpp \
    MiniLib/MGL/mglText.cpp \
    MiniLib/MGL/mglTexture.cpp \
    MiniLib/MGL/mglTransform.cpp \
    MiniLib/MTL/mtlMathParser.cpp \
    MiniLib/MTL/mtlParser.cpp \
    MiniLib/MTL/mtlRandom.cpp \
	MiniLib/MTL/mtlString.cpp \
	parser.cpp \
    swsl_shader.cpp \
    compiler.cpp \
    MiniLib/MTL/mtlPath.cpp

HEADERS += \
    swsl_compiler.h \
    swsl_instr.h \
    swsl.h \
    swsl_gfx.h \
    swsl_buffers.h \
    MiniLib/MGL/mglCamera.h \
    MiniLib/MGL/mglFramebuffer.h \
    MiniLib/MGL/mglGeometry.h \
    MiniLib/MGL/mglGraphics.h \
    MiniLib/MGL/mglImage.h \
    MiniLib/MGL/mglIndex.h \
    MiniLib/MGL/mglModel.h \
    MiniLib/MGL/mglNoise.h \
    MiniLib/MGL/mglParticleDynamics2D.h \
    MiniLib/MGL/mglPixel.h \
    MiniLib/MGL/mglPlane.h \
    MiniLib/MGL/mglRasterizer.h \
    MiniLib/MGL/mglRay.h \
    MiniLib/MGL/mglRenderer.h \
    MiniLib/MGL/mglText.h \
    MiniLib/MGL/mglTexture.h \
    MiniLib/MGL/mglTransform.h \
    MiniLib/MML/mmlFixed.h \
    MiniLib/MML/mmlMath.h \
    MiniLib/MML/mmlMatrix.h \
    MiniLib/MML/mmlQuaternion.h \
    MiniLib/MML/mmlVector.h \
    MiniLib/MTL/mtlArray.h \
    MiniLib/MTL/mtlAsset.h \
    MiniLib/MTL/mtlBinaryTree.h \
    MiniLib/MTL/mtlBits.h \
    MiniLib/MTL/mtlDuplex.h \
    MiniLib/MTL/mtlHashTable.h \
    MiniLib/MTL/mtlList.h \
    MiniLib/MTL/mtlMathParser.h \
    MiniLib/MTL/mtlMemory.h \
    MiniLib/MTL/mtlParser.h \
    MiniLib/MTL/mtlPointer.h \
    MiniLib/MTL/mtlRandom.h \
    MiniLib/MTL/mtlString.h \
    MiniLib/MTL/mtlStringMap.h \
    MiniLib/MTL/mtlType.h \
    parser.h \
    swsl_shader.h \
    MiniLib/MPL/old_mplCommon.h \
    MiniLib/MPL/old_mplMask4.h \
    MiniLib/MPL/mplWide.h \
    MiniLib/MPL/mplCommon.h \
    compiler.h \
    MiniLib/MTL/mtlPath.h \
    MiniLib/MPL/mplMath.h \
    swsl_aux.h \
    MiniLib/MML/mmlInt.h

macx: {
	OBJECTIVE_SOURCES += \
		SDLmain.m

	HEADERS += \
		SDLmain.h
}

win32: {
	# on Windows we need to manually add SDL and GLEW search paths
	LIBS += \
		-lSDL \
		-lSDLmain
}

macx: {
	LIBS += \
		-framework Cocoa \
		-framework SDL
}

unix:!macx: { # unix-like, e.g. linux, freeBSD
	LIBS += \
		-lSDL \
		-lSDLmain
}
