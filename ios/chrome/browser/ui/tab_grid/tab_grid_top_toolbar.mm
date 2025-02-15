// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/tab_grid/tab_grid_top_toolbar.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// Height of the toolbar.
const CGFloat kToolbarHeight = 44.0f;
// Height of the segmented control. The segmented control should have an
// intrinsic width.
const CGFloat kSegmentedControlHeight = 30.0f;
}  // namespace

@implementation TabGridTopToolbar
@synthesize leadingButton = _leadingButton;
@synthesize trailingButton = _trailingButton;
@synthesize segmentedControl = _segmentedControl;

- (instancetype)init {
  if (self = [super initWithFrame:CGRectZero]) {
    UIVisualEffect* blurEffect =
        [UIBlurEffect effectWithStyle:UIBlurEffectStyleDark];
    UIVisualEffectView* toolbar =
        [[UIVisualEffectView alloc] initWithEffect:blurEffect];
    toolbar.translatesAutoresizingMaskIntoConstraints = NO;
    [self addSubview:toolbar];

    UIButton* leadingButton = [UIButton buttonWithType:UIButtonTypeSystem];
    leadingButton.translatesAutoresizingMaskIntoConstraints = NO;
    leadingButton.titleLabel.font =
        [UIFont preferredFontForTextStyle:UIFontTextStyleHeadline];
    leadingButton.titleLabel.adjustsFontForContentSizeCategory = YES;
    leadingButton.tintColor = [UIColor whiteColor];

    UILabel* segmentedControl = [[UILabel alloc] init];
    segmentedControl.translatesAutoresizingMaskIntoConstraints = NO;
    segmentedControl.backgroundColor = [UIColor whiteColor];
    segmentedControl.text = @"Segmented Control";
    segmentedControl.layer.cornerRadius = 11.0f;
    segmentedControl.layer.masksToBounds = YES;

    UIButton* trailingButton = [UIButton buttonWithType:UIButtonTypeSystem];
    trailingButton.translatesAutoresizingMaskIntoConstraints = NO;
    trailingButton.titleLabel.font =
        [UIFont preferredFontForTextStyle:UIFontTextStyleBody];
    trailingButton.titleLabel.adjustsFontForContentSizeCategory = YES;
    trailingButton.tintColor = [UIColor whiteColor];

    [toolbar.contentView addSubview:leadingButton];
    [toolbar.contentView addSubview:trailingButton];
    [toolbar.contentView addSubview:segmentedControl];
    _leadingButton = leadingButton;
    _trailingButton = trailingButton;
    _segmentedControl = segmentedControl;

    NSArray* constraints = @[
      [toolbar.topAnchor constraintEqualToAnchor:self.topAnchor],
      [toolbar.bottomAnchor constraintEqualToAnchor:self.bottomAnchor],
      [toolbar.leadingAnchor constraintEqualToAnchor:self.leadingAnchor],
      [toolbar.trailingAnchor constraintEqualToAnchor:self.trailingAnchor],
      [leadingButton.heightAnchor constraintEqualToConstant:kToolbarHeight],
      [leadingButton.leadingAnchor
          constraintEqualToAnchor:toolbar.layoutMarginsGuide.leadingAnchor],
      [leadingButton.bottomAnchor constraintEqualToAnchor:toolbar.bottomAnchor],
      [segmentedControl.heightAnchor
          constraintEqualToConstant:kSegmentedControlHeight],
      [segmentedControl.centerXAnchor
          constraintEqualToAnchor:toolbar.centerXAnchor],
      [segmentedControl.bottomAnchor
          constraintEqualToAnchor:toolbar.bottomAnchor
                         constant:-7.0f],
      [trailingButton.heightAnchor constraintEqualToConstant:kToolbarHeight],
      [trailingButton.trailingAnchor
          constraintEqualToAnchor:toolbar.layoutMarginsGuide.trailingAnchor],
      [trailingButton.bottomAnchor
          constraintEqualToAnchor:toolbar.bottomAnchor],
    ];
    [NSLayoutConstraint activateConstraints:constraints];
  }
  return self;
}

- (CGSize)intrinsicContentSize {
  return CGSizeMake(UIViewNoIntrinsicMetric, kToolbarHeight);
}

@end
