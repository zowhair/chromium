// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/editing/markers/SuggestionMarkerProperties.h"

namespace blink {

SuggestionMarkerProperties::SuggestionMarkerProperties(
    const SuggestionMarkerProperties& other) = default;
SuggestionMarkerProperties::SuggestionMarkerProperties() = default;
SuggestionMarkerProperties::Builder::Builder() = default;

SuggestionMarkerProperties::Builder::Builder(
    const SuggestionMarkerProperties& data) {
  data_ = data;
}

SuggestionMarkerProperties SuggestionMarkerProperties::Builder::Build() const {
  return data_;
}

SuggestionMarkerProperties::Builder&
SuggestionMarkerProperties::Builder::SetType(
    SuggestionMarker::SuggestionType type) {
  data_.type_ = type;
  return *this;
}

SuggestionMarkerProperties::Builder&
SuggestionMarkerProperties::Builder::SetSuggestions(
    const Vector<String>& suggestions) {
  data_.suggestions_ = suggestions;
  return *this;
}

SuggestionMarkerProperties::Builder&
SuggestionMarkerProperties::Builder::SetHighlightColor(Color highlight_color) {
  data_.highlight_color_ = highlight_color;
  return *this;
}

SuggestionMarkerProperties::Builder&
SuggestionMarkerProperties::Builder::SetUnderlineColor(Color underline_color) {
  data_.underline_color_ = underline_color;
  return *this;
}

SuggestionMarkerProperties::Builder&
SuggestionMarkerProperties::Builder::SetBackgroundColor(
    Color background_color) {
  data_.background_color_ = background_color;
  return *this;
}

SuggestionMarkerProperties::Builder&
SuggestionMarkerProperties::Builder::SetThickness(
    ui::mojom::ImeTextSpanThickness thickness) {
  data_.thickness_ = thickness;
  return *this;
}

}  // namespace blink
