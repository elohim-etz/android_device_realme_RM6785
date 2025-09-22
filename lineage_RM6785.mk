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
$(call inherit-product, vendor/lineage/config/common_full_phone.mk)

# Axion Stuff
AXION_CAMERA_REAR_INFO := 64,8,2,2
AXION_CAMERA_FRONT_INFO := 16
AXION_MAINTAINER := 𝙴𝙻𝙾𝙷𝙸𝙼
AXION_PROCESSOR := MTK_Helio_G90T
PRODUCT_NO_CAMERA := false

# CPUsets configuration
AXION_CPU_BG := 0-3
AXION_CPU_FG := 0-7
AXION_CPU_LIMIT_BG := 0-1
AXION_ALL_CORES := 0-7
AXION_CPU_LIMIT_UI := 0-4
AXION_CPU_DISPLAY := 0-3

PERF_GOV_SUPPORTED := true
PERF_DEFAULT_GOV := schedutil

# LOS Prebuilts
TARGET_INCLUDES_LOS_PREBUILTS := true

# Blur
TARGET_ENABLE_BLUR := false

# BCR
TARGET_PREBUILT_BCR := false

# Boot animation
TARGET_BOOT_ANIMATION_RES := 1920

# Perf Activity Anim Override
PERF_ANIM_OVERRIDE := true

# Device identifier. This must come after all inclusions.
PRODUCT_NAME := lineage_RM6785
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
