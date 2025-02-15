// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/ntp/recent_tabs/recent_tabs_table_coordinator.h"

#include "base/ios/block_types.h"
#include "base/mac/foundation_util.h"
#import "ios/chrome/browser/ui/ntp/recent_tabs/recent_tabs_handset_view_controller.h"
#import "ios/chrome/browser/ui/ntp/recent_tabs/recent_tabs_mediator.h"
#import "ios/chrome/browser/ui/ntp/recent_tabs/recent_tabs_table_view_controller.h"
#import "ios/chrome/browser/ui/table_view/table_container_view_controller.h"
#import "ios/chrome/browser/ui/util/form_sheet_navigation_controller.h"
#include "ios/chrome/grit/ios_strings.h"
#include "ui/base/l10n/l10n_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// TODO(crbug.com/805135): Remove RecentTabsHandsetViewControllerCommand and
// recent_tabs_handset_view_controller.h import. We need this to dismiss for
// now, but it can be improved.
@interface RecentTabsTableCoordinator ()<RecentTabsHandsetViewControllerCommand>
// Completion block called once the recentTabsViewController is dismissed.
@property(nonatomic, copy) ProceduralBlock completion;
// Mediator being managed by this Coordinator.
@property(nonatomic, strong) RecentTabsMediator* mediator;
// ViewController being managed by this Coordinator.
@property(nonatomic, strong)
    TableContainerViewController* recentTabsContainerViewController;
@end

@implementation RecentTabsTableCoordinator
@synthesize completion = _completion;
@synthesize dispatcher = _dispatcher;
@synthesize loader = _loader;
@synthesize mediator = _mediator;
@synthesize recentTabsContainerViewController =
    _recentTabsContainerViewController;

- (void)start {
  // Initialize and configure RecentTabsTableViewController.
  RecentTabsTableViewController* recentTabsTableViewController =
      [[RecentTabsTableViewController alloc] init];
  recentTabsTableViewController.browserState = self.browserState;
  recentTabsTableViewController.loader = self.loader;
  recentTabsTableViewController.dispatcher = self.dispatcher;
  recentTabsTableViewController.handsetCommandHandler = self;

  // Initialize and configure RecentTabsMediator.
  self.mediator = [[RecentTabsMediator alloc] init];
  self.mediator.browserState = self.browserState;
  self.mediator.consumer = recentTabsTableViewController;
  [self.mediator initObservers];
  [self.mediator reloadSessions];

  // Initialize and configure RecentTabsViewController.
  self.recentTabsContainerViewController = [[TableContainerViewController alloc]
      initWithTable:recentTabsTableViewController];
  self.recentTabsContainerViewController.title =
      l10n_util::GetNSString(IDS_IOS_CONTENT_SUGGESTIONS_RECENT_TABS);
  // TODO(crbug.com/805135): Move this configuration code to
  // RecentTabsContainerVC once its created, we will use a dispatcher then.
  [self.recentTabsContainerViewController.dismissButton setTarget:self];
  [self.recentTabsContainerViewController.dismissButton
      setAction:@selector(stop)];
  self.recentTabsContainerViewController.navigationItem.rightBarButtonItem =
      self.recentTabsContainerViewController.dismissButton;

  // Present RecentTabsViewController.
  FormSheetNavigationController* navController =
      [[FormSheetNavigationController alloc]
          initWithRootViewController:self.recentTabsContainerViewController];
  [navController setModalPresentationStyle:UIModalPresentationFormSheet];
  [self.baseViewController presentViewController:navController
                                        animated:YES
                                      completion:nil];
}

- (void)stop {
  // TODO(crbug.com/805135): Move this dismissal code to RecentTabsContainerVC
  // once its created. Remove "base/mac/foundation_util.h" import then.
  RecentTabsTableViewController* recentTabsTableViewController =
      base::mac::ObjCCastStrict<RecentTabsTableViewController>(
          self.recentTabsContainerViewController.tableViewController);
  [recentTabsTableViewController dismissModals];
  [self.recentTabsContainerViewController
      dismissViewControllerAnimated:YES
                         completion:self.completion];
  self.recentTabsContainerViewController = nil;
  [self.mediator disconnect];
}

#pragma mark - RecentTabsHandsetViewControllerCommand

- (void)dismissRecentTabsWithCompletion:(void (^)())completion {
  self.completion = completion;
  [self stop];
}

@end
