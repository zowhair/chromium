// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/geometry/LayoutRect.h"

#include "platform/wtf/text/WTFString.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

TEST(LayoutRectTest, ToString) {
  LayoutRect empty_rect = LayoutRect();
  EXPECT_EQ("0,0 0x0", empty_rect.ToString());

  LayoutRect rect(1, 2, 3, 4);
  EXPECT_EQ("1,2 3x4", rect.ToString());

  LayoutRect granular_rect(LayoutUnit(1.6f), LayoutUnit(2.7f), LayoutUnit(3.8f),
                           LayoutUnit(4.9f));
  EXPECT_EQ("1.59375,2.6875 3.79688x4.89063", granular_rect.ToString());
}

TEST(LayoutRectTest, InclusiveIntersect) {
  LayoutRect rect(11, 12, 0, 0);
  EXPECT_TRUE(rect.InclusiveIntersect(LayoutRect(11, 12, 13, 14)));
  EXPECT_EQ(rect, LayoutRect(11, 12, 0, 0));

  rect = LayoutRect(11, 12, 13, 14);
  EXPECT_TRUE(rect.InclusiveIntersect(LayoutRect(24, 8, 0, 7)));
  EXPECT_EQ(rect, LayoutRect(24, 12, 0, 3));

  rect = LayoutRect(11, 12, 13, 14);
  EXPECT_TRUE(rect.InclusiveIntersect(LayoutRect(9, 15, 4, 0)));
  EXPECT_EQ(rect, LayoutRect(11, 15, 2, 0));

  rect = LayoutRect(11, 12, 0, 14);
  EXPECT_FALSE(rect.InclusiveIntersect(LayoutRect(12, 13, 15, 16)));
  EXPECT_EQ(rect, LayoutRect());
}

TEST(LayoutRectTest, IntersectsInclusively) {
  LayoutRect a(11, 12, 0, 0);
  LayoutRect b(11, 12, 13, 14);
  // An empty rect can have inclusive intersection.
  EXPECT_TRUE(a.IntersectsInclusively(b));
  EXPECT_TRUE(b.IntersectsInclusively(a));

  a = LayoutRect(11, 12, 13, 14);
  b = LayoutRect(24, 8, 0, 7);
  // Intersecting left side is sufficient for inclusive intersection.
  EXPECT_TRUE(a.IntersectsInclusively(b));
  EXPECT_TRUE(b.IntersectsInclusively(a));

  a = LayoutRect(11, 12, 13, 14);
  b = LayoutRect(0, 26, 13, 8);
  // Intersecting bottom side is sufficient for inclusive intersection.
  EXPECT_TRUE(a.IntersectsInclusively(b));
  EXPECT_TRUE(b.IntersectsInclusively(a));

  a = LayoutRect(11, 12, 0, 0);
  b = LayoutRect(11, 12, 0, 0);
  // Two empty rects can intersect inclusively.
  EXPECT_TRUE(a.IntersectsInclusively(b));
  EXPECT_TRUE(b.IntersectsInclusively(a));

  a = LayoutRect(10, 10, 10, 10);
  b = LayoutRect(20, 20, 10, 10);
  // Two rects can intersect inclusively at a single point.
  EXPECT_TRUE(a.IntersectsInclusively(b));
  EXPECT_TRUE(b.IntersectsInclusively(a));

  a = LayoutRect(11, 12, 0, 0);
  b = LayoutRect(20, 21, 0, 0);
  // Two empty rects that do not touch do not intersect.
  EXPECT_FALSE(a.IntersectsInclusively(b));
  EXPECT_FALSE(b.IntersectsInclusively(a));

  a = LayoutRect(11, 12, 5, 5);
  b = LayoutRect(20, 21, 0, 0);
  // A rect that does not touch a point does not intersect.
  EXPECT_FALSE(a.IntersectsInclusively(b));
  EXPECT_FALSE(b.IntersectsInclusively(a));
}

TEST(LayoutRectTest, EnclosingIntRect) {
  LayoutUnit small;
  small.SetRawValue(1);
  LayoutRect small_dimensions_rect(LayoutUnit(42.5f), LayoutUnit(84.5f), small,
                                   small);
  EXPECT_EQ(IntRect(42, 84, 1, 1), EnclosingIntRect(small_dimensions_rect));

  LayoutRect integral_rect(IntRect(100, 150, 200, 350));
  EXPECT_EQ(IntRect(100, 150, 200, 350), EnclosingIntRect(integral_rect));

  LayoutRect fractional_pos_rect(LayoutUnit(100.6f), LayoutUnit(150.8f),
                                 LayoutUnit(200), LayoutUnit(350));
  EXPECT_EQ(IntRect(100, 150, 201, 351), EnclosingIntRect(fractional_pos_rect));

  LayoutRect fractional_dimensions_rect(LayoutUnit(100), LayoutUnit(150),
                                        LayoutUnit(200.6f), LayoutUnit(350.4f));
  EXPECT_EQ(IntRect(100, 150, 201, 351),
            EnclosingIntRect(fractional_dimensions_rect));

  LayoutRect fractional_both_rect1(LayoutUnit(100.6f), LayoutUnit(150.8f),
                                   LayoutUnit(200.4f), LayoutUnit(350.2f));
  EXPECT_EQ(IntRect(100, 150, 201, 351),
            EnclosingIntRect(fractional_both_rect1));

  LayoutRect fractional_both_rect2(LayoutUnit(100.5f), LayoutUnit(150.7f),
                                   LayoutUnit(200.3f), LayoutUnit(350.3f));
  EXPECT_EQ(IntRect(100, 150, 201, 351),
            EnclosingIntRect(fractional_both_rect2));

  LayoutRect fractional_both_rect3(LayoutUnit(100.3f), LayoutUnit(150.2f),
                                   LayoutUnit(200.8f), LayoutUnit(350.9f));
  EXPECT_EQ(IntRect(100, 150, 202, 352),
            EnclosingIntRect(fractional_both_rect3));

  LayoutRect fractional_negpos_rect1(LayoutUnit(-100.4f), LayoutUnit(-150.8f),
                                     LayoutUnit(200), LayoutUnit(350));
  EXPECT_EQ(IntRect(-101, -151, 201, 351),
            EnclosingIntRect(fractional_negpos_rect1));

  LayoutRect fractional_negpos_rect2(LayoutUnit(-100.5f), LayoutUnit(-150.7f),
                                     LayoutUnit(199.4f), LayoutUnit(350.3f));
  EXPECT_EQ(IntRect(-101, -151, 200, 351),
            EnclosingIntRect(fractional_negpos_rect2));

  LayoutRect fractional_negpos_rect3(LayoutUnit(-100.3f), LayoutUnit(-150.2f),
                                     LayoutUnit(199.6f), LayoutUnit(350.3f));
  EXPECT_EQ(IntRect(-101, -151, 201, 352),
            EnclosingIntRect(fractional_negpos_rect3));
}

TEST(LayoutRectTest, EnclosedIntRect) {
  LayoutUnit small;
  small.SetRawValue(1);
  LayoutRect small_dimensions_rect(LayoutUnit(42.5f), LayoutUnit(84.5f), small,
                                   small);

  LayoutRect integral_rect(IntRect(100, 150, 200, 350));
  EXPECT_EQ(IntRect(100, 150, 200, 350), EnclosedIntRect(integral_rect));

  LayoutRect fractional_pos_rect(LayoutUnit(100.6f), LayoutUnit(150.8f),
                                 LayoutUnit(200), LayoutUnit(350));
  EXPECT_EQ(IntRect(101, 151, 199, 349), EnclosedIntRect(fractional_pos_rect));

  LayoutRect fractional_dimensions_rect(LayoutUnit(100), LayoutUnit(150),
                                        LayoutUnit(200.6f), LayoutUnit(350.4f));
  EXPECT_EQ(IntRect(100, 150, 200, 350),
            EnclosedIntRect(fractional_dimensions_rect));

  LayoutRect fractional_both_rect1(LayoutUnit(100.6f), LayoutUnit(150.8f),
                                   LayoutUnit(200.4f), LayoutUnit(350.2f));
  EXPECT_EQ(IntRect(101, 151, 199, 349),
            EnclosedIntRect(fractional_both_rect1));

  LayoutRect fractional_both_rect2(LayoutUnit(100.5f), LayoutUnit(150.7f),
                                   LayoutUnit(200.3f), LayoutUnit(350.3f));
  EXPECT_EQ(IntRect(101, 151, 199, 349),
            EnclosedIntRect(fractional_both_rect2));

  LayoutRect fractional_both_rect3(LayoutUnit(100.3f), LayoutUnit(150.2f),
                                   LayoutUnit(200.6f), LayoutUnit(350.3f));
  EXPECT_EQ(IntRect(101, 151, 199, 349),
            EnclosedIntRect(fractional_both_rect3));

  LayoutRect fractional_negpos_rect1(LayoutUnit(-100.4f), LayoutUnit(-150.8f),
                                     LayoutUnit(200), LayoutUnit(350));
  EXPECT_EQ(IntRect(-100, -150, 199, 349),
            EnclosedIntRect(fractional_negpos_rect1));

  LayoutRect fractional_negpos_rect2(LayoutUnit(-100.5f), LayoutUnit(-150.7f),
                                     LayoutUnit(199.4f), LayoutUnit(350.3f));
  EXPECT_EQ(IntRect(-100, -150, 198, 349),
            EnclosedIntRect(fractional_negpos_rect2));

  LayoutRect fractional_negpos_rect3(LayoutUnit(-100.3f), LayoutUnit(-150.2f),
                                     LayoutUnit(199.6f), LayoutUnit(350.3f));
  EXPECT_EQ(IntRect(-100, -150, 199, 350),
            EnclosedIntRect(fractional_negpos_rect3));
}

}  // namespace blink
