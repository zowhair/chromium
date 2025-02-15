// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/properties/shorthands/webkit_mask_repeat.h"

#include "core/css/CSSInitialValue.h"
#include "core/css/parser/CSSParserContext.h"
#include "core/css/parser/CSSPropertyParserHelpers.h"
#include "core/css/properties/CSSParsingUtils.h"
#include "core/css/properties/ComputedStyleUtils.h"
#include "core/style/ComputedStyle.h"
#include "core/style_property_shorthand.h"

namespace blink {
namespace CSSShorthand {

bool WebkitMaskRepeat::ParseShorthand(
    bool important,
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    const CSSParserLocalContext& local_context,
    HeapVector<CSSPropertyValue, 256>& properties) const {
  CSSValue* result_x = nullptr;
  CSSValue* result_y = nullptr;
  bool implicit = false;
  if (!CSSParsingUtils::ConsumeRepeatStyle(range, result_x, result_y,
                                           implicit) ||
      !range.AtEnd())
    return false;

  CSSPropertyParserHelpers::AddProperty(
      CSSPropertyWebkitMaskRepeatX, CSSPropertyWebkitMaskRepeat, *result_x,
      important,
      implicit ? CSSPropertyParserHelpers::IsImplicitProperty::kImplicit
               : CSSPropertyParserHelpers::IsImplicitProperty::kNotImplicit,
      properties);
  CSSPropertyParserHelpers::AddProperty(
      CSSPropertyWebkitMaskRepeatY, CSSPropertyWebkitMaskRepeat, *result_y,
      important,
      implicit ? CSSPropertyParserHelpers::IsImplicitProperty::kImplicit
               : CSSPropertyParserHelpers::IsImplicitProperty::kNotImplicit,
      properties);

  return true;
}

const CSSValue* WebkitMaskRepeat::CSSValueFromComputedStyleInternal(
    const ComputedStyle& style,
    const SVGComputedStyle&,
    const LayoutObject*,
    Node*,
    bool allow_visited_style) const {
  return ComputedStyleUtils::BackgroundRepeatOrWebkitMaskRepeat(
      &style.MaskLayers());
}

}  // namespace CSSShorthand
}  // namespace blink
