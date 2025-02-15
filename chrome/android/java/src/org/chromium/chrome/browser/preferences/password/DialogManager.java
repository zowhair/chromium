// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences.password;

import android.app.DialogFragment;
import android.app.FragmentManager;
import android.support.annotation.Nullable;

import org.chromium.base.ThreadUtils;

/**
 * This class manages a {@link DialogFragment}.
 * In particular, it ensures that the dialog stays visible for a minimum time period, so that
 * earlier calls to hide it are delayed appropriately. It also allows to override the delaying for
 * testing purposes.
 */
public final class DialogManager {
    /**
     * Contains the reference to a {@link android.app.DialogFragment} between the call to {@link
     * show} and dismissing the dialog.
     */
    @Nullable
    private DialogFragment mDialogFragment;

    /**
     * The least amout of time for which {@link mDialogFragment} should stay visible to avoid
     * flickering. It was chosen so that it is enough to read the approx. 3 words on it, but not too
     * long (to avoid the user waiting while Chrome is already idle).
     */
    private static final long MINIMUM_LIFE_SPAN_MILLIS = 1000L;

    /** This is used to post the unblocking signal for hiding the dialog fragment. */
    private CallbackDelayer mDelayer = new TimedCallbackDelayer(MINIMUM_LIFE_SPAN_MILLIS);

    /** Allows to fake the timed delayer. */
    public void replaceCallbackDelayerForTesting(CallbackDelayer newDelayer) {
        mDelayer = newDelayer;
    }

    /**
     * Used to gate hiding of a dialog on two actions: one automatic delayed signal and one manual
     * call to {@link hide}. This is not null between the calls to {@link show} and {@link hide}.
     */
    @Nullable
    private SingleThreadBarrierClosure mBarrierClosure;

    /** Callback to run after the dialog was hidden. Can be null if no hiding was requested.*/
    @Nullable
    private Runnable mCallback;

    /**
     * Shows the dialog.
     * @param dialog to be shown.
     * @param fragmentManager needed to call {@link android.app.DialogFragment#show}
     */
    public void show(DialogFragment dialog, FragmentManager fragmentManager) {
        assert mDialogFragment == null;
        mDialogFragment = dialog;
        mDialogFragment.show(fragmentManager, null);
        // Initiate the barrier closure, expecting 2 runs: one automatic but delayed, and one
        // explicit, to hide the dialog.
        // TODO(crbug.com/821377) -- remove clang-format pragmas
        // clang-format off
        mBarrierClosure = new SingleThreadBarrierClosure(2, this::hideImmediately);
        // clang-format on
        // This is the automatic but delayed signal.
        mDelayer.delay(mBarrierClosure);
    }

    /**
     * Hides the dialog as soon as possible, but not sooner than {@link MINIMUM_LIFE_SPAN_MILLIS}
     * milliseconds after it was shown. Attempts to hide the dialog when none is shown are
     * gracefully ignored but the callback is called in any case.
     * @param callback is asynchronously called as soon as the dialog is no longer visible.
     */
    public void hide(Runnable callback) {
        mCallback = callback;
        // The barrier closure is null if the dialog was not shown. In that case don't wait before
        // confirming the hidden state.
        if (mBarrierClosure == null) {
            hideImmediately();
        } else {
            mBarrierClosure.run();
        }
    }

    /**
     * Synchronously hides the dialog without any delay. Attempts to hide the dialog when
     * none is shown are gracefully ignored but |mCallback| is called in any case if present.
     */
    private void hideImmediately() {
        if (mDialogFragment != null) mDialogFragment.dismiss();
        // Post the callback to ensure that it is always run asynchronously, even if hide() took a
        // shortcut for a missing shown().
        if (mCallback != null) ThreadUtils.postOnUiThread(mCallback);
        reset();
    }

    /** Resets the dialog reference and metadata related to it.*/
    private void reset() {
        mDialogFragment = null;
        mCallback = null;
        mBarrierClosure = null;
    }
}
