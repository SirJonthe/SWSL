TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

#QMAKE_CXXFLAGS += \
#	-mcpu=cortex-a7 \
#	-mfloat-abi=hard \
#	-mfpu=neon-vfpv4 \
#	-fopenmp \
#	-pthread

#QMAKE_LFLAGS += \
#	-fopenmp \
#	-pthread

#LIBS += \
#	-fopenmp \
#	-pthread

SOURCES += \
	main.cpp \
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
    swsl_shader.cpp \
    MiniLib/MTL/mtlPath.cpp \
    swsl_astgen.cpp \
    swsl_tokdisp.cpp \
    swsl_cpptrans.cpp \
    swsl_astgen_new.cpp \
    swsl_json.cpp

HEADERS += \
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
    swsl_shader.h \
    MiniLib/MPL/mplWide.h \
    MiniLib/MPL/mplCommon.h \
    MiniLib/MTL/mtlPath.h \
    MiniLib/MPL/mplMath.h \
    swsl_aux.h \
    MiniLib/MML/mmlInt.h \
    swsl_types.h \
    swsl_program.h \
    swsl_astgen.h \
    swsl_tokdisp.h \
    tmp_out.h \
    swsl_math.h \
    MiniLib/MPL/mplAlloc.h \
    swsl_cpptrans.h \
    swsl_astgen_new.h \
    swsl_json.h

macx: {
    OBJECTIVE_SOURCES += \
        SDLmain.m

    HEADERS += \
        SDLmain.h
}

win32: {
    # On Windows we need to manually add SDL search paths in the project file.
    # There is no default install directory.
	LIBS += \
		-lSDL \
		-lSDLmain
}

macx: {
    # On newer versions of OSX with Xcode installed
    # the default search path is not /Library/Frameworks.
    # Instead g++ searches for frameworks by default in
    # /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/System/Library/Frameworks
    # If you do not wish to set a new search path, install SDL framework files there.
    # Note: Even absolute search paths like /Library/Frameworks/SDL fails to detect the framework
    LIBS += \
        -framework Cocoa \
        -framework SDL
}

unix:!macx: { # unix-like, e.g. linux, freeBSD
    LIBS += \
        -lSDL \
        -lSDLmain
}

DISTFILES += \
    TODO.txt
