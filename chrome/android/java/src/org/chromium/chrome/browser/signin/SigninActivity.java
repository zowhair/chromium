// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.signin;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;

import org.chromium.base.Log;
import org.chromium.base.library_loader.ProcessInitException;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.init.ChromeBrowserInitializer;

/**
 * Allows user to pick an account and sign in. Started from Settings and various sign-in promos.
 */
// TODO(https://crbug.com/820491): extend AsyncInitializationActivity.
public class SigninActivity extends AppCompatActivity {
    private static final String TAG = "SigninActivity";

    /**
     * Creates an {@link Intent} which can be used to start the signin flow.
     * @param accessPoint {@link AccessPoint} for starting signin flow. Used in metrics.
     * @param isFromPersonalizedPromo Whether the signin activity is started from a personalized
     *         promo.
     */
    public static Intent createIntent(Context context,
            @AccountSigninActivity.AccessPoint int accessPoint, boolean isFromPersonalizedPromo) {
        Intent intent = new Intent(context, SigninActivity.class);
        // TODO(https://crbug.com/814728): Call SigninFragment.createArguments.
        Bundle fragmentArguments = new Bundle();
        intent.putExtras(fragmentArguments);
        return intent;
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        // The browser process must be started here because this activity may be started from the
        // recent apps list and it relies on other activities and the native library to be loaded.
        try {
            ChromeBrowserInitializer.getInstance(this).handleSynchronousStartup();
        } catch (ProcessInitException e) {
            Log.e(TAG, "Failed to start browser process.", e);
            // Since the library failed to initialize nothing in the application
            // can work, so kill the whole application not just the activity
            System.exit(-1);
        }

        super.onCreate(savedInstanceState);
        setContentView(R.layout.signin_activity);

        // TODO(https://crbug.com/814728): Add SigninFragment.
    }
}
