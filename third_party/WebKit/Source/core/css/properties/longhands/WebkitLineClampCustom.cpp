// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/properties/longhands/webkit_line_clamp.h"

#include "core/css/parser/CSSPropertyParserHelpers.h"
#include "core/style/ComputedStyle.h"

namespace blink {
namespace CSSLonghand {

const CSSValue* WebkitLineClamp::ParseSingleValue(
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    const CSSParserLocalContext&) const {
  if (range.Peek().GetType() != kNumberToken)
    return nullptr;

  // When specifying number of lines, don't allow 0 as a valid value.
  return CSSPropertyParserHelpers::ConsumePositiveInteger(range);
}

const CSSValue* WebkitLineClamp::CSSValueFromComputedStyleInternal(
    const ComputedStyle& style,
    const SVGComputedStyle&,
    const LayoutObject*,
    Node* styled_node,
    bool allow_visited_style) const {
  if (!style.HasLineClamp())
    return CSSIdentifierValue::Create(CSSValueNone);
  return CSSPrimitiveValue::Create(style.LineClamp(),
                                   CSSPrimitiveValue::UnitType::kNumber);
}

}  // namespace CSSLonghand
}  // namespace blink
