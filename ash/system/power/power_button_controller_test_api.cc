// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/power/power_button_controller_test_api.h"

#include "ash/system/power/power_button_display_controller.h"
#include "ash/system/power/power_button_menu_screen_view.h"
#include "ash/system/power/power_button_menu_view.h"
#include "ash/system/power/power_button_screenshot_controller.h"
#include "base/time/default_tick_clock.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/widget/widget.h"

namespace ash {

PowerButtonControllerTestApi::PowerButtonControllerTestApi(
    PowerButtonController* controller)
    : controller_(controller) {}

PowerButtonControllerTestApi::~PowerButtonControllerTestApi() = default;

bool PowerButtonControllerTestApi::ShutdownTimerIsRunning() const {
  return controller_->shutdown_timer_.IsRunning();
}

bool PowerButtonControllerTestApi::TriggerShutdownTimeout() {
  if (!controller_->shutdown_timer_.IsRunning())
    return false;

  base::Closure task = controller_->shutdown_timer_.user_task();
  controller_->shutdown_timer_.Stop();
  task.Run();
  return true;
}

bool PowerButtonControllerTestApi::PowerButtonMenuTimerIsRunning() const {
  return controller_->power_button_menu_timer_.IsRunning();
}

bool PowerButtonControllerTestApi::TriggerPowerButtonMenuTimeout() {
  if (!controller_->power_button_menu_timer_.IsRunning())
    return false;

  base::Closure task = controller_->power_button_menu_timer_.user_task();
  controller_->power_button_menu_timer_.Stop();
  task.Run();
  return true;
}

void PowerButtonControllerTestApi::SendKeyEvent(ui::KeyEvent* event) {
  controller_->display_controller_->OnKeyEvent(event);
}

gfx::Rect PowerButtonControllerTestApi::GetMenuBoundsInScreen() const {
  return IsMenuOpened() ? GetPowerButtonMenuView()->GetBoundsInScreen()
                        : gfx::Rect();
}

PowerButtonMenuView* PowerButtonControllerTestApi::GetPowerButtonMenuView()
    const {
  return IsMenuOpened() ? static_cast<PowerButtonMenuScreenView*>(
                              controller_->menu_widget_->GetContentsView())
                              ->power_button_menu_view()
                        : nullptr;
}

bool PowerButtonControllerTestApi::IsMenuOpened() const {
  return controller_->IsMenuOpened();
}

bool PowerButtonControllerTestApi::MenuHasSignOutItem() const {
  return IsMenuOpened() &&
         GetPowerButtonMenuView()->sign_out_item_for_testing();
}

bool PowerButtonControllerTestApi::ShouldTurnScreenOffForTap() const {
  return controller_->turn_screen_off_for_tap_;
}

PowerButtonScreenshotController*
PowerButtonControllerTestApi::GetScreenshotController() {
  return controller_->screenshot_controller_.get();
}

void PowerButtonControllerTestApi::SetPowerButtonType(
    PowerButtonController::ButtonType button_type) {
  controller_->button_type_ = button_type;
}

void PowerButtonControllerTestApi::SetTickClock(base::TickClock* tick_clock) {
  DCHECK(tick_clock);
  controller_->tick_clock_ = tick_clock;

  controller_->display_controller_ =
      std::make_unique<PowerButtonDisplayController>(
          controller_->backlights_forced_off_setter_, controller_->tick_clock_);
}

void PowerButtonControllerTestApi::SetTurnScreenOffForTap(
    bool turn_screen_off_for_tap) {
  controller_->turn_screen_off_for_tap_ = turn_screen_off_for_tap;
}

void PowerButtonControllerTestApi::SetShowMenuAnimationDone(
    bool show_menu_animation_done) {
  controller_->show_menu_animation_done_ = show_menu_animation_done;
}

}  // namespace ash
