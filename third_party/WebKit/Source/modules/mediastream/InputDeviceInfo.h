// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef InputDeviceInfo_h
#define InputDeviceInfo_h

#include "modules/mediastream/MediaDeviceInfo.h"

namespace blink {

class MediaTrackCapabilities;

class InputDeviceInfo final : public MediaDeviceInfo {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static InputDeviceInfo* Create(const String& device_id,
                                 const String& label,
                                 const String& group_id,
                                 MediaDeviceType);

  void getCapabilities(MediaTrackCapabilities&);

 private:
  InputDeviceInfo(const String& device_id,
                  const String& label,
                  const String& group_id,
                  MediaDeviceType);
};

}  // namespace blink

#endif  // InputDeviceInfo_h
