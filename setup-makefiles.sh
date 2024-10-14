#!/bin/bash
#
# Copyright (C) 2016 The CyanogenMod Project
# Copyright (C) 2017-2023 The LineageOS Project
#
# SPDX-License-Identifier: Apache-2.0
#

set -e

export DEVICE=RM6785
export VENDOR=realme

# Load extract_utils and do some sanity checks
MY_DIR="${BASH_SOURCE%/*}"
if [[ ! -d "${MY_DIR}" ]]; then MY_DIR="${PWD}"; fi

LINEAGE_ROOT="${MY_DIR}/../../.."

HELPER="${LINEAGE_ROOT}/tools/extract-utils/extract_utils.sh"
if [ ! -f "${HELPER}" ]; then
    echo "Unable to find helper script at ${HELPER}"
    exit 1
fi
source "${HELPER}"

function vendor_imports() {
    cat <<EOF >>"$1"
		"device/realme/RM6785",
		"hardware/mediatek",
		"hardware/mediatek/libmtkperf_client",
EOF
}

function lib_to_package_fixup_vendor_variants() {
    if [ "$2" != "vendor" ]; then
        return 1
    fi
    case "$1" in
        vendor.mediatek.hardware.videotelephony@1.0)
            echo "${1}_vendor"
            ;;
        *)
            return 1
            ;;
    esac
}
function lib_to_package_fixup() {
    lib_to_package_fixup_clang_rt_ubsan_standalone "$1" ||
        lib_to_package_fixup_proto_3_9_1 "$1" ||
        lib_to_package_fixup_vendor_variants "$@"
}

# Initialize the helper for device
setup_vendor "${DEVICE}" "${VENDOR}" "${LINEAGE_ROOT}"

# Copyright headers and common guards
write_headers

# The standard blobs
write_makefiles "${MY_DIR}/proprietary-files.txt" true

# Append the calls to firmware images
append_firmware_calls_to_makefiles "${MY_DIR}/proprietary-firmware.txt"

# Finish
write_footers
