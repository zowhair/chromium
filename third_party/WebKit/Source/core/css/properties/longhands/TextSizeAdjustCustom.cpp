// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/properties/longhands/text_size_adjust.h"

#include "core/css/parser/CSSPropertyParserHelpers.h"
#include "core/css_value_keywords.h"
#include "core/style/ComputedStyle.h"

namespace blink {
namespace CSSLonghand {

const CSSValue* TextSizeAdjust::ParseSingleValue(
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    const CSSParserLocalContext&) const {
  if (range.Peek().Id() == CSSValueAuto)
    return CSSPropertyParserHelpers::ConsumeIdent(range);
  if (range.Peek().Id() == CSSValueNone)
    return CSSPropertyParserHelpers::ConsumeIdent(range);
  return CSSPropertyParserHelpers::ConsumePercent(range,
                                                  kValueRangeNonNegative);
}

const CSSValue* TextSizeAdjust::CSSValueFromComputedStyleInternal(
    const ComputedStyle& style,
    const SVGComputedStyle&,
    const LayoutObject*,
    Node* styled_node,
    bool allow_visited_style) const {
  if (style.GetTextSizeAdjust().IsAuto())
    return CSSIdentifierValue::Create(CSSValueAuto);
  return CSSPrimitiveValue::Create(style.GetTextSizeAdjust().Multiplier() * 100,
                                   CSSPrimitiveValue::UnitType::kPercentage);
}

}  // namespace CSSLonghand
}  // namespace blink
