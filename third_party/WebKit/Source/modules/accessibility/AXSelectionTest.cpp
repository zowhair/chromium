// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/dom/Node.h"
#include "core/editing/Position.h"
#include "core/editing/SelectionTemplate.h"
#include "modules/accessibility/AXObject.h"
#include "modules/accessibility/AXPosition.h"
#include "modules/accessibility/AXSelection.h"
#include "modules/accessibility/testing/AccessibilityTest.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

//
// Basic tests.
//

TEST_F(AccessibilityTest, SetSelectionInText) {
  SetBodyInnerHTML(R"HTML(<p id='paragraph'>Hello</p>)HTML");
  const Node* text = GetElementById("paragraph")->firstChild();
  ASSERT_NE(nullptr, text);
  const AXObject* ax_static_text =
      *(GetAXObjectByElementId("paragraph")->Children().begin());
  ASSERT_NE(nullptr, ax_static_text);
  const auto ax_base =
      AXPosition::CreatePositionInTextObject(*ax_static_text, 3);
  const auto ax_extent = AXPosition::CreatePositionAfterObject(*ax_static_text);

  AXSelection::Builder builder;
  const AXSelection ax_selection =
      builder.SetBase(ax_base).SetExtent(ax_extent).Build();
  const SelectionInDOMTree dom_selection = ax_selection.AsSelection();
  EXPECT_EQ(text, dom_selection.Base().AnchorNode());
  EXPECT_EQ(3, dom_selection.Base().OffsetInContainerNode());
  EXPECT_EQ(text, dom_selection.Extent().AnchorNode());
  EXPECT_EQ(5, dom_selection.Extent().OffsetInContainerNode());
}

TEST_F(AccessibilityTest, SetSelectionInTextWithWhiteSpace) {
  SetBodyInnerHTML(R"HTML(<p id='paragraph'>     Hello</p>)HTML");
  const Node* text = GetElementById("paragraph")->firstChild();
  ASSERT_NE(nullptr, text);
  const AXObject* ax_static_text =
      *(GetAXObjectByElementId("paragraph")->Children().begin());
  ASSERT_NE(nullptr, ax_static_text);
  const auto ax_base =
      AXPosition::CreatePositionInTextObject(*ax_static_text, 3);
  const auto ax_extent = AXPosition::CreatePositionAfterObject(*ax_static_text);

  AXSelection::Builder builder;
  const AXSelection ax_selection =
      builder.SetBase(ax_base).SetExtent(ax_extent).Build();
  const SelectionInDOMTree dom_selection = ax_selection.AsSelection();
  EXPECT_EQ(text, dom_selection.Base().AnchorNode());
  EXPECT_EQ(8, dom_selection.Base().OffsetInContainerNode());
  EXPECT_EQ(text, dom_selection.Extent().AnchorNode());
  EXPECT_EQ(10, dom_selection.Extent().OffsetInContainerNode());
}

//
// Get selection tests.
// Retrieving a selection with endpoints which have no corresponding objects in
// the accessibility tree, e.g. which are aria-hidden, should shring
// |AXSelection| to valid endpoints.
//

//
// Set selection tests.
// Setting the selection from an |AXSelection| that has endpoints which are not
// present in the layout tree should shring the selection to visible endpoints.
//

}  // namespace blink
