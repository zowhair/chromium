// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLBAR_PUBLIC_TOOLBAR_COORDINATING_H_
#define IOS_CHROME_BROWSER_UI_TOOLBAR_PUBLIC_TOOLBAR_COORDINATING_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/ntp/new_tab_page_controller_delegate.h"
#import "ios/chrome/browser/ui/toolbar/public/side_swipe_toolbar_interacting.h"

@protocol TabHistoryUIUpdater;

// Defines a class coordinating the interactions with the toolbar.
@protocol ToolbarCoordinating<NewTabPageControllerDelegate,
                              SideSwipeToolbarInteracting>

// Updates the tools menu, changing its content to reflect the current page.
- (void)updateToolsMenu;

- (id<TabHistoryUIUpdater>)tabHistoryUIUpdater;

@end

#endif  // IOS_CHROME_BROWSER_UI_TOOLBAR_PUBLIC_TOOLBAR_COORDINATING_H_
