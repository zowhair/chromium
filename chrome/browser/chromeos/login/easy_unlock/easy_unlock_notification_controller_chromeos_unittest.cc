// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/easy_unlock/easy_unlock_notification_controller_chromeos.h"

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/notifications/notification_display_service_tester.h"
#include "chrome/test/base/browser_with_test_window_test.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "ui/message_center/public/cpp/notification.h"
#include "ui/message_center/public/cpp/notification_types.h"

namespace {

const char kPhoneName[] = "Nexus 6";

class TestableNotificationController
    : public chromeos::EasyUnlockNotificationControllerChromeOS {
 public:
  explicit TestableNotificationController(Profile* profile)
      : EasyUnlockNotificationControllerChromeOS(profile) {}

  ~TestableNotificationController() override {}

  // EasyUnlockNotificationControllerChromeOS:
  MOCK_METHOD0(LaunchEasyUnlockSettings, void());
  MOCK_METHOD0(LockScreen, void());

 private:
  DISALLOW_COPY_AND_ASSIGN(TestableNotificationController);
};

}  // namespace

class EasyUnlockNotificationControllerChromeOSTest
    : public BrowserWithTestWindowTest {
 protected:
  EasyUnlockNotificationControllerChromeOSTest() {}

  ~EasyUnlockNotificationControllerChromeOSTest() override {}

  void SetUp() override {
    BrowserWithTestWindowTest::SetUp();

    display_service_ =
        std::make_unique<NotificationDisplayServiceTester>(profile());
    notification_controller_ =
        std::make_unique<testing::StrictMock<TestableNotificationController>>(
            profile());
  }

  // const content::TestBrowserThreadBundle thread_bundle_;
  // TestMessageCenter message_center_;
  std::unique_ptr<testing::StrictMock<TestableNotificationController>>
      notification_controller_;
  std::unique_ptr<NotificationDisplayServiceTester> display_service_;

 private:
  DISALLOW_COPY_AND_ASSIGN(EasyUnlockNotificationControllerChromeOSTest);
};

TEST_F(EasyUnlockNotificationControllerChromeOSTest,
       TestShowChromebookAddedNotification) {
  const char kNotificationId[] = "easyunlock_notification_ids.chromebook_added";

  notification_controller_->ShowChromebookAddedNotification();
  base::Optional<message_center::Notification> notification =
      display_service_->GetNotification(kNotificationId);
  ASSERT_TRUE(notification);
  ASSERT_EQ(1u, notification->buttons().size());
  EXPECT_EQ(message_center::SYSTEM_PRIORITY, notification->priority());

  // Clicking notification button should launch settings.
  EXPECT_CALL(*notification_controller_, LaunchEasyUnlockSettings());
  notification->ButtonClick(0);

  // Clicking the notification itself should also launch settings.
  EXPECT_CALL(*notification_controller_, LaunchEasyUnlockSettings());
  notification->Click();
}

TEST_F(EasyUnlockNotificationControllerChromeOSTest,
       TestShowPairingChangeNotification) {
  const char kNotificationId[] = "easyunlock_notification_ids.pairing_change";

  notification_controller_->ShowPairingChangeNotification();
  base::Optional<message_center::Notification> notification =
      display_service_->GetNotification(kNotificationId);
  ASSERT_TRUE(notification);
  ASSERT_EQ(2u, notification->buttons().size());
  EXPECT_EQ(message_center::SYSTEM_PRIORITY, notification->priority());

  // Clicking 1st notification button should lock screen settings.
  EXPECT_CALL(*notification_controller_, LockScreen());
  notification->ButtonClick(0);

  // Clicking 2nd notification button should launch settings.
  EXPECT_CALL(*notification_controller_, LaunchEasyUnlockSettings());
  notification->ButtonClick(1);

  // Clicking the notification itself should do nothing.
  notification->Click();
}

TEST_F(EasyUnlockNotificationControllerChromeOSTest,
       TestShowPairingChangeAppliedNotification) {
  const char kNotificationId[] =
      "easyunlock_notification_ids.pairing_change_applied";

  notification_controller_->ShowPairingChangeAppliedNotification(kPhoneName);
  base::Optional<message_center::Notification> notification =
      display_service_->GetNotification(kNotificationId);
  ASSERT_TRUE(notification);
  ASSERT_EQ(1u, notification->buttons().size());
  EXPECT_EQ(message_center::SYSTEM_PRIORITY, notification->priority());

  // Check that the phone name is in the notification message.
  EXPECT_NE(std::string::npos,
            notification->message().find(base::UTF8ToUTF16(kPhoneName)));

  // Clicking notification button should launch settings.
  EXPECT_CALL(*notification_controller_, LaunchEasyUnlockSettings());
  notification->ButtonClick(0);

  // Clicking the notification itself should also launch settings.
  EXPECT_CALL(*notification_controller_, LaunchEasyUnlockSettings());
  notification->Click();
}

TEST_F(EasyUnlockNotificationControllerChromeOSTest,
       PairingAppliedRemovesPairingChange) {
  const char kPairingChangeId[] = "easyunlock_notification_ids.pairing_change";
  const char kPairingAppliedId[] =
      "easyunlock_notification_ids.pairing_change_applied";

  notification_controller_->ShowPairingChangeNotification();
  EXPECT_TRUE(display_service_->GetNotification(kPairingChangeId));

  notification_controller_->ShowPairingChangeAppliedNotification(kPhoneName);
  EXPECT_FALSE(display_service_->GetNotification(kPairingChangeId));
  EXPECT_TRUE(display_service_->GetNotification(kPairingAppliedId));
}

TEST_F(EasyUnlockNotificationControllerChromeOSTest,
       TestShowPromotionNotification) {
  const char kNotificationId[] = "easyunlock_notification_ids.promotion";

  notification_controller_->ShowPromotionNotification();
  base::Optional<message_center::Notification> notification =
      display_service_->GetNotification(kNotificationId);
  ASSERT_TRUE(notification);
  ASSERT_EQ(1u, notification->buttons().size());
  EXPECT_EQ(message_center::SYSTEM_PRIORITY, notification->priority());

  // Clicking notification button should launch settings.
  EXPECT_CALL(*notification_controller_, LaunchEasyUnlockSettings());
  notification->ButtonClick(0);

  // Clicking the notification itself should also launch settings.
  EXPECT_CALL(*notification_controller_, LaunchEasyUnlockSettings());
  notification->Click();
}
