// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/content_suggestions/content_suggestions_collection_utils.h"

#include <memory>

#include "ios/chrome/browser/ui/ui_util.h"
#import "ios/chrome/test/base/scoped_block_swizzler.h"
#include "testing/platform_test.h"
#import "third_party/ocmock/OCMock/OCMock.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace content_suggestions {

class ContentSuggestionsCollectionUtilsTest : public PlatformTest {
 public:
  void SetAsIPad() {
    device_type_swizzler_ = std::make_unique<ScopedBlockSwizzler>(
        [UIDevice class], @selector(userInterfaceIdiom),
        ^UIUserInterfaceIdiom(id self) {
          return UIUserInterfaceIdiomPad;
        });
  }
  void SetAsIPhone() {
    device_type_swizzler_ = std::make_unique<ScopedBlockSwizzler>(
        [UIDevice class], @selector(userInterfaceIdiom),
        ^UIUserInterfaceIdiom(id self) {
          return UIUserInterfaceIdiomPhone;
        });
  }
  void SetAsPortrait() {
    orientation_swizzler_ = std::make_unique<ScopedBlockSwizzler>(
        [UIApplication class], @selector(statusBarOrientation),
        ^UIInterfaceOrientation(id self) {
          return UIInterfaceOrientationPortrait;
        });
  }
  void SetAsLandscape() {
    orientation_swizzler_ = std::make_unique<ScopedBlockSwizzler>(
        [UIApplication class], @selector(statusBarOrientation),
        ^UIInterfaceOrientation(id self) {
          return UIInterfaceOrientationLandscapeLeft;
        });
  }

 private:
  std::unique_ptr<ScopedBlockSwizzler> device_type_swizzler_;
  std::unique_ptr<ScopedBlockSwizzler> orientation_swizzler_;
};

TEST_F(ContentSuggestionsCollectionUtilsTest, centeredTilesMarginIPhone6) {
  // Setup.
  SetAsIPhone();

  // Action.
  CGFloat result = centeredTilesMarginForWidth(374);

  // Tests.
  EXPECT_EQ(17, result);
}

TEST_F(ContentSuggestionsCollectionUtilsTest, centeredTilesMarginIPad) {
  // Setup.
  SetAsIPad();

  // Action.
  CGFloat result = centeredTilesMarginForWidth(700);

  // Tests.
  EXPECT_EQ(168, result);
}

TEST_F(ContentSuggestionsCollectionUtilsTest, doodleFrameIPad) {
  // Setup.
  SetAsIPad();

  // Action.
  CGFloat height = doodleHeight(YES);
  CGFloat topMargin = doodleTopMargin(YES);
  CGFloat topMarginNoToolbar = doodleTopMargin(NO);

  // Test.
  if (IsUIRefreshPhase1Enabled()) {
    EXPECT_EQ(68, height);
    EXPECT_EQ(48, topMargin);
  } else {
    EXPECT_EQ(120, height);
    EXPECT_EQ(82, topMargin);
    EXPECT_EQ(82, topMarginNoToolbar);
  }
}

TEST_F(ContentSuggestionsCollectionUtilsTest, doodleFrameIPhonePortrait) {
  // Setup.
  SetAsIPhone();
  SetAsPortrait();

  // Action.
  CGFloat heightLogo = doodleHeight(YES);
  CGFloat heightNoLogo = doodleHeight(NO);
  CGFloat topMargin = doodleTopMargin(YES);
  CGFloat topMarginNoToolbar = doodleTopMargin(NO);

  // Test.
  if (IsUIRefreshPhase1Enabled()) {
    EXPECT_EQ(68, heightLogo);
    EXPECT_EQ(60, heightNoLogo);
    EXPECT_EQ(48, topMargin);
  } else {
    EXPECT_EQ(120, heightLogo);
    EXPECT_EQ(60, heightNoLogo);
    EXPECT_EQ(56, topMargin);
    EXPECT_EQ(0, topMarginNoToolbar);
  }
}

TEST_F(ContentSuggestionsCollectionUtilsTest, doodleFrameIPhoneLandscape) {
  // Setup.
  SetAsIPhone();
  SetAsLandscape();

  // Action.
  CGFloat heightLogo = doodleHeight(YES);
  CGFloat heightNoLogo = doodleHeight(NO);
  CGFloat topMargin = doodleTopMargin(YES);
  CGFloat topMarginNoToolbar = doodleTopMargin(NO);

  // Test.
  if (IsUIRefreshPhase1Enabled()) {
    EXPECT_EQ(68, heightLogo);
    EXPECT_EQ(60, heightNoLogo);
    EXPECT_EQ(48, topMargin);
  } else {
    EXPECT_EQ(120, heightLogo);
    EXPECT_EQ(60, heightNoLogo);
    EXPECT_EQ(56, topMargin);
    EXPECT_EQ(0, topMarginNoToolbar);
  }
}

TEST_F(ContentSuggestionsCollectionUtilsTest, searchFieldFrameIPad) {
  // Setup.
  SetAsIPad();
  CGFloat width = 500;
  CGFloat largeIPadWidth = 1366;
  CGFloat margin = centeredTilesMarginForWidth(width);

  // Action.
  CGFloat resultWidth = searchFieldWidth(width);
  CGFloat resultWidthLargeIPad = searchFieldWidth(largeIPadWidth);
  CGFloat topMargin = searchFieldTopMargin();

  // Test.
  if (IsUIRefreshPhase1Enabled()) {
    EXPECT_EQ(32, topMargin);
    EXPECT_EQ(343, resultWidth);
    EXPECT_EQ(343, resultWidthLargeIPad);
  } else {
    EXPECT_EQ(82, topMargin);
    EXPECT_EQ(width - 2 * margin, resultWidth);
    EXPECT_EQ(largeIPadWidth - 400, resultWidthLargeIPad);
  }
}

TEST_F(ContentSuggestionsCollectionUtilsTest, searchFieldFrameIPhonePortrait) {
  // Setup.
  SetAsIPhone();
  SetAsPortrait();
  CGFloat width = 500;
  CGFloat margin = centeredTilesMarginForWidth(width);

  // Action.
  CGFloat resultWidth = searchFieldWidth(width);
  CGFloat topMargin = searchFieldTopMargin();

  // Test.
  if (IsUIRefreshPhase1Enabled()) {
    EXPECT_EQ(32, topMargin);
    EXPECT_EQ(343, resultWidth);
  } else {
    EXPECT_EQ(32, topMargin);
    EXPECT_EQ(width - 2 * margin, resultWidth);
  }
}

TEST_F(ContentSuggestionsCollectionUtilsTest, searchFieldFrameIPhoneLandscape) {
  // Setup.
  SetAsIPhone();
  SetAsLandscape();
  CGFloat width = 500;
  CGFloat margin = centeredTilesMarginForWidth(width);

  // Action.
  CGFloat resultWidth = searchFieldWidth(width);
  CGFloat topMargin = searchFieldTopMargin();

  // Test.
  if (IsUIRefreshPhase1Enabled()) {
    EXPECT_EQ(32, topMargin);
    EXPECT_EQ(343, resultWidth);
  } else {
    EXPECT_EQ(32, topMargin);
    EXPECT_EQ(width - 2 * margin, resultWidth);
  }
}

TEST_F(ContentSuggestionsCollectionUtilsTest, heightForLogoHeaderIPad) {
  // Setup.
  SetAsIPad();

  // Action, tests.
  if (IsUIRefreshPhase1Enabled()) {
    EXPECT_EQ(214, heightForLogoHeader(YES, YES, YES));
    EXPECT_EQ(238, heightForLogoHeader(YES, NO, YES));
    EXPECT_EQ(214, heightForLogoHeader(YES, YES, NO));
    EXPECT_EQ(238, heightForLogoHeader(YES, NO, NO));
  } else {
    EXPECT_EQ(350, heightForLogoHeader(YES, YES, YES));
    EXPECT_EQ(374, heightForLogoHeader(YES, NO, YES));
    EXPECT_EQ(350, heightForLogoHeader(YES, YES, NO));
    EXPECT_EQ(374, heightForLogoHeader(YES, NO, NO));
  }
}

TEST_F(ContentSuggestionsCollectionUtilsTest, heightForLogoHeaderIPhone) {
  // Setup.
  SetAsIPhone();

  // Action, tests.
  if (IsUIRefreshPhase1Enabled()) {
    EXPECT_EQ(214, heightForLogoHeader(YES, YES, YES));
    EXPECT_EQ(214, heightForLogoHeader(YES, NO, YES));
    EXPECT_EQ(214, heightForLogoHeader(YES, YES, NO));
    EXPECT_EQ(214, heightForLogoHeader(YES, NO, NO));
  } else {
    EXPECT_EQ(274, heightForLogoHeader(YES, YES, YES));
    EXPECT_EQ(274, heightForLogoHeader(YES, NO, YES));
    EXPECT_EQ(218, heightForLogoHeader(YES, YES, NO));
    EXPECT_EQ(218, heightForLogoHeader(YES, NO, NO));
  }
}

TEST_F(ContentSuggestionsCollectionUtilsTest, SizeIPhone6) {
  // Setup.
  SetAsIPhone();

  // Test.
  EXPECT_EQ(4U, numberOfTilesForWidth(360));
}

TEST_F(ContentSuggestionsCollectionUtilsTest, SizeIPhone5) {
  // Setup.
  SetAsIPhone();

  // Test.
  if (IsUIRefreshPhase1Enabled()) {
    EXPECT_EQ(4U, numberOfTilesForWidth(320));
  } else {
    EXPECT_EQ(3U, numberOfTilesForWidth(320));
  }
}

// Test for iPad portrait and iPhone landscape.
TEST_F(ContentSuggestionsCollectionUtilsTest, SizeLarge) {
  // Test.
  EXPECT_EQ(4U, numberOfTilesForWidth(720));
}

TEST_F(ContentSuggestionsCollectionUtilsTest, SizeIPadSplit) {
  // Setup.
  SetAsIPad();

  // Test.
  if (IsUIRefreshPhase1Enabled()) {
    EXPECT_EQ(4U, numberOfTilesForWidth(360));
  } else {
    EXPECT_EQ(3U, numberOfTilesForWidth(360));
  }
}

TEST_F(ContentSuggestionsCollectionUtilsTest, NearestAncestor) {
  // Setup.
  // The types of the view has no meaning.
  UILabel* rootView = [[UILabel alloc] init];
  UIView* intermediaryView = [[UIView alloc] init];
  UIScrollView* leafView = [[UIScrollView alloc] init];
  [rootView addSubview:intermediaryView];
  [intermediaryView addSubview:leafView];

  // Tests.
  EXPECT_EQ(leafView, nearestAncestor(leafView, [UIScrollView class]));
  EXPECT_EQ(leafView, nearestAncestor(leafView, [UIView class]));
  EXPECT_EQ(rootView, nearestAncestor(leafView, [UILabel class]));
  EXPECT_EQ(nil, nearestAncestor(leafView, [UITextView class]));
}

}  // namespace content_suggestions
