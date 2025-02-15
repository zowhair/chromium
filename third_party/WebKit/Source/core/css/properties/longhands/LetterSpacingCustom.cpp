// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/properties/longhands/letter_spacing.h"

#include "core/css/ZoomAdjustedPixelValue.h"
#include "core/css/properties/CSSParsingUtils.h"
#include "core/style/ComputedStyle.h"

namespace blink {
namespace CSSLonghand {

const CSSValue* LetterSpacing::ParseSingleValue(
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    const CSSParserLocalContext&) const {
  return CSSParsingUtils::ParseSpacing(range, context);
}

const CSSValue* LetterSpacing::CSSValueFromComputedStyleInternal(
    const ComputedStyle& style,
    const SVGComputedStyle&,
    const LayoutObject*,
    Node* styled_node,
    bool allow_visited_style) const {
  if (!style.LetterSpacing())
    return CSSIdentifierValue::Create(CSSValueNormal);
  return ZoomAdjustedPixelValue(style.LetterSpacing(), style);
}

}  // namespace CSSLonghand
}  // namespace blink
