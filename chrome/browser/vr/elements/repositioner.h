// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_ELEMENTS_REPOSITIONER_H_
#define CHROME_BROWSER_VR_ELEMENTS_REPOSITIONER_H_

#include <sstream>

#include "base/macros.h"
#include "chrome/browser/vr/elements/ui_element.h"
#include "ui/gfx/transform.h"

namespace vr {

// A repositioner adjusts the position of its children by rotation. The
// reposition is driven by controller. It maintains a transform and updates it
// when enabled as either the head or the controller move. In a nutshell, it
// rotates the elements per the angular change in the controller orientation,
// adjusting the up vector of the content so that it aligns with the head's up
// vector. As the window is being repositioned, we rotate it so that it remains
// pointing upward.
class Repositioner : public UiElement {
 public:
  Repositioner();
  ~Repositioner() override;

  void set_laser_direction(const gfx::Vector3dF& laser_direction) {
    laser_direction_ = laser_direction;
  }

  void SetEnabled(bool enabled);
  void Reset();

 private:
  gfx::Transform LocalTransform() const override;
  gfx::Transform GetTargetLocalTransform() const override;
  void UpdateTransform(const gfx::Transform& head_pose);
  bool OnBeginFrame(const base::TimeTicks& time,
                    const gfx::Transform& head_pose) override;
#ifndef NDEBUG
  void DumpGeometry(std::ostringstream* os) const override;
#endif

  bool enabled_ = false;
  gfx::Transform transform_;
  gfx::Vector3dF laser_direction_;

  gfx::Transform initial_transform_;
  gfx::Vector3dF initial_laser_direction_;

  DISALLOW_COPY_AND_ASSIGN(Repositioner);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_ELEMENTS_REPOSITIONER_H_
