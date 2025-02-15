// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import org.chromium.content.browser.webcontents.WebContentsUserData;
import org.chromium.content.browser.webcontents.WebContentsUserData.UserDataFactory;
import org.chromium.content_public.browser.WebContents;

import java.util.ArrayList;
import java.util.List;

/**
 * Controls all the popup views on content view.
 */
public class PopupController {
    /** Interface for popup views that expose a method for hiding itself. */
    public interface HideablePopup {
        /**
         * Called when the popup needs to be hidden.
         */
        void hide();
    }

    private static final class UserDataFactoryLazyHolder {
        private static final UserDataFactory<PopupController> INSTANCE = PopupController::new;
    }

    private final List<HideablePopup> mHideablePopups = new ArrayList<>();

    public static PopupController fromWebContents(WebContents webContents) {
        return WebContentsUserData.fromWebContents(
                webContents, PopupController.class, UserDataFactoryLazyHolder.INSTANCE);
    }

    private PopupController(WebContents webContents) {}

    /**
     * Hide all popup views.
     * @param webContents {@link WebContents} for current content.
     */
    public static void hideAll(WebContents webContents) {
        if (webContents == null) return;
        PopupController.fromWebContents(webContents).hideAllPopups();
    }

    /**
     * Register a hideable popup.
     * @param webContents {@link WebContents} for current content.
     * @param popup {@link Hideable} popup view object.
     */
    public static void register(WebContents webContents, HideablePopup popup) {
        if (webContents == null) return;
        PopupController.fromWebContents(webContents).registerPopup(popup);
    }

    public void hideAllPopups() {
        for (HideablePopup popup : mHideablePopups) popup.hide();
    }

    public void registerPopup(HideablePopup popup) {
        mHideablePopups.add(popup);
    }
}
