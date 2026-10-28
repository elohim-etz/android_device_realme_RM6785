/*
 * Copyright (C) 2024 The LineageOS Project
 *               2024 Paranoid Android
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "Fingerprint.h"

#include <android-base/logging.h>
#include <android-base/properties.h>

using ::android::hardware::hidl_string;
using ::android::hardware::Return;
using ::android::hardware::Void;
using IOplusFingerprintClientCallback = ::vendor::oplus::hardware::biometrics::fingerprint::V2_1::
        IBiometricsFingerprintClientCallback;

namespace aidl::android::hardware::biometrics::fingerprint {

namespace {
constexpr int SENSOR_ID = 0;
constexpr common::SensorStrength SENSOR_STRENGTH = common::SensorStrength::STRONG;
constexpr int MAX_ENROLLMENTS_PER_USER = 5;

// Side sensor on the power button, matches config_sfps_sensor_props
constexpr int SENSOR_LOCATION_X = 1080;
constexpr int SENSOR_LOCATION_Y = 880;
constexpr int SENSOR_RADIUS = 115;

constexpr char HW_COMPONENT_ID[] = "fingerprintSensor";
constexpr char SW_COMPONENT_ID[] = "matchingAlgorithm";

// Set by the stock service from /proc/fp_id, "F_1541" (FPC1541) or "G_3626" (Goodix GF3626)
constexpr char FP_ID_PROP[] = "persist.vendor.fingerprint.fp_id";

std::string getHardwareVersion() {
    auto fpId = ::android::base::GetProperty(FP_ID_PROP, "");
    if (fpId == "F_1541") return "fpc/fpc1541";
    if (fpId == "G_3626") return "goodix/gf3626";
    return "unknown/" + fpId;
}
}  // namespace

class Fingerprint::OplusCallback : public IOplusFingerprintClientCallback {
  public:
    OplusCallback(Fingerprint* fingerprint) : mFingerprint(fingerprint) {}

    Return<void> onEnrollResult(uint64_t /*deviceId*/, uint32_t fingerId, uint32_t /*groupId*/,
                                uint32_t remaining) override {
        if (auto session = getSession()) session->onEnrollResult(fingerId, remaining);
        return Void();
    }

    Return<void> onAcquired(uint64_t /*deviceId*/, OplusAcquiredInfo acquiredInfo,
                            int32_t vendorCode) override {
        if (auto session = getSession()) session->onAcquired(acquiredInfo, vendorCode);
        return Void();
    }

    Return<void> onAuthenticated(uint64_t /*deviceId*/, uint32_t fingerId, uint32_t /*groupId*/,
                                 const hidl_vec<uint8_t>& token) override {
        if (auto session = getSession()) session->onAuthenticated(fingerId, token);
        return Void();
    }

    Return<void> onError(uint64_t /*deviceId*/, OplusError error, int32_t vendorCode) override {
        if (auto session = getSession()) session->onError(error, vendorCode);
        return Void();
    }

    Return<void> onRemoved(uint64_t /*deviceId*/, uint32_t fingerId, uint32_t /*groupId*/,
                           uint32_t /*remaining*/) override {
        if (auto session = getSession()) session->onRemoved(fingerId);
        return Void();
    }

    // Never sent by the stock service, enrollments come through onSyncTemplates
    Return<void> onEnumerate(uint64_t /*deviceId*/, uint32_t /*fingerId*/, uint32_t /*groupId*/,
                             uint32_t /*remaining*/) override {
        return Void();
    }

    Return<void> onSyncTemplates(uint64_t /*deviceId*/, const hidl_vec<uint32_t>& fingerId,
                                 uint32_t /*remaining*/) override {
        if (auto session = getSession()) session->onSyncTemplates(fingerId);
        return Void();
    }

    // Optical sensor, engineering mode and DCS reports, unused on RM6785
    Return<void> onTouchDown(uint64_t /*deviceId*/) override { return Void(); }
    Return<void> onTouchUp(uint64_t /*deviceId*/) override { return Void(); }
    Return<void> onMonitorEventTriggered(uint32_t /*type*/, const hidl_string& /*data*/) override {
        return Void();
    }
    Return<void> onImageInfoAcquired(uint32_t /*type*/, uint32_t /*quality*/,
                                     uint32_t /*match_score*/) override {
        return Void();
    }
    Return<void> onEngineeringInfoUpdated(uint32_t /*length*/, const hidl_vec<uint32_t>& /*keys*/,
                                          const hidl_vec<hidl_string>& /*values*/) override {
        return Void();
    }
    Return<void> onFingerprintCmd(int32_t /*deviceId*/, const hidl_vec<int8_t>& /*groupId*/,
                                  uint32_t /*remaining*/) override {
        return Void();
    }

  private:
    std::shared_ptr<Session> getSession() {
        auto session = mFingerprint->mSession;
        if (session == nullptr || session->isClosed()) {
            ALOGE("Receiving callbacks before a session is opened.");
            return nullptr;
        }
        return session;
    }

    Fingerprint* mFingerprint;
};

Fingerprint::Fingerprint() {
    mDevice = IOplusFingerprint::getService();
    CHECK(mDevice != nullptr) << "Can't get the vendor.oplus fingerprint service";

    mOplusCallback = new OplusCallback(this);
    auto ret = mDevice->setNotify(mOplusCallback);
    CHECK(ret.isOk()) << "Can't register the vendor.oplus fingerprint callback";
}

Fingerprint::~Fingerprint() = default;

ndk::ScopedAStatus Fingerprint::getSensorProps(std::vector<SensorProps>* out) {
    std::vector<common::ComponentInfo> componentInfo = {
            {HW_COMPONENT_ID, getHardwareVersion(), "" /* firmwareVersion */,
             "" /* serialNumber */, "" /* softwareVersion */},
            {SW_COMPONENT_ID, "" /* hardwareVersion */, "" /* firmwareVersion */,
             "" /* serialNumber */, "oplus/fingerprint@2.1"}};

    common::CommonProps commonProps = {SENSOR_ID, SENSOR_STRENGTH, MAX_ENROLLMENTS_PER_USER,
                                       componentInfo};

    std::vector<SensorLocation> sensorLocations = {{.sensorLocationX = SENSOR_LOCATION_X,
                                                    .sensorLocationY = SENSOR_LOCATION_Y,
                                                    .sensorRadius = SENSOR_RADIUS}};

    *out = {{commonProps, FingerprintSensorType::POWER_BUTTON, sensorLocations,
             false /* supportsNavigationGestures */, true /* supportsDetectInteraction */,
             false /* halHandlesDisplayTouches */, false /* halControlsIllumination */,
             std::nullopt /* touchDetectionParameters */}};
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Fingerprint::createSession(int32_t /*sensorId*/, int32_t userId,
                                              const std::shared_ptr<ISessionCallback>& cb,
                                              std::shared_ptr<ISession>* out) {
    CHECK(mSession == nullptr || mSession->isClosed()) << "Open session already exists!";

    mSession = SharedRefBase::make<Session>(mDevice, userId, cb, mLockoutTracker);
    *out = mSession;

    mSession->linkToDeath(cb->asBinder().get());

    return ndk::ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::biometrics::fingerprint
