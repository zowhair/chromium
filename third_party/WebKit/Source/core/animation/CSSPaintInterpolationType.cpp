// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/animation/CSSPaintInterpolationType.h"

#include <memory>
#include <utility>

#include "base/memory/ptr_util.h"
#include "core/animation/CSSColorInterpolationType.h"
#include "core/css/StyleColor.h"
#include "core/css/resolver/StyleResolverState.h"
#include "core/css_property_names.h"
#include "core/style/ComputedStyle.h"

namespace blink {

namespace {

static bool GetColorFromPaint(const SVGPaint& paint, StyleColor& result) {
  if (!paint.IsColor())
    return false;
  if (paint.HasCurrentColor())
    result = StyleColor::CurrentColor();
  else
    result = paint.GetColor();
  return true;
}

bool GetColor(const CSSProperty& property,
              const ComputedStyle& style,
              StyleColor& result) {
  switch (property.PropertyID()) {
    case CSSPropertyFill:
      return GetColorFromPaint(style.SvgStyle().FillPaint(), result);
    case CSSPropertyStroke:
      return GetColorFromPaint(style.SvgStyle().StrokePaint(), result);
    default:
      NOTREACHED();
      return false;
  }
}

}  // namespace

InterpolationValue CSSPaintInterpolationType::MaybeConvertNeutral(
    const InterpolationValue&,
    ConversionCheckers&) const {
  return InterpolationValue(
      CSSColorInterpolationType::CreateInterpolableColor(Color::kTransparent));
}

InterpolationValue CSSPaintInterpolationType::MaybeConvertInitial(
    const StyleResolverState&,
    ConversionCheckers& conversion_checkers) const {
  StyleColor initial_color;
  if (!GetColor(CssProperty(), ComputedStyle::InitialStyle(), initial_color))
    return nullptr;
  return InterpolationValue(
      CSSColorInterpolationType::CreateInterpolableColor(initial_color));
}

class InheritedPaintChecker
    : public CSSInterpolationType::CSSConversionChecker {
 public:
  static std::unique_ptr<InheritedPaintChecker> Create(
      const CSSProperty& property,
      const StyleColor& color) {
    return base::WrapUnique(new InheritedPaintChecker(property, color));
  }
  static std::unique_ptr<InheritedPaintChecker> Create(
      const CSSProperty& property) {
    return base::WrapUnique(new InheritedPaintChecker(property));
  }

 private:
  InheritedPaintChecker(const CSSProperty& property)
      : property_(property), valid_color_(false) {}
  InheritedPaintChecker(const CSSProperty& property, const StyleColor& color)
      : property_(property), valid_color_(true), color_(color) {}

  bool IsValid(const StyleResolverState& state,
               const InterpolationValue& underlying) const final {
    StyleColor parent_color;
    if (!GetColor(property_, *state.ParentStyle(), parent_color))
      return !valid_color_;
    return valid_color_ && parent_color == color_;
  }

  const CSSProperty& property_;
  const bool valid_color_;
  const StyleColor color_;
};

InterpolationValue CSSPaintInterpolationType::MaybeConvertInherit(
    const StyleResolverState& state,
    ConversionCheckers& conversion_checkers) const {
  if (!state.ParentStyle())
    return nullptr;
  StyleColor parent_color;
  if (!GetColor(CssProperty(), *state.ParentStyle(), parent_color)) {
    conversion_checkers.push_back(InheritedPaintChecker::Create(CssProperty()));
    return nullptr;
  }
  conversion_checkers.push_back(
      InheritedPaintChecker::Create(CssProperty(), parent_color));
  return InterpolationValue(
      CSSColorInterpolationType::CreateInterpolableColor(parent_color));
}

InterpolationValue CSSPaintInterpolationType::MaybeConvertValue(
    const CSSValue& value,
    const StyleResolverState*,
    ConversionCheckers&) const {
  std::unique_ptr<InterpolableValue> interpolable_color =
      CSSColorInterpolationType::MaybeCreateInterpolableColor(value);
  if (!interpolable_color)
    return nullptr;
  return InterpolationValue(std::move(interpolable_color));
}

InterpolationValue
CSSPaintInterpolationType::MaybeConvertStandardPropertyUnderlyingValue(
    const ComputedStyle& style) const {
  // TODO(alancutter): Support capturing and animating with the visited paint
  // color.
  StyleColor underlying_color;
  if (!GetColor(CssProperty(), style, underlying_color))
    return nullptr;
  return InterpolationValue(
      CSSColorInterpolationType::CreateInterpolableColor(underlying_color));
}

void CSSPaintInterpolationType::ApplyStandardPropertyValue(
    const InterpolableValue& interpolable_color,
    const NonInterpolableValue*,
    StyleResolverState& state) const {
  Color color = CSSColorInterpolationType::ResolveInterpolableColor(
      interpolable_color, state);
  SVGComputedStyle& mutable_svg_style = state.Style()->AccessSVGStyle();
  switch (CssProperty().PropertyID()) {
    case CSSPropertyFill:
      mutable_svg_style.SetFillPaint(SVGPaint(color));
      mutable_svg_style.SetVisitedLinkFillPaint(SVGPaint(color));
      break;
    case CSSPropertyStroke:
      mutable_svg_style.SetStrokePaint(SVGPaint(color));
      mutable_svg_style.SetVisitedLinkStrokePaint(SVGPaint(color));
      break;
    default:
      NOTREACHED();
  }
}

}  // namespace blink
