// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/properties/longhands/scroll_margin_top.h"

#include "core/css/ZoomAdjustedPixelValue.h"
#include "core/css/parser/CSSParserContext.h"
#include "core/css/parser/CSSPropertyParserHelpers.h"
#include "core/css/properties/ComputedStyleUtils.h"

namespace blink {
namespace CSSLonghand {

const CSSValue* ScrollMarginTop::ParseSingleValue(
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    const CSSParserLocalContext&) const {
  return ConsumeLength(range, context.Mode(), kValueRangeAll,
                       CSSPropertyParserHelpers::UnitlessQuirk::kAllow);
}

const CSSValue* ScrollMarginTop::CSSValueFromComputedStyleInternal(
    const ComputedStyle& style,
    const SVGComputedStyle&,
    const LayoutObject*,
    Node*,
    bool allow_visited_style) const {
  return ZoomAdjustedPixelValue(style.ScrollMarginTop(), style);
}

}  // namespace CSSLonghand
}  // namespace blink
