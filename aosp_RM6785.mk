#
# Copyright (C) 2024 The LineageOS Project
#
# SPDX-License-Identifier: Apache-2.0
#

# Inherit from those products. Most specific first.
$(call inherit-product, $(SRC_TARGET_DIR)/product/core_64_bit.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base_telephony.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/non_ab_device.mk)

# Inherit from device makefile
$(call inherit-product, device/realme/RM6785/device.mk)

# Inherit some common PixelOS stuff.
$(call inherit-product, vendor/aosp/config/common_full_phone.mk)

# 2by2 Project Flags
CUSTOM_MAINTAINER := 𝙴𝙻𝙾𝙷𝙸𝙼
TARGET_USES_MINI_GAPPS := true
TARGET_INCLUDE_EXTRA_APPS := false
TARGET_USES_BLUR := false

# Boot animation
TARGET_SCREEN_HEIGHT := 2400
TARGET_SCREEN_WIDTH := 1080

PRODUCT_NAME := aosp_RM6785
PRODUCT_DEVICE := RM6785
PRODUCT_BRAND := realme
PRODUCT_MODEL := RM6785
PRODUCT_MANUFACTURER := realme
PRODUCT_GMS_CLIENTID_BASE := android-oppo

PRODUCT_BUILD_PROP_OVERRIDES += \
    BuildDesc="sys_oplus_mssi_64_cn-user 11 RP1A.200720.011 1623809323039 release-keys" \
    BuildFingerprint=realme/RMX2002/RMX2002L1:11/RP1A.200720.011/1651754371157:user/release-keys \
    DeviceProduct=RM6785 \