// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/properties/longhands/border_bottom_color.h"

#include "core/css/CSSColorValue.h"
#include "core/css/properties/CSSParsingUtils.h"
#include "core/css/properties/ComputedStyleUtils.h"
#include "core/style/ComputedStyle.h"

namespace blink {

class CSSParserContext;
class CSSParserLocalContext;
class CSSParserTokenRange;

namespace CSSLonghand {

const CSSValue* BorderBottomColor::ParseSingleValue(
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    const CSSParserLocalContext& local_context) const {
  return CSSParsingUtils::ConsumeBorderColorSide(range, context, local_context);
}

const blink::Color BorderBottomColor::ColorIncludingFallback(
    bool visited_link,
    const ComputedStyle& style) const {
  StyleColor result = visited_link ? style.VisitedLinkBorderBottomColor()
                                   : style.BorderBottomColor();
  EBorderStyle border_style = style.BorderBottomStyle();
  return ComputedStyleUtils::BorderSideColor(style, result, border_style,
                                             visited_link);
}

const CSSValue* BorderBottomColor::CSSValueFromComputedStyleInternal(
    const ComputedStyle& style,
    const SVGComputedStyle&,
    const LayoutObject*,
    Node*,
    bool allow_visited_style) const {
  return allow_visited_style ? cssvalue::CSSColorValue::Create(
                                   style.VisitedDependentColor(*this).Rgb())
                             : ComputedStyleUtils::CurrentColorOrValidColor(
                                   style, style.BorderBottomColor());
}

}  // namespace CSSLonghand
}  // namespace blink
