// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/CSSComputedStyleDeclaration.h"

#include "core/dom/ShadowRoot.h"
#include "core/html/HTMLElement.h"
#include "core/testing/PageTestBase.h"

namespace blink {

class CSSComputedStyleDeclarationTest : public PageTestBase {};

TEST_F(CSSComputedStyleDeclarationTest, CleanAncestorsNoRecalc) {
  GetDocument().body()->SetInnerHTMLFromString(R"HTML(
    <div id=dirty></div>
    <div>
      <div id=target style='color:green'></div>
    </div>
  )HTML");
  GetDocument().View()->UpdateAllLifecyclePhases();
  EXPECT_FALSE(GetDocument().NeedsLayoutTreeUpdate());

  GetDocument().getElementById("dirty")->setAttribute("style", "color:pink");
  EXPECT_TRUE(GetDocument().NeedsLayoutTreeUpdate());

  Element* target = GetDocument().getElementById("target");
  CSSComputedStyleDeclaration* computed =
      CSSComputedStyleDeclaration::Create(target);

  EXPECT_STREQ("rgb(0, 128, 0)",
               computed->GetPropertyValue(CSSPropertyColor).Utf8().data());
  EXPECT_TRUE(GetDocument().NeedsLayoutTreeUpdate());
}

TEST_F(CSSComputedStyleDeclarationTest, CleanShadowAncestorsNoRecalc) {
  GetDocument().body()->SetInnerHTMLFromString(R"HTML(
    <div id=dirty></div>
    <div id=host></div>
  )HTML");

  Element* host = GetDocument().getElementById("host");

  ShadowRoot& shadow_root =
      host->AttachShadowRootInternal(ShadowRootType::kOpen);
  shadow_root.SetInnerHTMLFromString(R"HTML(
    <div id=target style='color:green'></div>
  )HTML");

  GetDocument().View()->UpdateAllLifecyclePhases();
  EXPECT_FALSE(GetDocument().NeedsLayoutTreeUpdate());

  GetDocument().getElementById("dirty")->setAttribute("style", "color:pink");
  EXPECT_TRUE(GetDocument().NeedsLayoutTreeUpdate());

  Element* target = shadow_root.getElementById("target");
  CSSComputedStyleDeclaration* computed =
      CSSComputedStyleDeclaration::Create(target);

  EXPECT_STREQ("rgb(0, 128, 0)",
               computed->GetPropertyValue(CSSPropertyColor).Utf8().data());
  EXPECT_EQ(RuntimeEnabledFeatures::SlotInFlatTreeEnabled(),
            GetDocument().NeedsLayoutTreeUpdate());
}

}  // namespace blink
