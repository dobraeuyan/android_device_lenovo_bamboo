# Copyright (C) 2015 The CyanogenMod Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := camera_shim.cpp
LOCAL_MODULE := camera.msm8909
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MULTILIB := 32
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_CFLAGS := -Werror -Wno-unused-parameter -Wno-missing-field-initializers
LOCAL_SHARED_LIBRARIES := \
    liblog \
    libcutils \
    libutils \
    libhardware \
    libcamera_client
LOCAL_C_INCLUDES += \
    system/media/camera/include \
    hardware/libhardware/include
include $(BUILD_SHARED_LIBRARY)
