TARGET_HAS_FM_RADIO := false

$(call inherit-product, device/qcom/msm8909-common/msm8909.mk)

$(call inherit-product, vendor/lenovo/bamboo/bamboo-vendor.mk)

TARGET_SCREEN_HEIGHT := 1280
TARGET_SCREEN_WIDTH := 800

$(call inherit-product-if-exists, frameworks/native/build/tablet-7in-hdpi-1024-dalvik-heap.mk)

PRODUCT_PACKAGES += \
    android.hardware.gatekeeper@1.0-impl \
    android.hardware.keymaster@3.0-impl \
    android.hardware.keymaster@3.0-service \
    keystore.msm8909

DEVICE_PACKAGE_OVERLAYS += device/lenovo/bamboo/overlay

PRODUCT_PACKAGES += \
    BambooOverlayFrameworksRes \
    BambooOverlaySystemUI \
    BambooOverlayLineageSDK

PRODUCT_COPY_FILES += \
    device/lenovo/bamboo/audio/audio_platform_info.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio_platform_info.xml \
    device/lenovo/bamboo/audio/mixer_paths_skue.xml:$(TARGET_COPY_OUT_VENDOR)/etc/mixer_paths_skue.xml

PRODUCT_COPY_FILES += \
    device/lenovo/bamboo/rootdir/etc/fstab.qcom:$(TARGET_COPY_OUT_VENDOR)/etc/fstab.qcom \
    device/lenovo/bamboo/rootdir/etc/init.device.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.device.rc
