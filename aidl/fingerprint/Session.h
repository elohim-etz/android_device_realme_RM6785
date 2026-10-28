/*
 * Copyright (C) 2024 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once
#define LOG_TAG "android.hardware.biometrics.fingerprint-service.RM6785"

#include <aidl/android/hardware/biometrics/fingerprint/BnSession.h>
#include <aidl/android/hardware/biometrics/fingerprint/ISessionCallback.h>
#include <android/log.h>
#include <log/log.h>
#include <vendor/oplus/hardware/biometrics/fingerprint/2.1/IBiometricsFingerprint.h>

#include <atomic>
#include <mutex>

#include "LockoutTracker.h"

using ::aidl::android::hardware::biometrics::common::ICancellationSignal;
using ::aidl::android::hardware::biometrics::common::OperationContext;
using ::aidl::android::hardware::biometrics::fingerprint::PointerContext;
using ::aidl::android::hardware::keymaster::HardwareAuthToken;
using ::android::sp;
using ::android::hardware::hidl_vec;

using IOplusFingerprint = ::vendor::oplus::hardware::biometrics::fingerprint::V2_1::IBiometricsFingerprint;
using OplusAcquiredInfo = ::vendor::oplus::hardware::biometrics::fingerprint::V2_1::FingerprintAcquiredInfo;
using OplusError = ::vendor::oplus::hardware::biometrics::fingerprint::V2_1::FingerprintError;
using OplusRequestStatus = ::vendor::oplus::hardware::biometrics::fingerprint::V2_1::RequestStatus;

namespace aidl::android::hardware::biometrics::fingerprint {

void onClientDeath(void* cookie);

class Session : public BnSession {
  public:
    Session(sp<IOplusFingerprint> device, int userId, std::shared_ptr<ISessionCallback> cb,
            LockoutTracker lockoutTracker);
    ndk::ScopedAStatus generateChallenge() override;
    ndk::ScopedAStatus revokeChallenge(int64_t challenge) override;
    ndk::ScopedAStatus enroll(const HardwareAuthToken& hat,
                              std::shared_ptr<ICancellationSignal>* out) override;
    ndk::ScopedAStatus authenticate(int64_t operationId,
                                    std::shared_ptr<ICancellationSignal>* out) override;
    ndk::ScopedAStatus detectInteraction(std::shared_ptr<ICancellationSignal>* out) override;
    ndk::ScopedAStatus enumerateEnrollments() override;
    ndk::ScopedAStatus removeEnrollments(const std::vector<int32_t>& enrollmentIds) override;
    ndk::ScopedAStatus getAuthenticatorId() override;
    ndk::ScopedAStatus invalidateAuthenticatorId() override;
    ndk::ScopedAStatus resetLockout(const HardwareAuthToken& hat) override;
    ndk::ScopedAStatus close() override;
    ndk::ScopedAStatus onPointerDown(int32_t pointerId, int32_t x, int32_t y, float minor,
                                     float major) override;
    ndk::ScopedAStatus onPointerUp(int32_t pointerId) override;
    ndk::ScopedAStatus onUiReady() override;
    ndk::ScopedAStatus authenticateWithContext(int64_t operationId, const OperationContext& context,
                                               std::shared_ptr<ICancellationSignal>* out) override;
    ndk::ScopedAStatus enrollWithContext(const HardwareAuthToken& hat,
                                         const OperationContext& context,
                                         std::shared_ptr<ICancellationSignal>* out) override;
    ndk::ScopedAStatus detectInteractionWithContext(
            const OperationContext& context, std::shared_ptr<ICancellationSignal>* out) override;
    ndk::ScopedAStatus onPointerDownWithContext(const PointerContext& context) override;
    ndk::ScopedAStatus onPointerUpWithContext(const PointerContext& context) override;
    ndk::ScopedAStatus onContextChanged(const OperationContext& context) override;
    ndk::ScopedAStatus onPointerCancelWithContext(const PointerContext& context) override;
    ndk::ScopedAStatus setIgnoreDisplayTouches(bool shouldIgnore) override;

    ndk::ScopedAStatus cancel();
    binder_status_t linkToDeath(AIBinder* binder);
    bool isClosed();

    // Callbacks from the stock vendor.oplus fingerprint service
    void onEnrollResult(uint32_t fid, uint32_t remaining);
    void onAcquired(OplusAcquiredInfo info, int32_t vendorCode);
    void onAuthenticated(uint32_t fid, const hidl_vec<uint8_t>& token);
    void onError(OplusError error, int32_t vendorCode);
    void onRemoved(uint32_t fid);
    void onSyncTemplates(const hidl_vec<uint32_t>& fids);

  private:
    sp<IOplusFingerprint> mDevice;
    LockoutTracker mLockoutTracker;
    bool mClosed = false;

    static Error VendorErrorFilter(OplusError error, int32_t vendorCode, int32_t* outVendorCode);
    static AcquiredInfo VendorAcquiredFilter(OplusAcquiredInfo info, int32_t vendorCode,
                                             int32_t* outVendorCode);

    bool checkSensorLockout();
    void clearLockout(bool clearAttemptCounter);
    void startLockoutTimer(int64_t timeout);
    void lockoutTimerExpired();

    // lockout timer
    bool mIsLockoutTimerStarted = false;
    bool mIsLockoutTimerAborted = false;

    // The stock service never sends onEnumerate, it reports the enrolled
    // templates through onSyncTemplates instead.
    std::mutex mTemplatesLock;
    std::vector<int32_t> mTemplates;
    bool mEnumeratePending = false;

    // detectInteraction() is running on top of a stock authentication
    std::atomic<bool> mDetecting = false;

    // The stock service removes one template per call, collect them so that
    // the framework gets a single onEnrollmentsRemoved for the whole request.
    std::mutex mRemovalLock;
    std::vector<int32_t> mPendingRemoval;
    std::vector<int32_t> mRemoved;

    // The user ID for which this session was created.
    int32_t mUserId;

    // Callback for talking to the framework. This callback must only be called from non-binder
    // threads to prevent nested binder calls and consequently a binder thread exhaustion.
    // Practically, it means that this callback should always be called from the worker thread.
    std::shared_ptr<ISessionCallback> mCb;

    // Binder death handler.
    AIBinder_DeathRecipient* mDeathRecipient;
};

}  // namespace aidl::android::hardware::biometrics::fingerprint
