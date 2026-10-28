/*
 * Copyright (C) 2024 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/stat.h>
#include <cerrno>
#include <cstring>
#include <format>
#include <thread>

#include "Legacy2Aidl.h"
#include "Session.h"

#include "CancellationSignal.h"

using ::android::hardware::hidl_array;

namespace aidl::android::hardware::biometrics::fingerprint {

namespace {
// Same location the framework hands to HIDL fingerprint HALs, templates
// enrolled with the old HIDL HAL stay valid.
constexpr char FPDATA_PATH[] = "/data/vendor_de/{}/fpdata";
}  // namespace

void onClientDeath(void* cookie) {
    ALOGI("FingerprintService has died");
    Session* session = static_cast<Session*>(cookie);
    if (session && !session->isClosed()) {
        session->close();
    }
}

Session::Session(sp<IOplusFingerprint> device, int userId, std::shared_ptr<ISessionCallback> cb,
                 LockoutTracker lockoutTracker)
    : mDevice(device), mLockoutTracker(lockoutTracker), mUserId(userId), mCb(cb) {
    mDeathRecipient = AIBinder_DeathRecipient_new(onClientDeath);

    // The stock service refuses a store path it can't write to, and only the
    // framework's HIDL client used to create it.
    auto path = std::format(FPDATA_PATH, userId);
    if (mkdir(path.c_str(), 0700) != 0 && errno != EEXIST) {
        ALOGE("Can't create %s: %s", path.c_str(), strerror(errno));
    }
    auto ret = mDevice->setActiveGroup(mUserId, path);
    if (!ret.isOk() || ret != OplusRequestStatus::SYS_OK) {
        ALOGE("setActiveGroup failed for %s", path.c_str());
    }
}

ndk::ScopedAStatus Session::generateChallenge() {
    uint64_t challenge = mDevice->preEnroll();
    ALOGI("generateChallenge: %ld", challenge);
    mCb->onChallengeGenerated(challenge);

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::revokeChallenge(int64_t challenge) {
    ALOGI("revokeChallenge: %ld", challenge);
    mDevice->postEnroll();
    mCb->onChallengeRevoked(challenge);

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::enroll(const HardwareAuthToken& hat,
                                   std::shared_ptr<ICancellationSignal>* out) {
    hw_auth_token_t authToken;
    translate(hat, authToken);
    hidl_array<uint8_t, sizeof(hw_auth_token_t)> token;
    memcpy(token.data(), &authToken, sizeof(authToken));

    OplusRequestStatus error = mDevice->enroll(token, mUserId, 60);
    if (error != OplusRequestStatus::SYS_OK) {
        ALOGE("enroll failed: %d", static_cast<int32_t>(error));
        mCb->onError(Error::UNABLE_TO_PROCESS, static_cast<int32_t>(error));
    }

    *out = SharedRefBase::make<CancellationSignal>(this);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::authenticate(int64_t operationId,
                                         std::shared_ptr<ICancellationSignal>* out) {
    *out = SharedRefBase::make<CancellationSignal>(this);

    // The operation ends with the lockout, don't let the stock service match
    if (checkSensorLockout()) {
        return ndk::ScopedAStatus::ok();
    }

    OplusRequestStatus error = mDevice->authenticate(operationId, mUserId);
    if (error != OplusRequestStatus::SYS_OK) {
        ALOGE("authenticate failed: %d", static_cast<int32_t>(error));
        mCb->onError(Error::UNABLE_TO_PROCESS, static_cast<int32_t>(error));
    }

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::detectInteraction(std::shared_ptr<ICancellationSignal>* out) {
    // The stock service has no detect mode, run a regular authentication and
    // report any finger on the sensor, matched or not, as an interaction.
    mDetecting = true;
    OplusRequestStatus error = mDevice->authenticate(0 /* operationId */, mUserId);
    if (error != OplusRequestStatus::SYS_OK) {
        ALOGE("detectInteraction failed: %d", static_cast<int32_t>(error));
        mDetecting = false;
        mCb->onError(Error::UNABLE_TO_PROCESS, static_cast<int32_t>(error));
    }

    *out = SharedRefBase::make<CancellationSignal>(this);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::enumerateEnrollments() {
    {
        std::lock_guard<std::mutex> lock(mTemplatesLock);
        mEnumeratePending = true;
    }

    OplusRequestStatus error = mDevice->enumerate();
    if (error != OplusRequestStatus::SYS_OK) {
        ALOGE("enumerate failed: %d", static_cast<int32_t>(error));
        std::lock_guard<std::mutex> lock(mTemplatesLock);
        mEnumeratePending = false;
        mCb->onEnrollmentsEnumerated(mTemplates);
    }

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::removeEnrollments(const std::vector<int32_t>& enrollmentIds) {
    ALOGI("removeEnrollments, size: %zu", enrollmentIds.size());

    {
        std::lock_guard<std::mutex> lock(mRemovalLock);
        mPendingRemoval = enrollmentIds;
        mRemoved.clear();
    }

    for (int32_t fid : enrollmentIds) {
        OplusRequestStatus error = mDevice->remove(mUserId, fid);
        if (error != OplusRequestStatus::SYS_OK) {
            ALOGE("remove failed: %d", static_cast<int32_t>(error));
        }
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::getAuthenticatorId() {
    uint64_t auth_id = mDevice->getAuthenticatorId();
    ALOGI("getAuthenticatorId: %ld", auth_id);
    mCb->onAuthenticatorIdRetrieved(auth_id);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::invalidateAuthenticatorId() {
    uint64_t auth_id = mDevice->getAuthenticatorId();
    ALOGI("invalidateAuthenticatorId: %ld", auth_id);
    mCb->onAuthenticatorIdInvalidated(auth_id);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::resetLockout(const HardwareAuthToken& /*hat*/) {
    clearLockout(true);
    if (mIsLockoutTimerStarted) mIsLockoutTimerAborted = true;

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::onPointerDown(int32_t /*pointerId*/, int32_t /*x*/, int32_t /*y*/,
                                          float /*minor*/, float /*major*/) {
    // Not used by power button sensors
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::onPointerUp(int32_t /*pointerId*/) {
    // Not used by power button sensors
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::onUiReady() {
    // Not used by power button sensors
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::authenticateWithContext(
        int64_t operationId, const common::OperationContext& /*context*/,
        std::shared_ptr<common::ICancellationSignal>* out) {
    return authenticate(operationId, out);
}

ndk::ScopedAStatus Session::enrollWithContext(const keymaster::HardwareAuthToken& hat,
                                              const common::OperationContext& /*context*/,
                                              std::shared_ptr<common::ICancellationSignal>* out) {
    return enroll(hat, out);
}

ndk::ScopedAStatus Session::detectInteractionWithContext(
        const common::OperationContext& /*context*/,
        std::shared_ptr<common::ICancellationSignal>* out) {
    return detectInteraction(out);
}

ndk::ScopedAStatus Session::onPointerDownWithContext(const PointerContext& context) {
    return onPointerDown(context.pointerId, context.x, context.y, context.minor, context.major);
}

ndk::ScopedAStatus Session::onPointerUpWithContext(const PointerContext& context) {
    return onPointerUp(context.pointerId);
}

ndk::ScopedAStatus Session::onContextChanged(const common::OperationContext& /*context*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::onPointerCancelWithContext(const PointerContext& /*context*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::setIgnoreDisplayTouches(bool /*shouldIgnore*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::cancel() {
    mDetecting = false;
    OplusRequestStatus ret = mDevice->cancel();

    // Only the Goodix path of the stock service reports ERROR_CANCELED, the
    // FPC one stays silent. Always report it here and drop the vendor one in
    // onError so the framework gets exactly one.
    if (ret == OplusRequestStatus::SYS_OK) {
        mCb->onError(Error::CANCELED, 0 /* vendorCode */);
        return ndk::ScopedAStatus::ok();
    }

    return ndk::ScopedAStatus::fromServiceSpecificError(static_cast<int32_t>(ret));
}

ndk::ScopedAStatus Session::close() {
    mClosed = true;
    mCb->onSessionClosed();
    AIBinder_DeathRecipient_delete(mDeathRecipient);
    return ndk::ScopedAStatus::ok();
}

binder_status_t Session::linkToDeath(AIBinder* binder) {
    return AIBinder_linkToDeath(binder, mDeathRecipient, this);
}

bool Session::isClosed() {
    return mClosed;
}

// Translate errors reported by the stock service to AIDL-compliant Error.
// The service already strips FINGERPRINT_ERROR_VENDOR_BASE from vendor codes.
Error Session::VendorErrorFilter(OplusError error, int32_t vendorCode, int32_t* outVendorCode) {
    *outVendorCode = 0;

    switch (error) {
        case OplusError::ERROR_HW_UNAVAILABLE:
            return Error::HW_UNAVAILABLE;
        case OplusError::ERROR_UNABLE_TO_PROCESS:
            return Error::UNABLE_TO_PROCESS;
        case OplusError::ERROR_TIMEOUT:
            return Error::TIMEOUT;
        case OplusError::ERROR_NO_SPACE:
            return Error::NO_SPACE;
        case OplusError::ERROR_CANCELED:
            return Error::CANCELED;
        case OplusError::ERROR_UNABLE_TO_REMOVE:
            return Error::UNABLE_TO_REMOVE;
        case OplusError::ERROR_LOCKOUT: {
            *outVendorCode = static_cast<int32_t>(OplusError::ERROR_LOCKOUT);
            return Error::VENDOR;
        }
        case OplusError::ERROR_VENDOR: {
            *outVendorCode = vendorCode;
            return Error::VENDOR;
        }
        default:
            break;
    }
    ALOGE("Unknown error from fingerprint service: %d", static_cast<int32_t>(error));
    return Error::UNABLE_TO_PROCESS;
}

// Translate acquired messages reported by the stock service to AIDL-compliant
// AcquiredInfo. The service strips ACQUIRED_VENDOR_BASE from vendor codes,
// except for its own ACQUIRED_TOO_SIMILAR and ACQUIRED_ALREADY_ENROLLED.
AcquiredInfo Session::VendorAcquiredFilter(OplusAcquiredInfo info, int32_t vendorCode,
                                           int32_t* outVendorCode) {
    constexpr int32_t vendorBase = static_cast<int32_t>(OplusAcquiredInfo::ACQUIRED_VENDOR_BASE);
    *outVendorCode = 0;

    switch (info) {
        case OplusAcquiredInfo::ACQUIRED_GOOD:
            return AcquiredInfo::GOOD;
        case OplusAcquiredInfo::ACQUIRED_PARTIAL:
            return AcquiredInfo::PARTIAL;
        case OplusAcquiredInfo::ACQUIRED_INSUFFICIENT:
            return AcquiredInfo::INSUFFICIENT;
        case OplusAcquiredInfo::ACQUIRED_IMAGER_DIRTY:
            return AcquiredInfo::SENSOR_DIRTY;
        case OplusAcquiredInfo::ACQUIRED_TOO_SLOW:
            return AcquiredInfo::TOO_SLOW;
        case OplusAcquiredInfo::ACQUIRED_TOO_FAST:
            return AcquiredInfo::TOO_FAST;
        case OplusAcquiredInfo::ACQUIRED_VENDOR: {
            *outVendorCode = vendorCode;
            return AcquiredInfo::VENDOR;
        }
        case OplusAcquiredInfo::ACQUIRED_TOO_SIMILAR:
            // Same area of the finger captured again during enrollment
            return AcquiredInfo::IMMOBILE;
        default:
            // Includes ACQUIRED_ALREADY_ENROLLED, vendor code 2
            if (static_cast<int32_t>(info) >= vendorBase) {
                *outVendorCode = static_cast<int32_t>(info) - vendorBase;
                return AcquiredInfo::VENDOR;
            }
    }
    ALOGE("Unknown acquired message from fingerprint service: %d", static_cast<int32_t>(info));
    return AcquiredInfo::UNKNOWN;
}

bool Session::checkSensorLockout() {
    LockoutTracker::LockoutMode lockoutMode = mLockoutTracker.getMode();
    if (lockoutMode == LockoutTracker::LockoutMode::kPermanent) {
        ALOGE("Fail: lockout permanent");
        mCb->onLockoutPermanent();
        mIsLockoutTimerAborted = true;
        return true;
    }
    if (lockoutMode == LockoutTracker::LockoutMode::kTimed) {
        int64_t timeLeft = mLockoutTracker.getLockoutTimeLeft();
        ALOGE("Fail: lockout timed: %ld", timeLeft);
        mCb->onLockoutTimed(timeLeft);
        if (!mIsLockoutTimerStarted) startLockoutTimer(timeLeft);
        return true;
    }
    return false;
}

void Session::clearLockout(bool clearAttemptCounter) {
    mLockoutTracker.reset(clearAttemptCounter);
    mCb->onLockoutCleared();
}

void Session::startLockoutTimer(int64_t timeout) {
    std::function<void()> action = std::bind(&Session::lockoutTimerExpired, this);
    std::thread([timeout, action]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(timeout));
        action();
    }).detach();

    mIsLockoutTimerStarted = true;
}

void Session::lockoutTimerExpired() {
    if (!mIsLockoutTimerAborted) clearLockout(false);

    mIsLockoutTimerStarted = false;
    mIsLockoutTimerAborted = false;
}

void Session::onEnrollResult(uint32_t fid, uint32_t remaining) {
    ALOGD("onEnrollResult(fid=%d, rem=%d)", fid, remaining);
    mCb->onEnrollmentProgress(fid, remaining);
}

void Session::onAcquired(OplusAcquiredInfo info, int32_t vendorCode) {
    int32_t outVendorCode = 0;
    AcquiredInfo result = VendorAcquiredFilter(info, vendorCode, &outVendorCode);
    ALOGD("onAcquired(%hhd, %d)", result, outVendorCode);
    mCb->onAcquired(result, outVendorCode);
}

void Session::onAuthenticated(uint32_t fid, const hidl_vec<uint8_t>& token) {
    ALOGD("onAuthenticated(fid=%d)", fid);
    if (mDetecting.exchange(false)) {
        mCb->onInteractionDetected();
        mDevice->cancel();
        return;
    }

    if (fid != 0 && token.size() == sizeof(hw_auth_token_t)) {
        hw_auth_token_t hat;
        memcpy(&hat, token.data(), sizeof(hat));
        HardwareAuthToken authToken;
        translate(hat, authToken);

        mCb->onAuthenticationSucceeded(fid, authToken);
        mLockoutTracker.reset(true);
    } else {
        mCb->onAuthenticationFailed();
        mLockoutTracker.addFailedAttempt();
        // The operation ends with the lockout, stop the stock service from
        // matching further touches.
        if (checkSensorLockout()) {
            mDevice->cancel();
        }
    }
}

void Session::onError(OplusError error, int32_t vendorCode) {
    if (error == OplusError::ERROR_CANCELED) {
        // Already reported by cancel()
        return;
    }

    int32_t outVendorCode = 0;
    Error result = VendorErrorFilter(error, vendorCode, &outVendorCode);
    ALOGD("onError(%hhd, %d)", result, outVendorCode);
    mCb->onError(result, outVendorCode);
}

void Session::onRemoved(uint32_t fid) {
    ALOGD("onRemoved(fid=%d)", fid);

    std::lock_guard<std::mutex> lock(mRemovalLock);
    mRemoved.push_back(fid);
    if (mRemoved.size() >= mPendingRemoval.size()) {
        mCb->onEnrollmentsRemoved(mRemoved);
        mPendingRemoval.clear();
        mRemoved.clear();
    }
}

void Session::onSyncTemplates(const hidl_vec<uint32_t>& fids) {
    ALOGD("onSyncTemplates(size=%zu)", fids.size());

    std::lock_guard<std::mutex> lock(mTemplatesLock);
    mTemplates.assign(fids.begin(), fids.end());
    if (mEnumeratePending) {
        mEnumeratePending = false;
        mCb->onEnrollmentsEnumerated(mTemplates);
    }
}

}  // namespace aidl::android::hardware::biometrics::fingerprint
