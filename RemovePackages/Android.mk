LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := RemovePackages
LOCAL_MODULE_CLASS := APPS
LOCAL_MODULE_TAGS := optional
LOCAL_OVERRIDES_PACKAGES := \
          arcore \
          AdaptiveVPNPrebuilt \
          AmbientSensePrebuilt \
          AppDirectedSMSService \
          ConnMO \
          CalendarGooglePrebuilt \
          CarrierMetrics \
          Chrome \
          Drive \
          DCMO \
          DevicePolicyPrebuilt \
          DMService \
          FM2 \
          GCS \
          GoogleCamera \
          MaestroPrebuilt \
          Maps \
          MeetPrebuilt \
          MicropaperPrebuilt \
          MyVerizonServices \
          OBDM_Permissions \
          obdm_stub \
          Papers \
          Photos \
          PixelWallpapers2021 \
          PixelWallpapers2020 \
          PixelLiveWallpaperPrebuilt \
          RecorderPrebuilt \
          SafetyHubPrebuilt \
          SCONE \
          ScribePrebuilt \
          Showcase \
          Snap \
          SoundAmplifierPrebuilt \
          SprintDM \
          SprintHM \
          TurboPrebuilt \
          Tycho \
          TipsPrebuilt \
          talkback \
          USCCDM \
          VZWAPNLib \
          VzwOmaTrigger \
          WallpapersBReel2020 \
          YouTube \
          YouTubeMusicPrebuilt

LOCAL_UNINSTALLABLE_MODULE := true
LOCAL_CERTIFICATE := platform
LOCAL_SRC_FILES := /dev/null
include $(BUILD_PREBUILT)
