// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.support_lib_boundary;

import android.webkit.WebView;

import java.lang.reflect.InvocationHandler;

/**
 */
public interface WebViewProviderFactoryBoundaryInterface {
    /* SupportLibraryWebViewChromium */ InvocationHandler createWebView(WebView webview);
    /* SupportLibWebkitToCompatConverter */ InvocationHandler getWebkitToCompatConverter();
    /* StaticsAdapter */ InvocationHandler getStatics();
    String[] getSupportedFeatures();
}
