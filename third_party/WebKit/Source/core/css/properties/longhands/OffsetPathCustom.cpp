// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/properties/longhands/offset_path.h"

#include "core/css/BasicShapeFunctions.h"
#include "core/css/properties/CSSParsingUtils.h"

namespace blink {
namespace CSSLonghand {

const CSSValue* OffsetPath::ParseSingleValue(
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    const CSSParserLocalContext&) const {
  return CSSParsingUtils::ConsumeOffsetPath(range, context);
}

const CSSValue* OffsetPath::CSSValueFromComputedStyleInternal(
    const ComputedStyle& style,
    const SVGComputedStyle&,
    const LayoutObject*,
    Node*,
    bool allow_visited_style) const {
  if (const BasicShape* style_motion_path = style.OffsetPath())
    return ValueForBasicShape(style, style_motion_path);
  return CSSIdentifierValue::Create(CSSValueNone);
}

}  // namespace CSSLonghand
}  // namespace blink
