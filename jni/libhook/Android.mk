LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := libhook
LOCAL_SRC_FILES := main.cpp hook.cpp report.cpp hooks/io.cpp
LOCAL_LDLIBS    := -llog

include $(BUILD_SHARED_LIBRARY)
