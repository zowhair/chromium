// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/xr/XRInputSourceEvent.h"

namespace blink {

XRInputSourceEvent::XRInputSourceEvent() {}

XRInputSourceEvent::XRInputSourceEvent(const AtomicString& type,
                                       XRPresentationFrame* frame,
                                       XRInputSource* input_source)
    : Event(type, Bubbles::kYes, Cancelable::kNo),
      frame_(frame),
      input_source_(input_source) {}

XRInputSourceEvent::XRInputSourceEvent(
    const AtomicString& type,
    const XRInputSourceEventInit& initializer)
    : Event(type, initializer) {
  if (initializer.hasFrame())
    frame_ = initializer.frame();
  if (initializer.hasInputSource())
    input_source_ = initializer.inputSource();
}

XRInputSourceEvent::~XRInputSourceEvent() {}

const AtomicString& XRInputSourceEvent::InterfaceName() const {
  return EventNames::XRInputSourceEvent;
}

void XRInputSourceEvent::Trace(blink::Visitor* visitor) {
  visitor->Trace(frame_);
  visitor->Trace(input_source_);
  Event::Trace(visitor);
}

}  // namespace blink
