LOCAL_PATH := $(call my-dir)

ifneq ($(filter bamboo bamboolte,$(TARGET_DEVICE)),)

include $(call all-subdir-makefiles,$(LOCAL_PATH))

endif
