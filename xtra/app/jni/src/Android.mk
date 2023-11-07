LOCAL_PATH := $(call my-dir)

LOCAL_SHORT_COMMANDS := true
APP_SHORT_COMMANDS := true

# zirco\xtra\app\jni\src <-- @ src

# up 4
C_FOLDER := ../../../..

C_XTRA_SDK  := ./xtra/SDK



SDL_SUB_H_PATH := SDL2-devel-2.0.8-mingw/SDL2-2.0.8/i686-w64-mingw32/include

C_XTRA_SDK_SDL_H_PATH := $(C_XTRA_SDK)/$(SDL_SUB_H_PATH)

include $(CLEAR_VARS)

# SDL2 expects this name (libmain.so) so cannot rename to zircon or such
LOCAL_MODULE := main

#static libraries
#LOCAL_STATIC_LIBRARIES	:= systemutils vorbis vorbisfile ogg modplug  
#LOCAL_STATIC_LIBRARIES	:= vorbis vorbisfile ogg freetype2 jpeg9
# Baker: vorbis ogg static for vorbisfile, not us.
LOCAL_STATIC_LIBRARIES	:= vorbisfile freetype2 jpeg9


# zirco\xtra\app\jni\src <-- @ src
# header here!
LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(C_XTRA_SDK_SDL_H_PATH)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(C_FOLDER)
LOCAL_C_INCLUDES		+=	$(LOCAL_PATH)\
							$(LOCAL_PATH)/../libogg/include/ \
							$(LOCAL_PATH)/../vorbis/include/ \



LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(CORE_PATH_SDK)

# following line prints which is what the info command does.
$(info $(LOCAL_C_INCLUDES))


MY_FILE_LIST  := $(wildcard $(LOCAL_PATH)/$(C_FOLDER)/*.c)

LOCAL_SRC_FILES := $(MY_FILE_LIST:$(LOCAL_PATH)/%=%)


LOCAL_CFLAGS    :=	-DCORE_SDL					\
					-DCONFIG_MENU				\
					-D_FILE_OFFSET_BITS=64		\
					-D__KERNEL_STRICT_NAMES		\
					-DUSE_GLES2



#$(info $(LOCAL_PATH))
#$(info $(MY_FILE_LIST))
#$(error Bob)

LOCAL_SHORT_COMMANDS := true
APP_SHORT_COMMANDS := true

LOCAL_SHARED_LIBRARIES := SDL2


LOCAL_LDLIBS := -lGLESv2 -lEGL -ldl -llog



LOCAL_SHORT_COMMANDS := true
APP_SHORT_COMMANDS := true

include $(BUILD_SHARED_LIBRARY)
