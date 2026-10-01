#
# Copyright (C) 2021-2022 The LineageOS Project
#
# SPDX-License-Identifier: Apache-2.0
#

# Inherit from those products. Most specific first.
$(call inherit-product, $(SRC_TARGET_DIR)/product/core_64_bit.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base_telephony.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/non_ab_device.mk)

# Inherit from device makefile
$(call inherit-product, device/realme/RM6785/device.mk)

# Inherit some common Lineage stuff.
$(call inherit-product, vendor/infinity/config/common_full_phone.mk)

# InfinityX Flags
INFINITY_BUILD_TYPE := OFFICIAL
INFINITY_MAINTAINER := 𝙴𝙻𝙾𝙷𝙸𝙼
TARGET_SUPPORTS_BLUR := true
WITH_GAPPS := true
TARGET_SHIPS_FULL_GAPPS := false
TARGET_SHIPS_GOOGLE_DIALER := false
USE_MOTO_CALCULATOR := true
TARGET_SUPPORTS_QUICK_TAP := true
TARGET_FACE_UNLOCK_SUPPORTED := true

# Device identifier. This must come after all inclusions.
PRODUCT_NAME := infinity_RM6785
PRODUCT_DEVICE := RM6785
PRODUCT_BRAND := realme
PRODUCT_MODEL := RM6785
PRODUCT_MANUFACTURER := realme
PRODUCT_GMS_CLIENTID_BASE := android-oppo

PRODUCT_BUILD_PROP_OVERRIDES += \
    BuildDesc=$(call normalize-path-list, "sys_oplus_mssi_64_cn-user 11 RP1A.200720.011 1623809323039 release-keys")

    BUILD_FINGERPRINT := realme/RMX2002/RMX2002L1:11/RP1A.200720.011/1651754371157:user/release-keys

PRODUCT_PRODUCT_PROPERTIES += \
   ro.build.fingerprint=$(BUILD_FINGERPRINT)
