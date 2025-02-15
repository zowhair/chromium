// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/properties/longhands/image_orientation.h"

#include "core/css/CSSPrimitiveValue.h"
#include "core/css/parser/CSSPropertyParserHelpers.h"
#include "core/css_value_keywords.h"
#include "core/style/ComputedStyle.h"
#include "platform/runtime_enabled_features.h"

namespace blink {
namespace CSSLonghand {

const CSSValue* ImageOrientation::ParseSingleValue(
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    const CSSParserLocalContext&) const {
  DCHECK(RuntimeEnabledFeatures::ImageOrientationEnabled());
  if (range.Peek().Id() == CSSValueFromImage)
    return CSSPropertyParserHelpers::ConsumeIdent(range);
  if (range.Peek().GetType() != kNumberToken) {
    CSSPrimitiveValue* angle = CSSPropertyParserHelpers::ConsumeAngle(
        range, &context, WTF::Optional<WebFeature>());
    if (angle && angle->GetDoubleValue() == 0)
      return angle;
  }
  return nullptr;
}

const CSSValue* ImageOrientation::CSSValueFromComputedStyleInternal(
    const ComputedStyle& style,
    const SVGComputedStyle&,
    const LayoutObject*,
    Node* styled_node,
    bool allow_visited_style) const {
  if (style.RespectImageOrientation() == kRespectImageOrientation)
    return CSSIdentifierValue::Create(CSSValueFromImage);
  return CSSPrimitiveValue::Create(0, CSSPrimitiveValue::UnitType::kDegrees);
}

}  // namespace CSSLonghand
}  // namespace blink
