LOCAL_PATH := $(call my-dir)

# ------------------------------------------------------------------
# Static library
# ------------------------------------------------------------------

include $(CLEAR_VARS)

LOCAL_MODULE := freetype2

LOCAL_CFLAGS := -DANDROID_NDK \
		-DFT2_BUILD_LIBRARY=1

LOCAL_C_INCLUDES := $(LOCAL_PATH) \
		$(LOCAL_PATH)/../include \


LOCAL_SRC_FILES := \
	autofit/autofit.c \
	base/basepic.c \
	base/ftapi.c \
	base/ftbase.c \
	base/ftbbox.c \
	base/ftbitmap.c \
	base/ftdbgmem.c \
	base/ftdebug.c \
	base/ftglyph.c \
	base/ftinit.c \
	base/ftpic.c \
	base/ftstroke.c \
	base/ftsynth.c \
	base/ftsystem.c \
	cff/cff.c \
	pshinter/pshinter.c \
	psnames/psnames.c \
	raster/raster.c \
	sfnt/sfnt.c \
	smooth/smooth.c \
	truetype/truetype.c

LOCAL_LDLIBS := -ldl -llog

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)

include $(BUILD_STATIC_LIBRARY)
