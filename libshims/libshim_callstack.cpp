
/*
 * Copyright (C) 2026 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <utils/CallStack.h>

// No-op replacement for the CallStack symbols used by libdpframework and
// libgralloc_extra. Unlike libutilscallstack it doesn't provide the
// getCurrentInternal/logStackInternal hooks, so libutils doesn't unwind
// a call stack on every RefBase explicit destruction in libgpu_aux,
// which stalls the SurfaceFlinger RenderEngine thread for seconds.

namespace android {

CallStack::CallStack() {}

CallStack::~CallStack() {}

void CallStack::update(int32_t /* ignoreDepth */, pid_t /* tid */) {}

void CallStack::log(const char* /* logtag */, android_LogPriority /* priority */,
                    const char* /* prefix */) const {}

}  // namespace android
