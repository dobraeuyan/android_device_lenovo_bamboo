#
# Copyright (C) 2012 The CyanogenMod Project
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
#

# Boot animation
TARGET_SCREEN_HEIGHT := 800
TARGET_SCREEN_WIDTH := 1280

# Release name
PRODUCT_RELEASE_NAME := bamboo

# Inherit some common Lineage stuff.
$(call inherit-product, vendor/cm/config/common_full_tablet_wifionly.mk)

# Inherit device configuration
$(call inherit-product, device/lenovo/bamboo/device_bamboo.mk)

# Device identifier. This must come after all inclusions
PRODUCT_NAME := lineage_bamboo
PRODUCT_BRAND := lenovo
PRODUCT_DEVICE := bamboo
PRODUCT_MODEL := YT3-X50F
PRODUCT_MANUFACTURER := lenovo

PRODUCT_BUILD_PROP_OVERRIDES += \
    BUILD_FINGERPRINT=Lenovo/YT3-850F/YT3-850F:6.0.1/MMB29M/YT3-850F_USR_S128_220828_Q1241_ROW:user/release-keys \
	PRIVATE_BUILD_DESC="msm8909-user 6.0.1 MMB29M YT3-850F_USR_S128_220828_Q1241_ROW release-keys"
