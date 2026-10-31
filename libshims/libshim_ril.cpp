/*
 * Copyright (C) 2026 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

// The modem reports IMS registration info as "+EIREG=..." instead of
// "+EIREG: ...", so RmcImsControlUrcHandler::handleImsEiregInfo() fails
// to tokenize it and the URC is dropped. ImsService then never learns
// that IMS moved to Wi-Fi on LTE <-> Wi-Fi handover. Interpose
// RfxAtLine::atTokStart() and also accept '=' for +EIREG lines; every
// other line is handled exactly like the original.

struct RfxAtLine {
    char* line;
    void* next;
    char* cur;
};

extern "C" void _ZN9RfxAtLine10atTokStartEPi(RfxAtLine* thisptr, int* err) {
    char* p = nullptr;

    *err = 0;
    thisptr->cur = thisptr->line;
    if (thisptr->line != nullptr) {
        p = strchr(thisptr->line, ':');
        if (p == nullptr && strncmp(thisptr->line, "+EIREG=", 7) == 0) {
            p = thisptr->line + 6;
        }
    }

    if (p == nullptr) {
        thisptr->cur = nullptr;
        *err = -1;
        return;
    }

    thisptr->cur = p + 1;
}
