// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/download/download_manager_coordinator.h"

#import <StoreKit/StoreKit.h>
#import <UIKit/UIKit.h>

#include "base/files/file_util.h"
#include "base/mac/foundation_util.h"
#include "base/run_loop.h"
#include "base/strings/sys_string_conversions.h"
#include "ios/chrome/browser/download/download_directory_util.h"
#import "ios/chrome/browser/download/download_manager_tab_helper.h"
#import "ios/chrome/browser/ui/download/download_manager_view_controller.h"
#import "ios/chrome/test/fakes/fake_contained_presenter.h"
#import "ios/chrome/test/fakes/fake_document_interaction_controller.h"
#import "ios/chrome/test/scoped_key_window.h"
#import "ios/testing/wait_util.h"
#import "ios/web/public/test/fakes/fake_download_task.h"
#import "ios/web/public/test/fakes/test_web_state.h"
#include "ios/web/public/test/test_web_thread_bundle.h"
#include "net/url_request/url_fetcher_response_writer.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"
#import "third_party/ocmock/OCMock/OCMock.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using testing::WaitUntilConditionOrTimeout;
using testing::kWaitForUIElementTimeout;

namespace {

// Constants for configuring a fake download task.
const char kTestUrl[] = "https://chromium.test/download.txt";
const char kTestMimeType[] = "text/html";
const int64_t kTestTotalBytes = 10;
const int64_t kTestReceivedBytes = 0;
NSString* const kTestSuggestedFileName = @"file.zip";

// Creates a fake download task for testing.
std::unique_ptr<web::FakeDownloadTask> CreateTestTask() {
  auto task =
      std::make_unique<web::FakeDownloadTask>(GURL(kTestUrl), kTestMimeType);
  task->SetTotalBytes(kTestTotalBytes);
  task->SetReceivedBytes(kTestReceivedBytes);
  task->SetSuggestedFilename(base::SysNSStringToUTF16(kTestSuggestedFileName));
  return task;
}

// Substitutes real TabHelper for testing.
class StubTabHelper : public DownloadManagerTabHelper {
 public:
  StubTabHelper(web::WebState* web_state)
      : DownloadManagerTabHelper(web_state, /*delegate=*/nullptr) {}
  DISALLOW_COPY_AND_ASSIGN(StubTabHelper);
};

}  // namespace

// Test fixture for testing DownloadManagerCoordinator class.
class DownloadManagerCoordinatorTest : public PlatformTest {
 protected:
  DownloadManagerCoordinatorTest()
      : presenter_([[FakeContainedPresenter alloc] init]),
        base_view_controller_([[UIViewController alloc] init]),
        document_interaction_controller_class_(
            OCMClassMock([UIDocumentInteractionController class])),
        tab_helper_(&web_state_),
        coordinator_([[DownloadManagerCoordinator alloc]
            initWithBaseViewController:base_view_controller_]) {
    [scoped_key_window_.Get() setRootViewController:base_view_controller_];
    coordinator_.presenter = presenter_;
  }
  ~DownloadManagerCoordinatorTest() override {
    [document_interaction_controller_class_ stopMocking];
  }

  FakeContainedPresenter* presenter_;
  UIViewController* base_view_controller_;
  ScopedKeyWindow scoped_key_window_;
  web::TestWebState web_state_;
  id document_interaction_controller_class_;
  StubTabHelper tab_helper_;
  DownloadManagerCoordinator* coordinator_;
};

// Tests starting the coordinator. Verifies that view controller is presented
// without animation (default configuration) and that
// DownloadManagerViewController is propertly configured and presented.
TEST_F(DownloadManagerCoordinatorTest, Start) {
  auto task = CreateTestTask();
  coordinator_.downloadTask = task.get();
  [coordinator_ start];

  // By default coordinator presents without animation.
  EXPECT_FALSE(presenter_.lastPresentationWasAnimated);

  // Verify that presented view controller is DownloadManagerViewController.
  EXPECT_EQ(1U, base_view_controller_.childViewControllers.count);
  DownloadManagerViewController* viewController =
      base_view_controller_.childViewControllers.firstObject;
  ASSERT_EQ([DownloadManagerViewController class], [viewController class]);

  // Verify that DownloadManagerViewController configuration matches download
  // task.
  EXPECT_FALSE(viewController.actionButton.hidden);
  EXPECT_NSEQ(@"file.zip - 10 bytes", viewController.statusLabel.text);
  EXPECT_NSEQ(@"Download",
              [viewController.actionButton titleForState:UIControlStateNormal]);

  // Stop to avoid holding a dangling pointer to destroyed task.
  [coordinator_ stop];
}

// Tests stopping coordinator. Verifies that hiding web states dismisses the
// presented view controller and download task is reset to null (to prevent a
// stale raw pointer).
TEST_F(DownloadManagerCoordinatorTest, Stop) {
  web::FakeDownloadTask task(GURL(kTestUrl), kTestMimeType);
  coordinator_.downloadTask = &task;
  [coordinator_ start];
  [coordinator_ stop];

  // Verify that child view controller is removed and download task is set to
  // null.
  EXPECT_EQ(0U, base_view_controller_.childViewControllers.count);
  EXPECT_FALSE(coordinator_.downloadTask);
}

// Tests downloadManagerTabHelper:didCreateDownload:webStateIsVisible: callback
// for visible web state. Verifies that coordinator's properties are set up and
// that DownloadManagerViewController is properly configured and presented with
// animation.
TEST_F(DownloadManagerCoordinatorTest, DelegateCreatedDownload) {
  auto task = CreateTestTask();
  [coordinator_ downloadManagerTabHelper:&tab_helper_
                       didCreateDownload:task.get()
                       webStateIsVisible:YES];

  // Verify that coordinator's properties are set up.
  EXPECT_EQ(task.get(), coordinator_.downloadTask);
  EXPECT_TRUE(coordinator_.animatesPresentation);

  // First presentation of Download Manager UI should be animated.
  EXPECT_TRUE(presenter_.lastPresentationWasAnimated);

  // Verify that presented view controller is DownloadManagerViewController.
  EXPECT_EQ(1U, base_view_controller_.childViewControllers.count);
  DownloadManagerViewController* viewController =
      base_view_controller_.childViewControllers.firstObject;
  ASSERT_EQ([DownloadManagerViewController class], [viewController class]);

  // Verify that DownloadManagerViewController configuration matches download
  // task.
  EXPECT_NSEQ(@"file.zip - 10 bytes", viewController.statusLabel.text);
  EXPECT_FALSE(viewController.actionButton.hidden);
  EXPECT_NSEQ(@"Download",
              [viewController.actionButton titleForState:UIControlStateNormal]);

  // Stop to avoid holding a dangling pointer to destroyed task.
  [coordinator_ stop];
}

// Tests calling downloadManagerTabHelper:didCreateDownload:webStateIsVisible:
// callback twice. Second call should replace the old download task with the new
// one.
TEST_F(DownloadManagerCoordinatorTest, DelegateReplacedDownload) {
  auto task = CreateTestTask();
  task->SetDone(true);
  [coordinator_ downloadManagerTabHelper:&tab_helper_
                       didCreateDownload:task.get()
                       webStateIsVisible:YES];

  // Verify that presented view controller is DownloadManagerViewController.
  EXPECT_EQ(1U, base_view_controller_.childViewControllers.count);
  DownloadManagerViewController* viewController =
      base_view_controller_.childViewControllers.firstObject;
  ASSERT_EQ([DownloadManagerViewController class], [viewController class]);

  // Verify that DownloadManagerViewController configuration matches download
  // task.
  EXPECT_NSEQ(@"file.zip", viewController.statusLabel.text);
  EXPECT_FALSE(viewController.actionButton.hidden);
  EXPECT_NSEQ(@"Open in…",
              [viewController.actionButton titleForState:UIControlStateNormal]);

  // Replace download task with a new one.
  auto new_task = CreateTestTask();
  [coordinator_ downloadManagerTabHelper:&tab_helper_
                       didCreateDownload:new_task.get()
                       webStateIsVisible:YES];

  // Verify that DownloadManagerViewController configuration matches new
  // download task.
  EXPECT_NSEQ(@"file.zip - 10 bytes", viewController.statusLabel.text);
  EXPECT_FALSE(viewController.actionButton.hidden);
  EXPECT_NSEQ(@"Download",
              [viewController.actionButton titleForState:UIControlStateNormal]);

  // Stop to avoid holding a dangling pointer to destroyed task.
  [coordinator_ stop];
}

// Tests downloadManagerTabHelper:didCreateDownload:webStateIsVisible: callback
// for hidden web state. Verifies that coordinator ignores callback from
// a background tab.
TEST_F(DownloadManagerCoordinatorTest,
       DelegateCreatedDownloadForHiddenWebState) {
  auto task = CreateTestTask();
  [coordinator_ downloadManagerTabHelper:&tab_helper_
                       didCreateDownload:task.get()
                       webStateIsVisible:NO];

  // Background tab should not present Download Manager UI.
  EXPECT_EQ(0U, base_view_controller_.childViewControllers.count);
}

// Tests downloadManagerTabHelper:didHideDownload: callback. Verifies that
// hiding web states dismisses the presented view controller and download task
// is reset to null (to prevent a stale raw pointer).
TEST_F(DownloadManagerCoordinatorTest, DelegateHideDownload) {
  auto task = CreateTestTask();
  [coordinator_ downloadManagerTabHelper:&tab_helper_
                       didCreateDownload:task.get()
                       webStateIsVisible:YES];
  [coordinator_ downloadManagerTabHelper:&tab_helper_
                         didHideDownload:task.get()];

  // Verify that child view controller is removed and download task is set to
  // null.
  EXPECT_EQ(0U, base_view_controller_.childViewControllers.count);
  EXPECT_FALSE(coordinator_.downloadTask);
}

// Tests downloadManagerTabHelper:didShowDownload: callback. Verifies that
// showing web state presents download manager UI for that web state.
TEST_F(DownloadManagerCoordinatorTest, DelegateShowDownload) {
  auto task = CreateTestTask();
  [coordinator_ downloadManagerTabHelper:&tab_helper_
                         didShowDownload:task.get()];

  // Only first presentation is animated. Switching between tab should create
  // the impression that UI was never dismissed.
  EXPECT_FALSE(presenter_.lastPresentationWasAnimated);

  // Verify that presented view controller is DownloadManagerViewController.
  EXPECT_EQ(1U, base_view_controller_.childViewControllers.count);
  DownloadManagerViewController* viewController =
      base_view_controller_.childViewControllers.firstObject;
  ASSERT_EQ([DownloadManagerViewController class], [viewController class]);

  // Verify that DownloadManagerViewController configuration matches download
  // task for shown web state.
  EXPECT_NSEQ(@"file.zip - 10 bytes", viewController.statusLabel.text);
  EXPECT_FALSE(viewController.actionButton.hidden);

  // Stop to avoid holding a dangling pointer to destroyed task.
  [coordinator_ stop];
}

// Tests closing view controller. Coordinator should be stopped and task
// cancelled.
TEST_F(DownloadManagerCoordinatorTest, Close) {
  web::FakeDownloadTask task(GURL(kTestUrl), kTestMimeType);
  coordinator_.downloadTask = &task;
  [coordinator_ start];

  EXPECT_EQ(1U, base_view_controller_.childViewControllers.count);
  DownloadManagerViewController* viewController =
      base_view_controller_.childViewControllers.firstObject;
  ASSERT_EQ([DownloadManagerViewController class], [viewController class]);
  [viewController.delegate
      downloadManagerViewControllerDidClose:viewController];

  // Verify that child view controller is removed, download task is set to null
  // and download task is cancelled.
  EXPECT_EQ(0U, base_view_controller_.childViewControllers.count);
  EXPECT_FALSE(coordinator_.downloadTask);
  EXPECT_EQ(web::DownloadTask::State::kCancelled, task.GetState());

  // Stop to avoid holding a dangling pointer to destroyed task.
  [coordinator_ stop];
}

// Tests presenting Install Google Drive dialog. Coordinator presents StoreKit
// dialog and hides Install Google Drive button.
TEST_F(DownloadManagerCoordinatorTest, InstallDrive) {
  web::FakeDownloadTask task(GURL(kTestUrl), kTestMimeType);
  coordinator_.downloadTask = &task;
  [coordinator_ start];

  EXPECT_EQ(1U, base_view_controller_.childViewControllers.count);
  DownloadManagerViewController* viewController =
      base_view_controller_.childViewControllers.firstObject;
  ASSERT_EQ([DownloadManagerViewController class], [viewController class]);

  // Make Install Google Drive UI visible.
  [viewController setInstallDriveButtonVisible:YES animated:NO];
  // The button itself is never hidden, but the superview which contains the
  // button changes it's alpha.
  ASSERT_EQ(1.0f, viewController.installDriveButton.superview.alpha);

  [viewController.delegate
      installDriveForDownloadManagerViewController:viewController];

  // Verify that Store Kit dialog was presented.
  EXPECT_TRUE(WaitUntilConditionOrTimeout(kWaitForUIElementTimeout, ^{
    return [base_view_controller_.presentedViewController class] ==
           [SKStoreProductViewController class];
  }));

  // Verify that Install Google Drive UI is hidden.
  EXPECT_TRUE(WaitUntilConditionOrTimeout(kWaitForUIElementTimeout, ^{
    return viewController.installDriveButton.superview.alpha == 0.0f;
  }));

  // Stop to avoid holding a dangling pointer to destroyed task.
  [coordinator_ stop];
}

// Tests presenting Open In... menu.
TEST_F(DownloadManagerCoordinatorTest, OpenIn) {
  web::TestWebThreadBundle thread_bundle;

  web::FakeDownloadTask task(GURL(kTestUrl), kTestMimeType);
  task.SetSuggestedFilename(base::SysNSStringToUTF16(kTestSuggestedFileName));
  coordinator_.downloadTask = &task;
  [coordinator_ start];

  EXPECT_EQ(1U, base_view_controller_.childViewControllers.count);
  DownloadManagerViewController* viewController =
      base_view_controller_.childViewControllers.firstObject;
  ASSERT_EQ([DownloadManagerViewController class], [viewController class]);

  // Start and complete the download.
  base::FilePath path;
  ASSERT_TRUE(base::GetTempDir(&path));
  task.Start(std::make_unique<net::URLFetcherFileWriter>(
      base::ThreadTaskRunnerHandle::Get(), path));
  task.SetDone(true);

  // Stub UIDocumentInteractionController.
  FakeDocumentInteractionController* document_interaction_controller =
      [[FakeDocumentInteractionController alloc] init];
  NSURL* url = [NSURL fileURLWithPath:base::SysUTF8ToNSString(path.value())];
  OCMStub(
      [document_interaction_controller_class_ interactionControllerWithURL:url])
      .andReturn(document_interaction_controller);

  // Present Open In... menu.
  UILayoutGuide* guide = [[UILayoutGuide alloc] init];
  UIView* view = [[UIView alloc] init];
  [view addLayoutGuide:guide];
  ASSERT_FALSE(document_interaction_controller.presentedOpenInMenu);
  [viewController.delegate downloadManagerViewController:viewController
                        presentOpenInMenuWithLayoutGuide:guide];
  ASSERT_TRUE(document_interaction_controller.presentedOpenInMenu);
  ASSERT_TRUE(CGRectEqualToRect(
      CGRectZero, document_interaction_controller.presentedOpenInMenu.rect));
  ASSERT_EQ(view, document_interaction_controller.presentedOpenInMenu.view);
  ASSERT_TRUE(document_interaction_controller.presentedOpenInMenu.animated);
}

// Tests closing view controller while the download is in progress. Coordinator
// should present the confirmation dialog.
TEST_F(DownloadManagerCoordinatorTest, CloseInProgressDownload) {
  web::FakeDownloadTask task(GURL(kTestUrl), kTestMimeType);
  task.Start(std::make_unique<net::URLFetcherStringWriter>());
  coordinator_.downloadTask = &task;
  [coordinator_ start];

  EXPECT_EQ(1U, base_view_controller_.childViewControllers.count);
  DownloadManagerViewController* viewController =
      base_view_controller_.childViewControllers.firstObject;
  ASSERT_EQ([DownloadManagerViewController class], [viewController class]);
  [viewController.delegate
      downloadManagerViewControllerDidClose:viewController];

  // Verify that UIAlert is presented.
  ASSERT_TRUE([base_view_controller_.presentedViewController
      isKindOfClass:[UIAlertController class]]);
  UIAlertController* alert = base::mac::ObjCCast<UIAlertController>(
      base_view_controller_.presentedViewController);
  EXPECT_NSEQ(@"Cancel Download?", alert.title);
  EXPECT_FALSE(alert.message);

  // Stop to avoid holding a dangling pointer to destroyed task.
  [coordinator_ stop];

  // |stop| should dismiss the apert.
  ASSERT_TRUE(WaitUntilConditionOrTimeout(testing::kWaitForUIElementTimeout, ^{
    return !base_view_controller_.presentedViewController;
  }));
}

// Tests downloadManagerTabHelper:decidePolicyForDownload:completionHandler:.
// Coordinator should present the confirmation dialog.
TEST_F(DownloadManagerCoordinatorTest, DecidePolicyForDownload) {
  web::FakeDownloadTask task(GURL(kTestUrl), kTestMimeType);
  [coordinator_ downloadManagerTabHelper:&tab_helper_
                 decidePolicyForDownload:&task
                       completionHandler:^(NewDownloadPolicy){
                       }];

  // Verify that UIAlert is presented.
  ASSERT_TRUE([base_view_controller_.presentedViewController
      isKindOfClass:[UIAlertController class]]);
  UIAlertController* alert = base::mac::ObjCCast<UIAlertController>(
      base_view_controller_.presentedViewController);
  EXPECT_NSEQ(@"Start New Download?", alert.title);
  EXPECT_NSEQ(@"This will stop all progress for your current download.",
              alert.message);

  // Stop to avoid holding a dangling pointer to destroyed task.
  [coordinator_ stop];

  // |stop| should dismiss the apert.
  ASSERT_TRUE(WaitUntilConditionOrTimeout(testing::kWaitForUIElementTimeout, ^{
    return !base_view_controller_.presentedViewController;
  }));
}

// Tests starting the download. Verifies that download task is started and its
// file writer is configured to write into download directory.
TEST_F(DownloadManagerCoordinatorTest, StartDownload) {
  web::TestWebThreadBundle thread_bundle;

  web::FakeDownloadTask task(GURL(kTestUrl), kTestMimeType);
  task.SetSuggestedFilename(base::SysNSStringToUTF16(kTestSuggestedFileName));
  web::DownloadTask* task_ptr = &task;
  coordinator_.downloadTask = &task;
  [coordinator_ start];

  DownloadManagerViewController* viewController =
      base_view_controller_.childViewControllers.firstObject;
  ASSERT_EQ([DownloadManagerViewController class], [viewController class]);
  [viewController.delegate
      downloadManagerViewControllerDidStartDownload:viewController];

  // Starting download is async for model.
  ASSERT_TRUE(WaitUntilConditionOrTimeout(testing::kWaitForDownloadTimeout, ^{
    base::RunLoop().RunUntilIdle();
    return task_ptr->GetState() == web::DownloadTask::State::kInProgress;
  }));

  // Download file should be located in download directory.
  base::FilePath file = task.GetResponseWriter()->AsFileWriter()->file_path();
  base::FilePath download_dir;
  ASSERT_TRUE(GetDownloadsDirectory(&download_dir));
  EXPECT_TRUE(download_dir.IsParent(file));

  // Stop to avoid holding a dangling pointer to destroyed task.
  [coordinator_ stop];
}

// Tests that viewController returns correct view controller if coordinator is
// started and nil when stopped.
TEST_F(DownloadManagerCoordinatorTest, ViewController) {
  auto task = CreateTestTask();
  coordinator_.downloadTask = task.get();
  ASSERT_FALSE(coordinator_.viewController);
  [coordinator_ start];

  // Verify that presented view controller is DownloadManagerViewController.
  EXPECT_EQ(1U, base_view_controller_.childViewControllers.count);
  DownloadManagerViewController* viewController =
      base_view_controller_.childViewControllers.firstObject;
  ASSERT_EQ([DownloadManagerViewController class], [viewController class]);

  // Verify view controller property.
  EXPECT_NSEQ(viewController, coordinator_.viewController);

  // Stop to avoid holding a dangling pointer to destroyed task.
  [coordinator_ stop];
  EXPECT_FALSE(coordinator_.viewController);
}
