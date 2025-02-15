// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/StyleChangeReason.h"

#include "platform/instrumentation/tracing/TraceEvent.h"
#include "platform/wtf/StaticConstructors.h"

namespace blink {

namespace StyleChangeReason {
const char kActiveStylesheetsUpdate[] = "ActiveStylesheetsUpdate";
const char kAnimation[] = "Animation";
const char kAttribute[] = "Attribute";
const char kCleanupPlaceholderStyles[] = "CleanupPlaceholderStyles";
const char kControlValue[] = "ControlValue";
const char kControl[] = "Control";
const char kDeclarativeContent[] = "Extension declarativeContent.css";
const char kDesignMode[] = "DesignMode";
const char kFontSizeChange[] = "FontSizeChange";
const char kFonts[] = "Fonts";
const char kFullScreen[] = "FullScreen";
const char kInheritedStyleChangeFromParentFrame[] =
    "InheritedStyleChangeFromParentFrame";
const char kInline[] = "Inline";
const char kInlineCSSStyleMutated[] =
    "Inline CSS style declaration was mutated";
const char kInspector[] = "Inspector";
const char kLanguage[] = "Language";
const char kLinkColorChange[] = "LinkColorChange";
const char kPlatformColorChange[] = "PlatformColorChange";
const char kPropagateInheritChangeToDistributedNodes[] =
    "PropagateInheritChangeToDistributedNodes";
const char kPropertyRegistration[] = "PropertyRegistration";
const char kPropertyUnregistration[] = "PropertyUnregistration";
const char kPseudoClass[] = "PseudoClass";
const char kSVGContainerSizeChange[] = "SVGContainerSizeChange";
const char kSVGCursor[] = "SVGCursor";
const char kSettings[] = "Settings";
const char kShadow[] = "Shadow";
const char kStyleInvalidator[] = "StyleInvalidator";
const char kStyleSheetChange[] = "StyleSheetChange";
const char kViewportUnits[] = "ViewportUnits";
const char kVisitedLink[] = "VisitedLink";
const char kVisuallyOrdered[] = "VisuallyOrdered";
const char kWritingModeChange[] = "WritingModeChange";
const char kZoom[] = "Zoom";
}  // namespace StyleChangeReason

namespace StyleChangeExtraData {
DEFINE_GLOBAL(AtomicString, g_active);
DEFINE_GLOBAL(AtomicString, g_disabled);
DEFINE_GLOBAL(AtomicString, g_drag);
DEFINE_GLOBAL(AtomicString, g_focus);
DEFINE_GLOBAL(AtomicString, g_focus_visible);
DEFINE_GLOBAL(AtomicString, g_focus_within);
DEFINE_GLOBAL(AtomicString, g_hover);
DEFINE_GLOBAL(AtomicString, g_past);
DEFINE_GLOBAL(AtomicString, g_unresolved);

void Init() {
  DCHECK(IsMainThread());

  new (NotNull, (void*)&g_active) AtomicString(":active");
  new (NotNull, (void*)&g_disabled) AtomicString(":disabled");
  new (NotNull, (void*)&g_drag) AtomicString(":-webkit-drag");
  new (NotNull, (void*)&g_focus) AtomicString(":focus");
  new (NotNull, (void*)&g_focus_visible) AtomicString(":focus-visible");
  new (NotNull, (void*)&g_focus_within) AtomicString(":focus-within");
  new (NotNull, (void*)&g_hover) AtomicString(":hover");
  new (NotNull, (void*)&g_past) AtomicString(":past");
  new (NotNull, (void*)&g_unresolved) AtomicString(":unresolved");
}

}  // namespace StyleChangeExtraData

}  // namespace blink
