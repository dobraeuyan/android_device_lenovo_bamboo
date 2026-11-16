LOCAL_PATH := $(call my-dir)

ifneq ($(filter bamboo bamboolte,$(TARGET_DEVICE)),)

$(call add-radio-file,splash.img)

include $(call all-subdir-makefiles,$(LOCAL_PATH))

endif
