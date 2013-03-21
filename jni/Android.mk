LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := sample

COMMON_DIR := common

LOCAL_CFLAGS := -DANDROID_NDK -DUSE_ORIGINAL_BACKGROUND \
				 -I$(COMMON_DIR)/include

# seigo3 add mp32wav.cpp
LOCAL_SRC_FILES := \
    sample.cpp \
    app-android.c \
    Base64Transcoder.c

LOCAL_LDLIBS := -lGLESv1_CM -ldl -lz -llog -lMPU -lMP

# seigo3
LOCAL_LDLIBS += -landroid -lOpenSLES

include $(BUILD_SHARED_LIBRARY)
