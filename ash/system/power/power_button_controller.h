// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_POWER_POWER_BUTTON_CONTROLLER_H_
#define ASH_SYSTEM_POWER_POWER_BUTTON_CONTROLLER_H_

#include <memory>

#include "ash/ash_export.h"
#include "ash/system/power/backlights_forced_off_setter.h"
#include "ash/wm/lock_state_observer.h"
#include "ash/wm/tablet_mode/tablet_mode_observer.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "chromeos/accelerometer/accelerometer_reader.h"
#include "chromeos/dbus/power_manager_client.h"
#include "ui/display/manager/chromeos/display_configurator.h"

namespace base {
class TickClock;
class TimeTicks;
}  // namespace base

namespace views {
class Widget;
}  // namespace views

namespace ash {

class LockStateController;
class PowerButtonDisplayController;
class PowerButtonScreenshotController;

// Handles power button and lock button events. Holding the power button
// displays a menu and later shuts down on all devices. Tapping the power button
// of convertible/slate/detachable devices (except forced clamshell set by
// command line) will turn screen off but nothing will happen for clamshell
// devices. In tablet mode, power button may also be consumed to take a
// screenshot.
class ASH_EXPORT PowerButtonController
    : public display::DisplayConfigurator::Observer,
      public chromeos::PowerManagerClient::Observer,
      public chromeos::AccelerometerReader::Observer,
      public BacklightsForcedOffSetter::Observer,
      public TabletModeObserver,
      public LockStateObserver {
 public:
  enum class ButtonType {
    // Indicates normal power button type.
    NORMAL,

    // Indicates legacy power button type. It could be set by command-line
    // switch telling us that we're running on hardware that misreports power
    // button releases.
    LEGACY,
  };

  // Amount of time since last screen state change that power button event needs
  // to be ignored.
  static constexpr base::TimeDelta kScreenStateChangeDelay =
      base::TimeDelta::FromMilliseconds(500);

  // Ignore button-up events occurring within this many milliseconds of the
  // previous button-up event. This prevents us from falling behind if the power
  // button is pressed repeatedly.
  static constexpr base::TimeDelta kIgnoreRepeatedButtonUpDelay =
      base::TimeDelta::FromMilliseconds(500);

  // Amount of time since last SuspendDone() that power button event needs to be
  // ignored.
  static constexpr base::TimeDelta kIgnorePowerButtonAfterResumeDelay =
      base::TimeDelta::FromSeconds(2);

  explicit PowerButtonController(
      BacklightsForcedOffSetter* backlights_forced_off_setter);
  ~PowerButtonController() override;

  // Handles power button behavior.
  void OnPowerButtonEvent(bool down, const base::TimeTicks& timestamp);

  // Handles lock button behavior.
  void OnLockButtonEvent(bool down, const base::TimeTicks& timestamp);

  // Cancels the ongoing power button behavior. This can be called while the
  // button is still held to prevent any action from being taken on release.
  void CancelPowerButtonEvent();

  // True if the menu is opened.
  bool IsMenuOpened() const;

  // Dismisses the menu.
  void DismissMenu();

  // display::DisplayConfigurator::Observer:
  void OnDisplayModeChanged(
      const display::DisplayConfigurator::DisplayStateList& outputs) override;

  // chromeos::PowerManagerClient::Observer:
  void ScreenBrightnessChanged(
      const power_manager::BacklightBrightnessChange& change) override;
  void PowerButtonEventReceived(bool down,
                                const base::TimeTicks& timestamp) override;
  void SuspendImminent(power_manager::SuspendImminent::Reason reason) override;
  void SuspendDone(const base::TimeDelta& sleep_duration) override;

  // Initializes |turn_screen_off_for_tap_| and |screenshot_controller_|
  // according to the tablet mode switch in |result|.
  void OnGetSwitchStates(
      base::Optional<chromeos::PowerManagerClient::SwitchStates> result);

  // TODO(minch): Remove this if/when all applicable devices expose a tablet
  // mode switch: https://crbug.com/798646.
  // chromeos::AccelerometerReader::Observer:
  void OnAccelerometerUpdated(
      scoped_refptr<const chromeos::AccelerometerUpdate> update) override;

  // BacklightsForcedOffSetter::Observer:
  void OnBacklightsForcedOffChanged(bool forced_off) override;
  void OnScreenStateChanged(
      BacklightsForcedOffSetter::ScreenState screen_state) override;

  // TabletModeObserver:
  void OnTabletModeStarted() override;
  void OnTabletModeEnded() override;

  // LockStateObserver:
  void OnLockStateEvent(LockStateObserver::EventType event) override;

 private:
  friend class PowerButtonControllerTestApi;

  // Stops |power_button_menu_timer_|, |shutdown_timer_| and dismisses the power
  // button menu.
  void StopTimersAndDismissMenu();

  // Starts the power menu animation. Called when a clamshell device's power
  // button is pressed or when |power_button_menu_timer_| fires.
  void StartPowerMenuAnimation();

  // Called by |shutdown_timer_| to turn the screen off and request shutdown.
  void OnShutdownTimeout();

  // Updates |button_type_| and |force_clamshell_power_button_| based on the
  // current command line.
  void ProcessCommandLine();

  // Initializes tablet power button behavior related members
  // |turn_screen_off_for_tap_| and |screenshot_controller_|.
  void InitTabletPowerButtonMembers();

  // Locks the screen if the "Show lock screen when waking from sleep" pref is
  // set and locking is possible.
  void LockScreenIfRequired();

  // Sets |show_menu_animation_done_| to true.
  void SetShowMenuAnimationDone();

  // Are the power or lock buttons currently held?
  bool power_button_down_ = false;
  bool lock_button_down_ = false;

  // Has the screen brightness been reduced to 0%?
  bool brightness_is_zero_ = false;

  // True if an internal display is off while an external display is on (e.g.
  // for Chrome OS's docked mode, where a Chromebook's lid is closed while an
  // external display is connected).
  bool internal_display_off_and_external_display_on_ = false;

  // True after the animation that shows the power menu has finished.
  bool show_menu_animation_done_ = false;

  // Saves the button type for this power button.
  ButtonType button_type_ = ButtonType::NORMAL;

  // True if the device should observe accelerometer events to enter tablet
  // mode.
  bool observe_accelerometer_events_ = false;

  // True if the device should use non-tablet-style power button behavior even
  // if it is a convertible device.
  bool force_clamshell_power_button_ = false;

  // True if the device has tablet mode switch.
  bool has_tablet_mode_switch_ = false;

  // True if should turn screen off when tapping the power button.
  bool turn_screen_off_for_tap_ = false;

  // True if the screen was off when the power button was pressed.
  bool screen_off_when_power_button_down_ = false;

  // True if the next button release event should force the display off.
  bool force_off_on_button_up_ = false;

  // Used to force backlights off, when needed.
  BacklightsForcedOffSetter* backlights_forced_off_setter_;  // Not owned.

  LockStateController* lock_state_controller_;  // Not owned.

  // Time source for performed action times.
  base::TickClock* tick_clock_;

  // Used to interact with the display.
  std::unique_ptr<PowerButtonDisplayController> display_controller_;

  // Handles events for power button screenshot.
  std::unique_ptr<PowerButtonScreenshotController> screenshot_controller_;

  // Saves the most recent timestamp that powerd resumed from suspend,
  // updated in SuspendDone().
  base::TimeTicks last_resume_time_;

  // Saves the most recent timestamp that power button was released.
  base::TimeTicks last_button_up_time_;

  // Started when the power button is pressed and stopped when it's released.
  // Runs OnShutdownTimeout() to start shutdown.
  base::OneShotTimer shutdown_timer_;

  // Started when the power button of convertible/slate/detachable devices is
  // pressed and stopped when it's released. Runs StartPowerMenuAnimation() to
  // show the power button menu.
  base::OneShotTimer power_button_menu_timer_;

  // The fullscreen widget of power button menu.
  std::unique_ptr<views::Widget> menu_widget_;

  ScopedObserver<BacklightsForcedOffSetter, BacklightsForcedOffSetter::Observer>
      backlights_forced_off_observer_;

  base::WeakPtrFactory<PowerButtonController> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PowerButtonController);
};

}  // namespace ash

#endif  // ASH_SYSTEM_POWER_POWER_BUTTON_CONTROLLER_H_
