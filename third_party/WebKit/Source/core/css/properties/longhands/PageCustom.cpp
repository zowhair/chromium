// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/properties/longhands/page.h"

#include "core/css/parser/CSSPropertyParserHelpers.h"

namespace blink {
namespace CSSLonghand {

const CSSValue* Page::ParseSingleValue(CSSParserTokenRange& range,
                                       const CSSParserContext& context,
                                       const CSSParserLocalContext&) const {
  if (range.Peek().Id() == CSSValueAuto)
    return CSSPropertyParserHelpers::ConsumeIdent(range);
  return CSSPropertyParserHelpers::ConsumeCustomIdent(range);
}

}  // namespace CSSLonghand
}  // namespace blink
