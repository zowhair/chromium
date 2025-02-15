// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/power/ml/user_activity_manager.h"

#include <memory>
#include <vector>

#include "base/test/test_mock_time_task_runner.h"
#include "base/time/clock.h"
#include "base/timer/timer.h"
#include "chrome/browser/chromeos/login/users/fake_chrome_user_manager.h"
#include "chrome/browser/chromeos/power/ml/fake_boot_clock.h"
#include "chrome/browser/chromeos/power/ml/idle_event_notifier.h"
#include "chrome/browser/chromeos/power/ml/user_activity_event.pb.h"
#include "chrome/browser/chromeos/power/ml/user_activity_ukm_logger.h"
#include "chrome/browser/engagement/site_engagement_service.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/tabs/tab_activity_simulator.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "chrome/test/base/test_browser_window_aura.h"
#include "chrome/test/base/testing_profile.h"
#include "chromeos/dbus/fake_power_manager_client.h"
#include "chromeos/dbus/power_manager/idle.pb.h"
#include "chromeos/dbus/power_manager/power_supply_properties.pb.h"
#include "chromeos/dbus/power_manager_client.h"
#include "components/session_manager/session_manager_types.h"
#include "components/ukm/test_ukm_recorder.h"
#include "components/ukm/ukm_source.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/user_activity/user_activity_detector.h"

namespace chromeos {
namespace power {
namespace ml {

using content::WebContentsTester;

void EqualEvent(const UserActivityEvent::Event& expected_event,
                const UserActivityEvent::Event& result_event) {
  EXPECT_EQ(expected_event.type(), result_event.type());
  EXPECT_EQ(expected_event.reason(), result_event.reason());
  EXPECT_EQ(expected_event.log_duration_sec(), result_event.log_duration_sec());
}

// Testing UKM logger.
class TestingUserActivityUkmLogger : public UserActivityUkmLogger {
 public:
  TestingUserActivityUkmLogger() = default;
  ~TestingUserActivityUkmLogger() override = default;

  const std::vector<UserActivityEvent>& events() const { return events_; }

  // UserActivityUkmLogger overrides:
  void LogActivity(
      const UserActivityEvent& event,
      const std::map<ukm::SourceId, TabProperty>& source_ids) override {
    events_.push_back(event);
  }

 private:
  std::vector<UserActivityEvent> events_;

  DISALLOW_COPY_AND_ASSIGN(TestingUserActivityUkmLogger);
};

class UserActivityManagerTest : public ChromeRenderViewHostTestHarness {
 public:
  UserActivityManagerTest()
      : task_runner_(base::MakeRefCounted<base::TestMockTimeTaskRunner>()) {
    fake_power_manager_client_.Init(nullptr);
    viz::mojom::VideoDetectorObserverPtr observer;
    idle_event_notifier_ = std::make_unique<IdleEventNotifier>(
        &fake_power_manager_client_, &user_activity_detector_,
        mojo::MakeRequest(&observer));
    activity_logger_ = std::make_unique<UserActivityManager>(
        &delegate_, idle_event_notifier_.get(), &user_activity_detector_,
        &fake_power_manager_client_, &session_manager_,
        mojo::MakeRequest(&observer), &fake_user_manager_);
    activity_logger_->SetTaskRunnerForTesting(
        task_runner_, std::make_unique<FakeBootClock>(
                          task_runner_, base::TimeDelta::FromSeconds(10)));
  }

  ~UserActivityManagerTest() override = default;

 protected:
  void ReportUserActivity(const ui::Event* event) {
    activity_logger_->OnUserActivity(event);
  }

  void ReportIdleEvent(const IdleEventNotifier::ActivityData& data) {
    activity_logger_->OnIdleEventObserved(data);
  }

  void ReportLidEvent(chromeos::PowerManagerClient::LidState state) {
    fake_power_manager_client_.SetLidState(state, base::TimeTicks::UnixEpoch());
  }

  void ReportPowerChangeEvent(
      power_manager::PowerSupplyProperties::ExternalPower power,
      float battery_percent) {
    power_manager::PowerSupplyProperties proto;
    proto.set_external_power(power);
    proto.set_battery_percent(battery_percent);
    fake_power_manager_client_.UpdatePowerProperties(proto);
  }

  void ReportTabletModeEvent(chromeos::PowerManagerClient::TabletMode mode) {
    fake_power_manager_client_.SetTabletMode(mode,
                                             base::TimeTicks::UnixEpoch());
  }

  void ReportVideoStart() { activity_logger_->OnVideoActivityStarted(); }

  void ReportScreenIdle() {
    power_manager::ScreenIdleState proto;
    proto.set_off(true);
    fake_power_manager_client_.SendScreenIdleStateChanged(proto);
  }

  void ReportScreenLocked() {
    session_manager_.SetSessionState(session_manager::SessionState::LOCKED);
  }

  void ReportSuspend(power_manager::SuspendImminent::Reason reason,
                     base::TimeDelta sleep_duration) {
    fake_power_manager_client_.SendSuspendImminent(reason);
    fake_power_manager_client_.SendSuspendDone(sleep_duration);
  }

  void ReportInactivityDelays(base::TimeDelta screen_dim_delay,
                              base::TimeDelta screen_off_delay) {
    power_manager::PowerManagementPolicy::Delays proto;
    proto.set_screen_dim_ms(screen_dim_delay.InMilliseconds());
    proto.set_screen_off_ms(screen_off_delay.InMilliseconds());
    fake_power_manager_client_.SetInactivityDelays(proto);
  }

  void UpdateOpenTabsURLs() { activity_logger_->UpdateOpenTabsURLs(); }

  // Creates a test browser window and sets its visibility, activity and
  // incognito status.
  std::unique_ptr<Browser> CreateTestBrowser(bool is_visible,
                                             bool is_focused,
                                             bool is_incognito = false) {
    Profile* const original_profile = profile();
    Profile* const used_profile =
        is_incognito ? original_profile->GetOffTheRecordProfile()
                     : original_profile;
    Browser::CreateParams params(used_profile, true);

    auto dummy_window = std::make_unique<aura::Window>(nullptr);
    dummy_window->Init(ui::LAYER_SOLID_COLOR);
    root_window()->AddChild(dummy_window.get());
    dummy_window->SetBounds(gfx::Rect(root_window()->bounds().size()));
    if (is_visible) {
      dummy_window->Show();
    } else {
      dummy_window->Hide();
    }

    std::unique_ptr<Browser> browser =
        chrome::CreateBrowserWithAuraTestWindowForParams(
            std::move(dummy_window), &params);
    if (is_focused) {
      browser->window()->Activate();
    } else {
      browser->window()->Deactivate();
    }
    return browser;
  }

  // Adds a tab with specified url to the tab strip model. Also optionally sets
  // the tab to be the active one in the tab strip model.
  // If |mime_type| is an empty string, the content has a default text type.
  // TODO(jiameng): there doesn't seem to be a way to set form entry (via
  // page importance signal). Check if there's some other way to set it.
  void CreateTestWebContents(TabStripModel* const tab_strip_model,
                             const GURL& url,
                             bool is_active,
                             const std::string& mime_type = "") {
    DCHECK(tab_strip_model);
    DCHECK(!url.is_empty());
    content::WebContents* contents =
        tab_activity_simulator_.AddWebContentsAndNavigate(tab_strip_model, url);
    if (is_active) {
      tab_strip_model->ActivateTabAt(tab_strip_model->count() - 1, false);
    }
    if (!mime_type.empty())
      WebContentsTester::For(contents)->SetMainFrameMimeType(mime_type);

    WebContentsTester::For(contents)->TestSetIsLoading(false);
  }

  ukm::SourceId GetSourceIdForUrl(const GURL& url) {
    const ukm::UkmSource* source = ukm_recorder_.GetSourceForUrl(url);
    DCHECK(source);
    return source->id();
  }

  void CheckTabsProperties(
      const std::map<ukm::SourceId, TabProperty>& expected_map) {
    // Does not just use std::equal to give a better sense of where they fail
    // when debugging.
    const auto& actual_map = activity_logger_->open_tabs_;
    EXPECT_EQ(expected_map.size(), actual_map.size());
    for (const auto& expected_value : expected_map) {
      const auto& it = actual_map.find(expected_value.first);
      ASSERT_TRUE(it != actual_map.end())
          << "Failed to find a match for " << expected_value.first;
      const TabProperty& actual = it->second;
      const TabProperty& expected = expected_value.second;
      EXPECT_EQ(expected.is_active, actual.is_active) << expected_value.first;
      EXPECT_EQ(expected.is_browser_focused, actual.is_browser_focused)
          << expected_value.first;
      EXPECT_EQ(expected.is_browser_visible, actual.is_browser_visible)
          << expected_value.first;
      EXPECT_EQ(expected.is_topmost_browser, actual.is_topmost_browser)
          << expected_value.first;
      EXPECT_EQ(expected.engagement_score, actual.engagement_score)
          << expected_value.first;
      EXPECT_EQ(expected.content_type, actual.content_type)
          << expected_value.first;
      EXPECT_EQ(expected.has_form_entry, actual.has_form_entry)
          << expected_value.first;
    }
  }

  const scoped_refptr<base::TestMockTimeTaskRunner>& GetTaskRunner() {
    return task_runner_;
  }

  TestingUserActivityUkmLogger delegate_;
  chromeos::FakeChromeUserManager fake_user_manager_;
  // Only used to get SourceIds for URLs.
  ukm::TestAutoSetUkmRecorder ukm_recorder_;
  TabActivitySimulator tab_activity_simulator_;

  const GURL url1_ = GURL("https://example1.com/");
  const GURL url2_ = GURL("https://example2.com/");
  const GURL url3_ = GURL("https://example3.com/");
  const GURL url4_ = GURL("https://example4.com/");

 private:
  scoped_refptr<base::TestMockTimeTaskRunner> task_runner_;

  ui::UserActivityDetector user_activity_detector_;
  std::unique_ptr<IdleEventNotifier> idle_event_notifier_;
  chromeos::FakePowerManagerClient fake_power_manager_client_;
  session_manager::SessionManager session_manager_;
  std::unique_ptr<UserActivityManager> activity_logger_;

  DISALLOW_COPY_AND_ASSIGN(UserActivityManagerTest);
};

// After an idle event, we have a ui::Event, we should expect one
// UserActivityEvent.
TEST_F(UserActivityManagerTest, LogAfterIdleEvent) {
  // Trigger an idle event.
  const IdleEventNotifier::ActivityData data;
  ReportIdleEvent(data);
  GetTaskRunner()->FastForwardBy(base::TimeDelta::FromSeconds(2));
  ReportUserActivity(nullptr);

  const std::vector<UserActivityEvent>& events = delegate_.events();
  ASSERT_EQ(1U, events.size());

  UserActivityEvent::Event expected_event;
  expected_event.set_type(UserActivityEvent::Event::REACTIVATE);
  expected_event.set_reason(UserActivityEvent::Event::USER_ACTIVITY);
  expected_event.set_log_duration_sec(2);
  EqualEvent(expected_event, events[0].event());
}

// Get a user event before an idle event, we should not log it.
TEST_F(UserActivityManagerTest, LogBeforeIdleEvent) {
  ReportUserActivity(nullptr);
  // Trigger an idle event.
  const IdleEventNotifier::ActivityData data;
  ReportIdleEvent(data);

  EXPECT_EQ(0U, delegate_.events().size());
}

// Get a user event, then an idle event, then another user event,
// we should log the last one.
TEST_F(UserActivityManagerTest, LogSecondEvent) {
  ReportUserActivity(nullptr);
  // Trigger an idle event.
  const IdleEventNotifier::ActivityData data;
  ReportIdleEvent(data);
  // Another user event.
  ReportUserActivity(nullptr);

  const std::vector<UserActivityEvent>& events = delegate_.events();
  ASSERT_EQ(1U, events.size());

  UserActivityEvent::Event expected_event;
  expected_event.set_type(UserActivityEvent::Event::REACTIVATE);
  expected_event.set_reason(UserActivityEvent::Event::USER_ACTIVITY);
  expected_event.set_log_duration_sec(0);
  EqualEvent(expected_event, events[0].event());
}

// Log multiple events.
TEST_F(UserActivityManagerTest, LogMultipleEvents) {
  // Trigger an idle event.
  const IdleEventNotifier::ActivityData data;
  ReportIdleEvent(data);
  // First user event.
  ReportUserActivity(nullptr);

  // Trigger an idle event.
  ReportIdleEvent(data);
  // Second user event.
  GetTaskRunner()->FastForwardBy(base::TimeDelta::FromSeconds(2));
  ReportUserActivity(nullptr);

  const std::vector<UserActivityEvent>& events = delegate_.events();
  ASSERT_EQ(2U, events.size());

  UserActivityEvent::Event expected_event1;
  expected_event1.set_type(UserActivityEvent::Event::REACTIVATE);
  expected_event1.set_reason(UserActivityEvent::Event::USER_ACTIVITY);
  expected_event1.set_log_duration_sec(0);

  UserActivityEvent::Event expected_event2;
  expected_event2.set_type(UserActivityEvent::Event::REACTIVATE);
  expected_event2.set_reason(UserActivityEvent::Event::USER_ACTIVITY);
  expected_event2.set_log_duration_sec(2);

  EqualEvent(expected_event1, events[0].event());
  EqualEvent(expected_event2, events[1].event());
}

TEST_F(UserActivityManagerTest, UserCloseLid) {
  ReportLidEvent(chromeos::PowerManagerClient::LidState::OPEN);
  // Trigger an idle event.
  const IdleEventNotifier::ActivityData data;
  ReportIdleEvent(data);

  GetTaskRunner()->FastForwardBy(base::TimeDelta::FromSeconds(2));
  ReportLidEvent(chromeos::PowerManagerClient::LidState::CLOSED);
  const auto& events = delegate_.events();
  EXPECT_TRUE(events.empty());
}

TEST_F(UserActivityManagerTest, PowerChangeActivity) {
  ReportPowerChangeEvent(power_manager::PowerSupplyProperties::AC, 23.0f);
  // Trigger an idle event.
  const IdleEventNotifier::ActivityData data;
  ReportIdleEvent(data);

  // We don't care about battery percentage change, but only power source.
  ReportPowerChangeEvent(power_manager::PowerSupplyProperties::AC, 25.0f);
  ReportPowerChangeEvent(power_manager::PowerSupplyProperties::DISCONNECTED,
                         28.0f);
  const auto& events = delegate_.events();
  ASSERT_EQ(1U, events.size());

  UserActivityEvent::Event expected_event;
  expected_event.set_type(UserActivityEvent::Event::REACTIVATE);
  expected_event.set_reason(UserActivityEvent::Event::POWER_CHANGED);
  expected_event.set_log_duration_sec(0);
  EqualEvent(expected_event, events[0].event());
}

TEST_F(UserActivityManagerTest, VideoActivity) {
  // Trigger an idle event.
  const IdleEventNotifier::ActivityData data;
  ReportIdleEvent(data);

  ReportVideoStart();
  const auto& events = delegate_.events();
  ASSERT_EQ(1U, events.size());

  UserActivityEvent::Event expected_event;
  expected_event.set_type(UserActivityEvent::Event::REACTIVATE);
  expected_event.set_reason(UserActivityEvent::Event::VIDEO_ACTIVITY);
  expected_event.set_log_duration_sec(0);
  EqualEvent(expected_event, events[0].event());
}

TEST_F(UserActivityManagerTest, SystemIdle) {
  // Trigger an idle event.
  const IdleEventNotifier::ActivityData data;
  ReportIdleEvent(data);

  ReportScreenIdle();
  GetTaskRunner()->FastForwardUntilNoTasksRemain();

  const auto& events = delegate_.events();
  ASSERT_EQ(1U, events.size());

  UserActivityEvent::Event expected_event;
  expected_event.set_type(UserActivityEvent::Event::TIMEOUT);
  expected_event.set_reason(UserActivityEvent::Event::SCREEN_OFF);
  expected_event.set_log_duration_sec(
      UserActivityManager::kIdleDelay.InSeconds());
  EqualEvent(expected_event, events[0].event());
}

// Test system idle interrupt by user activity.
// We should only observe user activity.
TEST_F(UserActivityManagerTest, SystemIdleInterrupted) {
  // Trigger an idle event.
  const IdleEventNotifier::ActivityData data;
  ReportIdleEvent(data);

  ReportScreenIdle();
  // User interruptted after 1 second.
  GetTaskRunner()->FastForwardBy(base::TimeDelta::FromSeconds(1));
  ReportUserActivity(nullptr);
  GetTaskRunner()->FastForwardUntilNoTasksRemain();

  const auto& events = delegate_.events();
  ASSERT_EQ(1U, events.size());

  UserActivityEvent::Event expected_event;
  expected_event.set_type(UserActivityEvent::Event::REACTIVATE);
  expected_event.set_reason(UserActivityEvent::Event::USER_ACTIVITY);
  expected_event.set_log_duration_sec(1);
  EqualEvent(expected_event, events[0].event());
}

TEST_F(UserActivityManagerTest, ScreenLock) {
  // Trigger an idle event.
  const IdleEventNotifier::ActivityData data;
  ReportIdleEvent(data);

  ReportScreenLocked();
  const auto& events = delegate_.events();
  ASSERT_EQ(1U, events.size());

  UserActivityEvent::Event expected_event;
  expected_event.set_type(UserActivityEvent::Event::OFF);
  expected_event.set_reason(UserActivityEvent::Event::SCREEN_LOCK);
  expected_event.set_log_duration_sec(0);
  EqualEvent(expected_event, events[0].event());
}

TEST_F(UserActivityManagerTest, SuspendIdle) {
  // Trigger an idle event.
  const IdleEventNotifier::ActivityData data;
  ReportIdleEvent(data);

  ReportSuspend(power_manager::SuspendImminent_Reason_IDLE,
                2 * UserActivityManager::kMinSuspendDuration);
  const auto& events = delegate_.events();
  ASSERT_EQ(1U, events.size());

  UserActivityEvent::Event expected_event;
  expected_event.set_type(UserActivityEvent::Event::TIMEOUT);
  expected_event.set_reason(UserActivityEvent::Event::IDLE_SLEEP);
  EqualEvent(expected_event, events[0].event());
}

TEST_F(UserActivityManagerTest, SuspendIdleCancelled) {
  // Trigger an idle event.
  const IdleEventNotifier::ActivityData data;
  ReportIdleEvent(data);

  ReportSuspend(power_manager::SuspendImminent_Reason_IDLE,
                UserActivityManager::kMinSuspendDuration -
                    base::TimeDelta::FromSeconds(2));
  const auto& events = delegate_.events();
  ASSERT_EQ(1U, events.size());

  UserActivityEvent::Event expected_event;
  expected_event.set_type(UserActivityEvent::Event::REACTIVATE);
  expected_event.set_reason(UserActivityEvent::Event::USER_ACTIVITY);
  EqualEvent(expected_event, events[0].event());
}

TEST_F(UserActivityManagerTest, SuspendLidClosed) {
  // Trigger an idle event.
  const IdleEventNotifier::ActivityData data;
  ReportIdleEvent(data);

  ReportSuspend(power_manager::SuspendImminent_Reason_LID_CLOSED,
                2 * UserActivityManager::kMinSuspendDuration);
  const auto& events = delegate_.events();
  ASSERT_EQ(1U, events.size());

  UserActivityEvent::Event expected_event;
  expected_event.set_type(UserActivityEvent::Event::OFF);
  expected_event.set_reason(UserActivityEvent::Event::LID_CLOSED);
  EqualEvent(expected_event, events[0].event());
}

TEST_F(UserActivityManagerTest, SuspendLidClosedCancelled) {
  // Trigger an idle event.
  const IdleEventNotifier::ActivityData data;
  ReportIdleEvent(data);

  ReportSuspend(power_manager::SuspendImminent_Reason_LID_CLOSED,
                UserActivityManager::kMinSuspendDuration -
                    base::TimeDelta::FromSeconds(2));
  const auto& events = delegate_.events();
  ASSERT_EQ(1U, events.size());

  UserActivityEvent::Event expected_event;
  expected_event.set_type(UserActivityEvent::Event::REACTIVATE);
  expected_event.set_reason(UserActivityEvent::Event::USER_ACTIVITY);
  EqualEvent(expected_event, events[0].event());
}

TEST_F(UserActivityManagerTest, SuspendOther) {
  // Trigger an idle event.
  const IdleEventNotifier::ActivityData data;
  ReportIdleEvent(data);

  ReportSuspend(power_manager::SuspendImminent_Reason_OTHER,
                UserActivityManager::kMinSuspendDuration);
  const auto& events = delegate_.events();
  ASSERT_EQ(1U, events.size());

  UserActivityEvent::Event expected_event;
  expected_event.set_type(UserActivityEvent::Event::OFF);
  expected_event.set_reason(UserActivityEvent::Event::MANUAL_SLEEP);
  EqualEvent(expected_event, events[0].event());
}

TEST_F(UserActivityManagerTest, SuspendOtherCancelled) {
  // Trigger an idle event.
  const IdleEventNotifier::ActivityData data;
  ReportIdleEvent(data);

  ReportSuspend(power_manager::SuspendImminent_Reason_OTHER,
                UserActivityManager::kMinSuspendDuration -
                    base::TimeDelta::FromSeconds(2));
  const auto& events = delegate_.events();
  ASSERT_EQ(1U, events.size());

  UserActivityEvent::Event expected_event;
  expected_event.set_type(UserActivityEvent::Event::REACTIVATE);
  expected_event.set_reason(UserActivityEvent::Event::USER_ACTIVITY);
  EqualEvent(expected_event, events[0].event());
}

// Test feature extraction.
TEST_F(UserActivityManagerTest, FeatureExtraction) {
  ReportLidEvent(chromeos::PowerManagerClient::LidState::OPEN);
  ReportTabletModeEvent(chromeos::PowerManagerClient::TabletMode::UNSUPPORTED);
  ReportPowerChangeEvent(power_manager::PowerSupplyProperties::AC, 23.0f);

  IdleEventNotifier::ActivityData data;
  data.last_activity_day = UserActivityEvent_Features_DayOfWeek_MON;
  data.last_activity_time_of_day = base::TimeDelta::FromSeconds(100);
  data.recent_time_active = base::TimeDelta::FromSeconds(10);
  data.time_since_last_mouse = base::TimeDelta::FromSeconds(20);
  data.time_since_last_touch = base::TimeDelta::FromSeconds(30);
  data.video_playing_time = base::TimeDelta::FromSeconds(90);
  data.time_since_video_ended = base::TimeDelta::FromSeconds(2);
  data.key_events_in_last_hour = 0;
  data.mouse_events_in_last_hour = 10;
  data.touch_events_in_last_hour = 20;

  ReportIdleEvent(data);
  ReportUserActivity(nullptr);

  const auto& events = delegate_.events();
  ASSERT_EQ(1U, events.size());

  const UserActivityEvent::Features& features = events[0].features();
  EXPECT_EQ(UserActivityEvent::Features::CLAMSHELL, features.device_mode());
  EXPECT_EQ(23.0f, features.battery_percent());
  EXPECT_FALSE(features.on_battery());
  EXPECT_EQ(UserActivityEvent::Features::UNMANAGED,
            features.device_management());
  EXPECT_EQ(UserActivityEvent_Features_DayOfWeek_MON,
            features.last_activity_day());
  EXPECT_EQ(100, features.last_activity_time_sec());
  EXPECT_EQ(10, features.recent_time_active_sec());
  EXPECT_EQ(20, features.time_since_last_mouse_sec());
  EXPECT_EQ(30, features.time_since_last_touch_sec());
  EXPECT_EQ(90, features.video_playing_time_sec());
  EXPECT_EQ(2, features.time_since_video_ended_sec());
  EXPECT_EQ(0, features.key_events_in_last_hour());
  EXPECT_EQ(10, features.mouse_events_in_last_hour());
  EXPECT_EQ(20, features.touch_events_in_last_hour());
  EXPECT_FALSE(features.has_last_user_activity_time_sec());
  EXPECT_FALSE(features.has_time_since_last_key_sec());
}

TEST_F(UserActivityManagerTest, ManagedDevice) {
  fake_user_manager_.set_is_enterprise_managed(true);

  const IdleEventNotifier::ActivityData data;
  ReportIdleEvent(data);
  ReportUserActivity(nullptr);

  const auto& events = delegate_.events();
  ASSERT_EQ(1U, events.size());

  const UserActivityEvent::Features& features = events[0].features();
  EXPECT_EQ(UserActivityEvent::Features::MANAGED, features.device_management());
}

TEST_F(UserActivityManagerTest, DimAndOffDelays) {
  ReportInactivityDelays(
      base::TimeDelta::FromMilliseconds(2000) /* screen_dim_delay */,
      base::TimeDelta::FromMilliseconds(3000) /* screen_off_delay */);
  const IdleEventNotifier::ActivityData data;
  ReportIdleEvent(data);
  ReportUserActivity(nullptr);

  const auto& events = delegate_.events();
  ASSERT_EQ(1U, events.size());

  const UserActivityEvent::Features& features = events[0].features();
  EXPECT_EQ(2, features.on_to_dim_sec());
  EXPECT_EQ(1, features.dim_to_screen_off_sec());
}

TEST_F(UserActivityManagerTest, DimDelays) {
  ReportInactivityDelays(
      base::TimeDelta::FromMilliseconds(2000) /* screen_dim_delay */,
      base::TimeDelta() /* screen_off_delay */);
  const IdleEventNotifier::ActivityData data;
  ReportIdleEvent(data);
  ReportUserActivity(nullptr);

  const auto& events = delegate_.events();
  ASSERT_EQ(1U, events.size());

  const UserActivityEvent::Features& features = events[0].features();
  EXPECT_EQ(2, features.on_to_dim_sec());
  EXPECT_TRUE(!features.has_dim_to_screen_off_sec());
}

TEST_F(UserActivityManagerTest, OffDelays) {
  ReportInactivityDelays(
      base::TimeDelta() /* screen_dim_delay */,
      base::TimeDelta::FromMilliseconds(4000) /* screen_off_delay */);
  const IdleEventNotifier::ActivityData data;
  ReportIdleEvent(data);
  ReportUserActivity(nullptr);

  const auto& events = delegate_.events();
  ASSERT_EQ(1U, events.size());

  const UserActivityEvent::Features& features = events[0].features();
  EXPECT_EQ(4, features.dim_to_screen_off_sec());
  EXPECT_TRUE(!features.has_on_to_dim_sec());
}

TEST_F(UserActivityManagerTest, BasicTabs) {
  std::unique_ptr<Browser> browser =
      CreateTestBrowser(true /* is_visible */, true /* is_focused */);
  BrowserList::GetInstance()->SetLastActive(browser.get());
  TabStripModel* tab_strip_model = browser->tab_strip_model();
  CreateTestWebContents(tab_strip_model, url1_, true /* is_active */,
                        "application/pdf");
  SiteEngagementService::Get(profile())->ResetBaseScoreForURL(url1_, 95);
  const ukm::SourceId source_id1 = GetSourceIdForUrl(url1_);

  CreateTestWebContents(tab_strip_model, url2_, false /* is_active */);
  const ukm::SourceId source_id2 = GetSourceIdForUrl(url2_);

  UpdateOpenTabsURLs();

  TabProperty expected_property1;
  expected_property1.is_active = true;
  expected_property1.is_browser_focused = true;
  expected_property1.is_browser_visible = true;
  expected_property1.is_topmost_browser = true;
  expected_property1.engagement_score = 90;
  expected_property1.content_type =
      metrics::TabMetricsEvent::CONTENT_TYPE_APPLICATION;
  expected_property1.has_form_entry = false;

  TabProperty expected_property2;
  expected_property2.is_active = false;
  expected_property2.is_browser_focused = true;
  expected_property2.is_browser_visible = true;
  expected_property2.is_topmost_browser = true;
  expected_property2.engagement_score = 0;
  expected_property2.content_type =
      metrics::TabMetricsEvent::CONTENT_TYPE_TEXT_HTML;
  expected_property2.has_form_entry = false;

  const std::map<ukm::SourceId, TabProperty> expected_sources(
      {{source_id1, expected_property1}, {source_id2, expected_property2}});
  CheckTabsProperties(expected_sources);

  tab_strip_model->CloseAllTabs();
}

TEST_F(UserActivityManagerTest, MultiBrowsersAndTabs) {
  // Simulates three browsers:
  //  - browser1 is the last active but minimized and so not visible.
  //  - browser2 and browser3 are both visible but browser2 is the topmost.
  std::unique_ptr<Browser> browser1 =
      CreateTestBrowser(false /* is_visible */, false /* is_focused */);
  std::unique_ptr<Browser> browser2 =
      CreateTestBrowser(true /* is_visible */, true /* is_focused */);
  std::unique_ptr<Browser> browser3 =
      CreateTestBrowser(true /* is_visible */, false /* is_focused */);

  BrowserList::GetInstance()->SetLastActive(browser3.get());
  BrowserList::GetInstance()->SetLastActive(browser2.get());
  BrowserList::GetInstance()->SetLastActive(browser1.get());

  TabStripModel* tab_strip_model1 = browser1->tab_strip_model();
  CreateTestWebContents(tab_strip_model1, url1_, false /* is_active */);
  CreateTestWebContents(tab_strip_model1, url2_, true /* is_active */);
  const ukm::SourceId source_id1 = GetSourceIdForUrl(url1_);
  const ukm::SourceId source_id2 = GetSourceIdForUrl(url2_);

  TabStripModel* tab_strip_model2 = browser2->tab_strip_model();
  CreateTestWebContents(tab_strip_model2, url3_, true /* is_active */);
  const ukm::SourceId source_id3 = GetSourceIdForUrl(url3_);

  TabStripModel* tab_strip_model3 = browser3->tab_strip_model();
  CreateTestWebContents(tab_strip_model3, url4_, true /* is_active */);
  const ukm::SourceId source_id4 = GetSourceIdForUrl(url4_);

  UpdateOpenTabsURLs();

  TabProperty expected_property1;
  expected_property1.is_active = false;
  expected_property1.is_browser_focused = false;
  expected_property1.is_browser_visible = false;
  expected_property1.is_topmost_browser = false;
  expected_property1.engagement_score = 0;
  expected_property1.content_type =
      metrics::TabMetricsEvent::CONTENT_TYPE_TEXT_HTML;
  expected_property1.has_form_entry = false;

  TabProperty expected_property2;
  expected_property2.is_active = true;
  expected_property2.is_browser_focused = false;
  expected_property2.is_browser_visible = false;
  expected_property2.is_topmost_browser = false;
  expected_property2.engagement_score = 0;
  expected_property2.content_type =
      metrics::TabMetricsEvent::CONTENT_TYPE_TEXT_HTML;
  expected_property2.has_form_entry = false;

  TabProperty expected_property3;
  expected_property3.is_active = true;
  expected_property3.is_browser_focused = true;
  expected_property3.is_browser_visible = true;
  expected_property3.is_topmost_browser = true;
  expected_property3.engagement_score = 0;
  expected_property3.content_type =
      metrics::TabMetricsEvent::CONTENT_TYPE_TEXT_HTML;
  expected_property3.has_form_entry = false;

  TabProperty expected_property4;
  expected_property4.is_active = true;
  expected_property4.is_browser_focused = false;
  expected_property4.is_browser_visible = true;
  expected_property4.is_topmost_browser = false;
  expected_property4.engagement_score = 0;
  expected_property4.content_type =
      metrics::TabMetricsEvent::CONTENT_TYPE_TEXT_HTML;
  expected_property4.has_form_entry = false;

  const std::map<ukm::SourceId, TabProperty> expected_properties(
      {{source_id1, expected_property1},
       {source_id2, expected_property2},
       {source_id3, expected_property3},
       {source_id4, expected_property4}});
  CheckTabsProperties(expected_properties);

  tab_strip_model1->CloseAllTabs();
  tab_strip_model2->CloseAllTabs();
  tab_strip_model3->CloseAllTabs();
}

TEST_F(UserActivityManagerTest, Incognito) {
  std::unique_ptr<Browser> browser = CreateTestBrowser(
      true /* is_visible */, true /* is_focused */, true /* is_incognito */);
  BrowserList::GetInstance()->SetLastActive(browser.get());

  TabStripModel* tab_strip_model = browser->tab_strip_model();
  CreateTestWebContents(tab_strip_model, url1_, true /* is_active */);
  CreateTestWebContents(tab_strip_model, url2_, false /* is_active */);

  UpdateOpenTabsURLs();

  const std::map<ukm::SourceId, TabProperty> empty_map;
  CheckTabsProperties(empty_map);

  tab_strip_model->CloseAllTabs();
}

TEST_F(UserActivityManagerTest, NoOpenTabs) {
  std::unique_ptr<Browser> browser =
      CreateTestBrowser(true /* is_visible */, true /* is_focused */);

  UpdateOpenTabsURLs();

  const std::map<ukm::SourceId, TabProperty> empty_map;
  CheckTabsProperties(empty_map);
}

}  // namespace ml
}  // namespace power
}  // namespace chromeos
