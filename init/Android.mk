LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES := \
    system/core/base/include \
    system/core/init \
    external/selinux/libselinux/include

LOCAL_CFLAGS := -Wall -DANDROID_TARGET=\"$(TARGET_BOARD_PLATFORM)\"
LOCAL_CPP_STD := experimental
LOCAL_SRC_FILES := init_bamboo.cpp
LOCAL_MODULE := libinit_bamboo
LOCAL_STATIC_LIBRARIES := libbase libselinux
include $(BUILD_STATIC_LIBRARY)
