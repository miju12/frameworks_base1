/*
 * Copyright (C) 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.server.biometrics.sensors.fingerprint.aidl;

import android.annotation.NonNull;
import android.hardware.biometrics.fingerprint.ISession;

import static com.android.server.biometrics.sensors.fingerprint.aidl.Sensor.HalSessionCallback;

/**
 * A holder for an AIDL {@link ISession} with additional metadata about the current user
 * and the backend.
 */
public class AidlSession {

    private final int mHalInterfaceVersion;
    @NonNull private final ISession mSession;
    private final int mUserId;
    @NonNull private final HalSessionCallback mHalSessionCallback;

    public AidlSession(int halInterfaceVersion, @NonNull ISession session, int userId,
            HalSessionCallback halSessionCallback) {
        mHalInterfaceVersion = halInterfaceVersion;
        mSession = session;
        mUserId = userId;
        mHalSessionCallback = halSessionCallback;
    }

    /** The underlying {@link ISession}. */
    @NonNull public ISession getSession() {
        return mSession;
    }

    /** The user id associated with the session. */
    public int getUserId() {
        return mUserId;
    }

    /** The HAL callback, which should only be used in tests {@See BiometricTestSessionImpl}. */
    HalSessionCallback getHalSessionCallback() {
        return mHalSessionCallback;
    }

    /**
     * If this backend implements the *WithContext methods for enroll, authenticate, and
     * detectInteraction. These variants should always be called if they are available.
     */
    public boolean hasContextMethods() {
        return mHalInterfaceVersion >= 2;
    }
}
