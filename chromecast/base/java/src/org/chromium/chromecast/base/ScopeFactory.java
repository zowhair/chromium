// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromecast.base;

/**
 * Interface representing the actions to perform when entering and exiting a state.
 *
 * The create() implementation is called when entering the state, and the AutoCloseable that it
 * returns is invoked when leaving the state. The side-effects are like a constructor, and the
 * side-effects of the AutoCloseable are like a destructor.
 *
 * @param <T> The argument type that the constructor takes.
 */
public interface ScopeFactory<T> { public AutoCloseable create(T data); }
