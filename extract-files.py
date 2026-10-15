#!/usr/bin/env -S PYTHONPATH=../../../tools/extract-utils python3
#
# SPDX-FileCopyrightText: 2024 The LineageOS Project
# SPDX-License-Identifier: Apache-2.0
#

from extract_utils.fixups_blob import (
    blob_fixup,
    blob_fixups_user_type,
)
from extract_utils.fixups_lib import (
    lib_fixups,
    lib_fixups_user_type,
)
from extract_utils.main import (
    ExtractUtils,
    ExtractUtilsModule,
)

namespace_imports = [
    'device/realme/RM6785',
	'hardware/mediatek',
	'hardware/mediatek/libmtkperf_client',
	'hardware/mediatek/libaedv',
]


def lib_fixup_odm_suffix(lib: str, partition: str, *args, **kwargs):
    return f'{lib}_{partition}' if partition == 'odm' else None


def lib_fixup_vendor_suffix(lib: str, partition: str, *args, **kwargs):
    return f'{lib}_{partition}' if partition == 'vendor' else None


lib_fixups: lib_fixups_user_type = {
    **lib_fixups,
    (
        'vendor.oplus.hardware.biometrics.fingerprint@2.1',
    ): lib_fixup_odm_suffix,
    (
        'vendor.mediatek.hardware.videotelephony@1.0',
    ): lib_fixup_vendor_suffix,
}

blob_fixups: blob_fixups_user_type = {
    'system_ext/lib64/libsource.so': blob_fixup()
        .add_needed('libui_shim.so'),
    ('vendor/lib/libnvram.so',
    'vendor/lib/libsysenv.so',
    'vendor/lib64/libnvram.so',
    'vendor/lib64/libsysenv.so',
    'vendor/bin/hw/android.hardware.neuralnetworks@1.3-service-mtk-neuron'): blob_fixup()
        .add_needed('libbase_shim.so'),
    'vendor/lib64/hw/android.hardware.camera.provider@2.6-impl-mediatek.so': blob_fixup()
        .add_needed('libcamera_metadata_shim.so'),
    'vendor/lib64/lib3a.flash.so': blob_fixup()
        .add_needed('liblog.so'),
    'vendor/lib64/libcam.halsensor.so': blob_fixup()
        .add_needed('libshim_utils.so'),
   ('vendor/lib64/libutils-v30.so', 'vendor/lib64/hw/hwcomposer.mt6785.so'): blob_fixup()
        .add_needed('libprocessgroup_shim.so'),
    'vendor/lib/hw/audio.primary.mt6785.so': blob_fixup()
        .add_needed('libshim_audio.so')
        .replace_needed('libalsautils.so', 'libalsautils-v30.so'),
    'vendor/lib/libmnl.so': blob_fixup()
        .add_needed('libcutils.so'),
    'vendor/bin/hw/mtkfusionrild': blob_fixup()
        .add_needed('libutils-v32.so'),
    ('vendor/bin/mnld', 'vendor/lib/libcam.utils.sensorprovider.so', 'vendor/lib/libaalservice.so', 'vendor/lib64/libcam.utils.sensorprovider.so', 'vendor/lib64/libaalservice.so'): blob_fixup()
        .add_needed('android.hardware.sensors@1.0-convert-shared.so'),
    'system_ext/lib64/libimsma.so': blob_fixup()
        .replace_needed('libsink.so', 'libsink-mtk.so'),
    'system/priv-app/ImsService/ImsService.apk': blob_fixup()
        .apktool_patch('blob-patches/ImsService/'),
    ('vendor/lib64/libmtkcam_stdutils.so', 'vendor/lib64/hw/dfps.mt6785.so', 'vendor/lib64/hw/vendor.mediatek.hardware.pq@2.6-impl.so'): blob_fixup()
        .replace_needed('libutils.so', 'libutils-v30.so'),
    'vendor/lib64/libmtkcam_featurepolicy.so': blob_fixup()
        .binary_regex_replace(b'\x34\xE8\x87\x40\xB9', b'\x34\x28\x02\x80\x52'),
    'vendor/lib/libMtkOmxCore.so': blob_fixup()
        .binary_regex_replace(b'mtk.vendor.omx.core.log', b'ro.vendor.mtk.omx.log\x00\x00'),
    'vendor/lib/libaudio_param_parser-vnd.so': blob_fixup()
        .binary_regex_replace(b'\x00audio.tuning.def_path', b'\x00ro.vendor.tuning_path')
        .binary_regex_replace(b'\x20audio.tuning.def_path', b'\x20ro.vendor.tuning_path'),
    'vendor/lib64/hw/hwcomposer.mt6785.so': blob_fixup()
        .binary_regex_replace(
            b'\x68\x00\x00\x34\x20\x00\x80\x52\x04\x00\x00\x14\x68\x62\x6f\x39\x1f\x01\x00\x71\xe0\x07\x9f\x1a',
            b'\x20\x00\x80\x52\xa8\x00\x00\x35\x68\x42\x40\xb9\x08\x15\x10\x12\x1f\x81\x41\x71\xe0\x17\x9f\x1a',
        )
        .binary_regex_replace(
            b'\x08\x60\x6e\x39\x48\x16\x00\x34\x08\xc0\x45\xf9',
            b'\x08\x40\x40\xb9\x48\x16\x90\x36\x08\xc0\x45\xf9',
        )
        .binary_regex_replace(
            b'\x09\x15\x10\x12\\\x3f\x81\x41\x71\x61\x00\x00\x54',
            b'\x09\x15\x10\x12\x3f\x81\x41\x71\x81\x14\x00\x54',
        )
        .binary_regex_replace(
            b'\x00\xe4\x00\x6f\x23\xfd\xff\x90\\\x24\xfd\xff\xd0\x25\xfd\xff\x90',
            b'\x0b\x40\xa0\x52\x5f\x01\x0b\x6b\x49\x02\x80\x52\x80\xff\xff\x54',
        )
        .binary_regex_replace(
            b'\x08\x15\x10\x12\x1f\x81\x41\x71\x61\x00\x00\x54\x35\x00\x80\x52\x09\x00\x00\x14',
            b'\x08\x15\x10\x12\x1f\x81\x41\x71\x61\x00\x00\x54\x35\x00\x80\x52\x29\x00\x00\x14',
        ),

    'vendor/lib64/libdpframework.so': blob_fixup()
        .binary_regex_replace(
            b'\xe0\x03\x80\x3d\xc1\x00\x00\x54\x81\x0e\x80\x52',
            b'\xe0\x03\x80\x3d\x1f\x20\x03\xd5\x81\x0e\x80\x52',
        )
        .binary_regex_replace(
            b'\x7f\x01\x0a\x6a\x80\xfb\xff\x54\x0a\x00\x8a\x52',
            b'\x7f\x01\x0a\x6a\xe0\xfa\xff\x54\x0a\x00\x8a\x52',
        )
        .binary_regex_replace(
            b'\xd8\x42\x02\x94\xe0\x03\x00\x91\xda\x42\x02\x94',
            b'\x20\x00\x80\x52\x1f\x20\x03\xd5\x1f\x20\x03\xd5',
        ),

    'vendor/lib64/libgpu_aux.so': blob_fixup()
        .binary_regex_replace(
            b'\xff\x03\x04\xd1\xfd\x7b\x0d\xa9\xfd\x43\x03\x91\xf6\x57\x0e\xa9',
            b'\x00\x00\x80\x52\xc0\x03\x5f\xd6\xfd\x43\x03\x91\xf6\x57\x0e\xa9',
        )
        .binary_regex_replace(
            b'\x9f\x02\x00\x72\xa9\x00\x80\x52\\\x29\x15\x9f\x1a\xe0\x03\x1f\x2a\x69\x02\x00\xb9',
            b'\x9f\x02\x00\x72\xa9\x00\x80\x52\x1f\x20\x03\xd5\xe0\x03\x1f\x2a\x69\x02\x00\xb9',
        ),

    'vendor/lib/libgpu_aux.so': blob_fixup()
        .binary_regex_replace(
            b'\x2d\xe9\xf0\x43\xb1\xb0\x04\x46\x25\x48\x14\\\x29',
            b'\x00\x20\x70\x47\xb1\xb0\x04\x46\x25\x48\x14\x29',
        )
        .binary_regex_replace(
            b'\x01\x22\x00\\\x28\x18\xbf\x03\x22\x0f\xe0\x01\x22\x00\\\x28\x18\xbf\x05\x22\x0a\xe0',
            b'\x03\x22\x00\x28\x00\xbf\x03\x22\x0f\xe0\x05\x22\x00\x28\x00\xbf\x05\x22\x0a\xe0',
        ),
    'vendor/etc/libnfc-nxp.conf': blob_fixup()
        .regex_replace('NXPLOG_EXTNS_LOGLEVEL=0x03', 'NXPLOG_EXTNS_LOGLEVEL=0x01')
        .regex_replace('NXPLOG_NCIHAL_LOGLEVEL=0x03', 'NXPLOG_NCIHAL_LOGLEVEL=0x01')
        .regex_replace('NXPLOG_NCIX_LOGLEVEL=0x03', 'NXPLOG_NCIX_LOGLEVEL=0x01')
        .regex_replace('NXPLOG_NCIR_LOGLEVEL=0x03', 'NXPLOG_NCIR_LOGLEVEL=0x01')
        .regex_replace('NXPLOG_FWDNLD_LOGLEVEL=0x03', 'NXPLOG_FWDNLD_LOGLEVEL=0x01')
        .regex_replace('NXPLOG_TML_LOGLEVEL=0x03', 'NXPLOG_TML_LOGLEVEL=0x01')
        .regex_replace('NFC_DEBUG_ENABLED=0x01', 'NFC_DEBUG_ENABLED=0x00'),
    'vendor/bin/hw/android.hardware.media.c2@1.2-mediatek-64b': blob_fixup()
        .replace_needed('libavservices_minijail_vendor.so', 'libavservices_minijail.so')
        .replace_needed('libcodec2_hidl@1.0.so', 'libcodec2_hidl@1.0-v31.so')
        .replace_needed('libcodec2_hidl@1.1.so', 'libcodec2_hidl@1.1-v31.so')
        .replace_needed('libcodec2_hidl@1.2.so', 'libcodec2_hidl@1.2-v31.so')
        .replace_needed('libcodec2_vndk.so', 'libcodec2_vndk-v31.so'),

    'vendor/lib64/libcodec2_hidl@1.0-v31.so': blob_fixup()
        .replace_needed('libstagefright_bufferqueue_helper.so', 'libstagefright_bufferqueue_helper-v33.so')
        .replace_needed('libcodec2_hidl_plugin.so', 'libcodec2_hidl_plugin-v31.so')
        .replace_needed('libcodec2_vndk.so', 'libcodec2_vndk-v31.so')
        .replace_needed('libui.so', 'libui-v34.so')
        .add_needed('libbase_shim.so'),

    'vendor/lib64/libcodec2_hidl@1.1-v31.so': blob_fixup()
        .replace_needed('libstagefright_bufferqueue_helper.so', 'libstagefright_bufferqueue_helper-v33.so')
        .replace_needed('libcodec2_hidl@1.0.so', 'libcodec2_hidl@1.0-v31.so')
        .replace_needed('libcodec2_hidl_plugin.so', 'libcodec2_hidl_plugin-v31.so')
        .replace_needed('libcodec2_vndk.so', 'libcodec2_vndk-v31.so')
        .replace_needed('libui.so', 'libui-v34.so')
        .add_needed('libbase_shim.so'),

    'vendor/lib64/libcodec2_hidl@1.2-v31.so': blob_fixup()
        .replace_needed('libstagefright_bufferqueue_helper.so', 'libstagefright_bufferqueue_helper-v33.so')
        .replace_needed('libcodec2_hidl@1.0.so', 'libcodec2_hidl@1.0-v31.so')
        .replace_needed('libcodec2_hidl@1.1.so', 'libcodec2_hidl@1.1-v31.so')
        .replace_needed('libcodec2_hidl_plugin.so', 'libcodec2_hidl_plugin-v31.so')
        .replace_needed('libcodec2_vndk.so', 'libcodec2_vndk-v31.so')
        .replace_needed('libui.so', 'libui-v34.so')
        .add_needed('libbase_shim.so'),

    'vendor/lib64/libcodec2_hidl_plugin-v31.so': blob_fixup()
        .replace_needed('libcodec2_vndk.so', 'libcodec2_vndk-v31.so'),

    ('vendor/lib64/libcodec2_mtk_c2store.so',
        'vendor/lib64/libcodec2_vpp_qt_plugin.so',
        'vendor/lib64/libcodec2_vpp_rs_plugin.so'): blob_fixup()
        .replace_needed('libcodec2_soft_common.so', 'libcodec2_soft_common-v31.so')
        .replace_needed('libcodec2_vndk.so', 'libcodec2_vndk-v31.so')
        .replace_needed('libstagefright_foundation.so', 'libstagefright_foundation-v33.so')
        .replace_needed('libsfplugin_ccodec_utils.so', 'libsfplugin_ccodec_utils-v31.so'),

    ('vendor/lib64/libcodec2_mtk_vdec.so',
        'vendor/lib64/libcodec2_mtk_venc.so'): blob_fixup()
        .replace_needed('libcodec2_soft_common.so', 'libcodec2_soft_common-v31.so')
        .replace_needed('libcodec2_vndk.so', 'libcodec2_vndk-v31.so')
        .replace_needed('libformatter.so', 'libformatter_mtk.so')
        .replace_needed('libstagefright_foundation.so', 'libstagefright_foundation-v33.so')
        .replace_needed('libsfplugin_ccodec_utils.so', 'libsfplugin_ccodec_utils-v31.so')
        .replace_needed('libui.so', 'libui-v34.so'),

    'vendor/lib64/libcodec2_soft_common-v31.so': blob_fixup()
        .replace_needed('libcodec2_vndk.so', 'libcodec2_vndk-v31.so')
        .replace_needed('libstagefright_foundation.so', 'libstagefright_foundation-v33.so')
        .replace_needed('libsfplugin_ccodec_utils.so', 'libsfplugin_ccodec_utils-v31.so'),

    'vendor/lib64/libcodec2_vndk-v31.so': blob_fixup()
        .replace_needed('libui.so', 'libui-v34.so')
        .replace_needed('libstagefright_foundation.so', 'libstagefright_foundation-v33.so'),

    'vendor/lib64/libsfplugin_ccodec_utils-v31.so': blob_fixup()
        .replace_needed('libcodec2_vndk.so', 'libcodec2_vndk-v31.so'),

    'vendor/etc/vintf/manifest/manifest_media_c2_V1_2_default.xml': blob_fixup()
        .regex_replace('1.1', '1.2'),

}  # fmt: skip

module = ExtractUtilsModule(
    'RM6785',
    'realme',
    blob_fixups=blob_fixups,
    lib_fixups=lib_fixups,
    namespace_imports=namespace_imports,
    add_firmware_proprietary_file=True,
)

if __name__ == '__main__':
    utils = ExtractUtils.device(module)
    utils.run()
