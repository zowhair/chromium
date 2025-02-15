// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/contextualsearch/unhandled_tap_notifier_impl.h"

#include <utility>

#include "base/memory/ptr_util.h"
#include "content/public/common/use_zoom_for_dsf_policy.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace contextual_search {

UnhandledTapNotifierImpl::UnhandledTapNotifierImpl(
    float device_scale_factor,
    UnhandledTapCallback callback)
    : device_scale_factor_(device_scale_factor),
      unhandled_tap_callback_(std::move(callback)) {}

UnhandledTapNotifierImpl::~UnhandledTapNotifierImpl() {}

void UnhandledTapNotifierImpl::ShowUnhandledTapUIIfNeeded(
    blink::mojom::UnhandledTapInfoPtr unhandled_tap_info) {
  float x_px = unhandled_tap_info->tapped_position_in_viewport.x();
  float y_px = unhandled_tap_info->tapped_position_in_viewport.y();

  if (!content::IsUseZoomForDSFEnabled()) {
    x_px /= device_scale_factor_;
    y_px /= device_scale_factor_;
  }
  // Call back through the callback if possible.  (The callback uses a weakptr
  // that might make this a NOP).
  unhandled_tap_callback_.Run(x_px, y_px);
}

// static
void CreateUnhandledTapNotifierImpl(
    float device_scale_factor,
    UnhandledTapCallback callback,
    blink::mojom::UnhandledTapNotifierRequest request) {
  mojo::MakeStrongBinding(std::make_unique<UnhandledTapNotifierImpl>(
                              device_scale_factor, std::move(callback)),
                          std::move(request));
}

}  // namespace contextual_search
