LOCAL_PATH := $(call my-dir)

ifeq ($(TARGET_DEVICE),bamboo)

include $(call all-makefiles-under,$(LOCAL_PATH))

include $(CLEAR_VARS)

WCNSS_CFG_INI := $(TARGET_OUT_ETC)/firmware/wlan/prima/WCNSS_qcom_cfg.ini
$(WCNSS_CFG_INI): $(LOCAL_INSTALLED_MODULE)
	@echo "WCNSS_qcom_cfg.ini Firmware link: $@"
	@mkdir -p $(dir $@)
	@rm -rf $@
	$(hide) ln -sf /data/misc/wifi/$(notdir $@) $@

PERSIST_NV := $(TARGET_OUT_ETC)/firmware/wlan/prima/WCNSS_qcom_wlan_nv.bin
$(PERSIST_NV): $(LOCAL_INSTALLED_MODULE)
	@echo "WCNSS_qcom_wlan_nv.bin  Firmware link: $@"
	@mkdir -p $(dir $@)
	@rm -rf $@
	$(hide) ln -sf /persist/$(notdir $@) $@
	
PERSIST_DICT := $(TARGET_OUT_ETC)/firmware/wlan/prima/WCNSS_wlan_dictionary.dat
$(PERSIST_DICT): $(LOCAL_INSTALLED_MODULE)
	@echo "WCNSS_wlan_dictionary.dat Firmware link: $@"
	@mkdir -p $(dir $@)
	@rm -rf $@
	$(hide) ln -sf /persist/$(notdir $@) $@

ALL_DEFAULT_INSTALLED_MODULES += $(WCNSS_CFG_INI) $(PERSIST_NV) $(PERSIST_DICT) 

endif
