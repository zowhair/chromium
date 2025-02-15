// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/common/common_custom_types_struct_traits.h"

#include "mojo/public/cpp/system/platform_handle.h"

namespace mojo {

// static
const std::vector<uint32_t>&
StructTraits<common::mojom::VersionDataView, base::Version>::components(
    const base::Version& version) {
  return version.components();
}

// static
bool StructTraits<common::mojom::VersionDataView, base::Version>::Read(
    common::mojom::VersionDataView data,
    base::Version* out) {
  std::vector<uint32_t> components;
  if (!data.ReadComponents(&components))
    return false;

  *out = base::Version(base::Version(std::move(components)));
  return out->IsValid();
}

// static
bool StructTraits<
    common::mojom::UnguessableTokenDataView,
    base::UnguessableToken>::Read(common::mojom::UnguessableTokenDataView data,
                                  base::UnguessableToken* out) {
  uint64_t high = data.high();
  uint64_t low = data.low();

  // Receiving a zeroed UnguessableToken is a security issue.
  if (high == 0 && low == 0)
    return false;

  *out = base::UnguessableToken::Deserialize(high, low);
  return true;
}

// static
common::mojom::TextDirection
EnumTraits<common::mojom::TextDirection, base::i18n::TextDirection>::ToMojom(
    base::i18n::TextDirection text_direction) {
  switch (text_direction) {
    case base::i18n::UNKNOWN_DIRECTION:
      return common::mojom::TextDirection::UNKNOWN_DIRECTION;
    case base::i18n::RIGHT_TO_LEFT:
      return common::mojom::TextDirection::RIGHT_TO_LEFT;
    case base::i18n::LEFT_TO_RIGHT:
      return common::mojom::TextDirection::LEFT_TO_RIGHT;
  }
  NOTREACHED();
  return common::mojom::TextDirection::UNKNOWN_DIRECTION;
}

// static
bool EnumTraits<common::mojom::TextDirection, base::i18n::TextDirection>::
    FromMojom(common::mojom::TextDirection input,
              base::i18n::TextDirection* out) {
  switch (input) {
    case common::mojom::TextDirection::UNKNOWN_DIRECTION:
      *out = base::i18n::UNKNOWN_DIRECTION;
      return true;
    case common::mojom::TextDirection::RIGHT_TO_LEFT:
      *out = base::i18n::RIGHT_TO_LEFT;
      return true;
    case common::mojom::TextDirection::LEFT_TO_RIGHT:
      *out = base::i18n::LEFT_TO_RIGHT;
      return true;
  }
  return false;
}

// static
common::mojom::ThreadPriority
EnumTraits<common::mojom::ThreadPriority, base::ThreadPriority>::ToMojom(
    base::ThreadPriority thread_priority) {
  switch (thread_priority) {
    case base::ThreadPriority::BACKGROUND:
      return common::mojom::ThreadPriority::BACKGROUND;
    case base::ThreadPriority::NORMAL:
      return common::mojom::ThreadPriority::NORMAL;
    case base::ThreadPriority::DISPLAY:
      return common::mojom::ThreadPriority::DISPLAY;
    case base::ThreadPriority::REALTIME_AUDIO:
      return common::mojom::ThreadPriority::REALTIME_AUDIO;
  }
  NOTREACHED();
  return common::mojom::ThreadPriority::BACKGROUND;
}

// static
bool EnumTraits<common::mojom::ThreadPriority, base::ThreadPriority>::FromMojom(
    common::mojom::ThreadPriority input,
    base::ThreadPriority* out) {
  switch (input) {
    case common::mojom::ThreadPriority::BACKGROUND:
      *out = base::ThreadPriority::BACKGROUND;
      return true;
    case common::mojom::ThreadPriority::NORMAL:
      *out = base::ThreadPriority::NORMAL;
      return true;
    case common::mojom::ThreadPriority::DISPLAY:
      *out = base::ThreadPriority::DISPLAY;
      return true;
    case common::mojom::ThreadPriority::REALTIME_AUDIO:
      *out = base::ThreadPriority::REALTIME_AUDIO;
      return true;
  }
  return false;
}

// static
bool StructTraits<common::mojom::MemoryAllocatorDumpCrossProcessUidDataView,
                  base::trace_event::MemoryAllocatorDumpGuid>::
    Read(common::mojom::MemoryAllocatorDumpCrossProcessUidDataView data,
         base::trace_event::MemoryAllocatorDumpGuid* out) {
  // Receiving a zeroed MemoryAllocatorDumpCrossProcessUid is a bug.
  if (data.value() == 0)
    return false;

  *out = base::trace_event::MemoryAllocatorDumpGuid(data.value());
  return true;
}

}  // namespace mojo
