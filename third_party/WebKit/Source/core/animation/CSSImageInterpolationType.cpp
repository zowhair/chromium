// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/animation/CSSImageInterpolationType.h"

#include <memory>

#include "base/memory/ptr_util.h"
#include "core/css/CSSCrossfadeValue.h"
#include "core/css/CSSPrimitiveValue.h"
#include "core/css/resolver/StyleResolverState.h"
#include "core/css_property_names.h"
#include "core/style/ComputedStyle.h"
#include "core/style/StyleImage.h"

namespace blink {

namespace {
const StyleImage* GetStyleImage(const CSSProperty& property,
                                const ComputedStyle& style) {
  switch (property.PropertyID()) {
    case CSSPropertyBorderImageSource:
      return style.BorderImageSource();
    case CSSPropertyListStyleImage:
      return style.ListStyleImage();
    case CSSPropertyWebkitMaskBoxImageSource:
      return style.MaskBoxImageSource();
    default:
      NOTREACHED();
      return nullptr;
  }
}
}  // namespace

class CSSImageNonInterpolableValue : public NonInterpolableValue {
 public:
  ~CSSImageNonInterpolableValue() final = default;

  static scoped_refptr<CSSImageNonInterpolableValue> Create(CSSValue* start,
                                                            CSSValue* end) {
    return base::AdoptRef(new CSSImageNonInterpolableValue(start, end));
  }

  bool IsSingle() const { return is_single_; }
  bool Equals(const CSSImageNonInterpolableValue& other) const {
    return DataEquivalent(start_, other.start_) &&
           DataEquivalent(end_, other.end_);
  }

  static scoped_refptr<CSSImageNonInterpolableValue> Merge(
      scoped_refptr<NonInterpolableValue> start,
      scoped_refptr<NonInterpolableValue> end);

  CSSValue* Crossfade(double progress) const {
    if (is_single_ || progress <= 0)
      return start_;
    if (progress >= 1)
      return end_;
    return cssvalue::CSSCrossfadeValue::Create(
        start_, end_,
        CSSPrimitiveValue::Create(progress,
                                  CSSPrimitiveValue::UnitType::kNumber));
  }

  DECLARE_NON_INTERPOLABLE_VALUE_TYPE();

 private:
  CSSImageNonInterpolableValue(CSSValue* start, CSSValue* end)
      : start_(start), end_(end), is_single_(start_ == end_) {
    DCHECK(start_);
    DCHECK(end_);
  }

  Persistent<CSSValue> start_;
  Persistent<CSSValue> end_;
  const bool is_single_;
};

DEFINE_NON_INTERPOLABLE_VALUE_TYPE(CSSImageNonInterpolableValue);
DEFINE_NON_INTERPOLABLE_VALUE_TYPE_CASTS(CSSImageNonInterpolableValue);

scoped_refptr<CSSImageNonInterpolableValue> CSSImageNonInterpolableValue::Merge(
    scoped_refptr<NonInterpolableValue> start,
    scoped_refptr<NonInterpolableValue> end) {
  const CSSImageNonInterpolableValue& start_image_pair =
      ToCSSImageNonInterpolableValue(*start);
  const CSSImageNonInterpolableValue& end_image_pair =
      ToCSSImageNonInterpolableValue(*end);
  DCHECK(start_image_pair.is_single_);
  DCHECK(end_image_pair.is_single_);
  return Create(start_image_pair.start_, end_image_pair.end_);
}

InterpolationValue CSSImageInterpolationType::MaybeConvertStyleImage(
    const StyleImage& style_image,
    bool accept_gradients) {
  return MaybeConvertCSSValue(*style_image.CssValue(), accept_gradients);
}

InterpolationValue CSSImageInterpolationType::MaybeConvertCSSValue(
    const CSSValue& value,
    bool accept_gradients) {
  if (value.IsImageValue() || (value.IsGradientValue() && accept_gradients)) {
    CSSValue* refable_css_value = const_cast<CSSValue*>(&value);
    return InterpolationValue(InterpolableNumber::Create(1),
                              CSSImageNonInterpolableValue::Create(
                                  refable_css_value, refable_css_value));
  }
  return nullptr;
}

PairwiseInterpolationValue
CSSImageInterpolationType::StaticMergeSingleConversions(
    InterpolationValue&& start,
    InterpolationValue&& end) {
  if (!ToCSSImageNonInterpolableValue(*start.non_interpolable_value)
           .IsSingle() ||
      !ToCSSImageNonInterpolableValue(*end.non_interpolable_value).IsSingle()) {
    return nullptr;
  }
  return PairwiseInterpolationValue(
      InterpolableNumber::Create(0), InterpolableNumber::Create(1),
      CSSImageNonInterpolableValue::Merge(start.non_interpolable_value,
                                          end.non_interpolable_value));
}

const CSSValue* CSSImageInterpolationType::CreateCSSValue(
    const InterpolableValue& interpolable_value,
    const NonInterpolableValue* non_interpolable_value,
    const StyleResolverState&) const {
  return StaticCreateCSSValue(interpolable_value, non_interpolable_value);
}

const CSSValue* CSSImageInterpolationType::StaticCreateCSSValue(
    const InterpolableValue& interpolable_value,
    const NonInterpolableValue* non_interpolable_value) {
  return ToCSSImageNonInterpolableValue(non_interpolable_value)
      ->Crossfade(ToInterpolableNumber(interpolable_value).Value());
}

StyleImage* CSSImageInterpolationType::ResolveStyleImage(
    const CSSProperty& property,
    const InterpolableValue& interpolable_value,
    const NonInterpolableValue* non_interpolable_value,
    StyleResolverState& state) {
  const CSSValue* image =
      StaticCreateCSSValue(interpolable_value, non_interpolable_value);
  return state.GetStyleImage(property.PropertyID(), *image);
}

bool CSSImageInterpolationType::EqualNonInterpolableValues(
    const NonInterpolableValue* a,
    const NonInterpolableValue* b) {
  return ToCSSImageNonInterpolableValue(*a).Equals(
      ToCSSImageNonInterpolableValue(*b));
}

class UnderlyingImageChecker
    : public CSSInterpolationType::CSSConversionChecker {
 public:
  ~UnderlyingImageChecker() final = default;

  static std::unique_ptr<UnderlyingImageChecker> Create(
      const InterpolationValue& underlying) {
    return base::WrapUnique(new UnderlyingImageChecker(underlying));
  }

 private:
  UnderlyingImageChecker(const InterpolationValue& underlying)
      : underlying_(underlying.Clone()) {}

  bool IsValid(const StyleResolverState&,
               const InterpolationValue& underlying) const final {
    if (!underlying && !underlying_)
      return true;
    if (!underlying || !underlying_)
      return false;
    return underlying_.interpolable_value->Equals(
               *underlying.interpolable_value) &&
           CSSImageInterpolationType::EqualNonInterpolableValues(
               underlying_.non_interpolable_value.get(),
               underlying.non_interpolable_value.get());
  }

  const InterpolationValue underlying_;
};

InterpolationValue CSSImageInterpolationType::MaybeConvertNeutral(
    const InterpolationValue& underlying,
    ConversionCheckers& conversion_checkers) const {
  conversion_checkers.push_back(UnderlyingImageChecker::Create(underlying));
  return InterpolationValue(underlying.Clone());
}

InterpolationValue CSSImageInterpolationType::MaybeConvertInitial(
    const StyleResolverState&,
    ConversionCheckers& conversion_checkers) const {
  return nullptr;
}

class InheritedImageChecker
    : public CSSInterpolationType::CSSConversionChecker {
 public:
  ~InheritedImageChecker() final = default;

  static std::unique_ptr<InheritedImageChecker> Create(
      const CSSProperty& property,
      StyleImage* inherited_image) {
    return base::WrapUnique(
        new InheritedImageChecker(property, inherited_image));
  }

 private:
  InheritedImageChecker(const CSSProperty& property,
                        StyleImage* inherited_image)
      : property_(property), inherited_image_(inherited_image) {}

  bool IsValid(const StyleResolverState& state,
               const InterpolationValue& underlying) const final {
    const StyleImage* inherited_image =
        GetStyleImage(property_, *state.ParentStyle());
    if (!inherited_image_ && !inherited_image)
      return true;
    if (!inherited_image_ || !inherited_image)
      return false;
    return *inherited_image_ == *inherited_image;
  }

  const CSSProperty& property_;
  Persistent<StyleImage> inherited_image_;
};

InterpolationValue CSSImageInterpolationType::MaybeConvertInherit(
    const StyleResolverState& state,
    ConversionCheckers& conversion_checkers) const {
  if (!state.ParentStyle())
    return nullptr;

  const StyleImage* inherited_image =
      GetStyleImage(CssProperty(), *state.ParentStyle());
  StyleImage* refable_image = const_cast<StyleImage*>(inherited_image);
  conversion_checkers.push_back(
      InheritedImageChecker::Create(CssProperty(), refable_image));
  return MaybeConvertStyleImage(inherited_image, true);
}

InterpolationValue CSSImageInterpolationType::MaybeConvertValue(
    const CSSValue& value,
    const StyleResolverState*,
    ConversionCheckers&) const {
  return MaybeConvertCSSValue(value, true);
}

InterpolationValue
CSSImageInterpolationType::MaybeConvertStandardPropertyUnderlyingValue(
    const ComputedStyle& style) const {
  return MaybeConvertStyleImage(GetStyleImage(CssProperty(), style), true);
}

void CSSImageInterpolationType::Composite(
    UnderlyingValueOwner& underlying_value_owner,
    double underlying_fraction,
    const InterpolationValue& value,
    double interpolation_fraction) const {
  underlying_value_owner.Set(*this, value);
}

void CSSImageInterpolationType::ApplyStandardPropertyValue(
    const InterpolableValue& interpolable_value,
    const NonInterpolableValue* non_interpolable_value,
    StyleResolverState& state) const {
  StyleImage* image = ResolveStyleImage(CssProperty(), interpolable_value,
                                        non_interpolable_value, state);
  switch (CssProperty().PropertyID()) {
    case CSSPropertyBorderImageSource:
      state.Style()->SetBorderImageSource(image);
      break;
    case CSSPropertyListStyleImage:
      state.Style()->SetListStyleImage(image);
      break;
    case CSSPropertyWebkitMaskBoxImageSource:
      state.Style()->SetMaskBoxImageSource(image);
      break;
    default:
      NOTREACHED();
  }
}

}  // namespace blink
