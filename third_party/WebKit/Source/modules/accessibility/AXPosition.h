// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef AXPosition_h
#define AXPosition_h

#include <base/logging.h>
#include <base/optional.h>
#include <stdint.h>
#include <ostream>

#include "core/editing/Forward.h"
#include "core/editing/TextAffinity.h"
#include "modules/ModulesExport.h"
#include "platform/heap/Persistent.h"
#include "platform/wtf/Allocator.h"

namespace blink {

class AXObject;

// Describes a position in the Blink accessibility tree.
// A position is either anchored to before or after a child object inside a
// container object, or is anchored to a character inside a text object.
class MODULES_EXPORT AXPosition final {
  DISALLOW_NEW_EXCEPT_PLACEMENT_NEW();

 public:
  static const AXPosition CreatePositionBeforeObject(const AXObject& child);
  static const AXPosition CreatePositionAfterObject(const AXObject& child);
  static const AXPosition CreateFirstPositionInContainerObject(
      const AXObject& container);
  static const AXPosition CreateLastPositionInContainerObject(
      const AXObject& container);
  static const AXPosition CreatePositionInTextObject(
      const AXObject& container,
      int offset,
      TextAffinity = TextAffinity::kDownstream);
  static const AXPosition FromPosition(const Position&);
  static const AXPosition FromPosition(const PositionWithAffinity&);

  AXPosition(const AXPosition&) = default;
  AXPosition& operator=(const AXPosition&) = default;
  ~AXPosition() = default;

  const AXObject* ContainerObject() const { return container_object_; }
  const AXObject* ObjectAfterPosition() const;
  int ChildIndex() const;
  int TextOffset() const;
  TextAffinity Affinity() const { return affinity_; }

  // Verifies if the anchor is present and if it's set to a live object with a
  // connected node.
  bool IsValid() const;

  // Returns whether this is a position anchored to a character inside a text
  // object.
  bool IsTextPosition() const;

  const PositionWithAffinity ToPositionWithAffinity() const;

 private:
  AXPosition();
  explicit AXPosition(const AXObject& container);

  // The |AXObject| in which the position is present.
  // Only valid during a single document lifecycle hence no need to maintain a
  // strong reference to it.
  WeakPersistent<const AXObject> container_object_;

  // If the position is anchored to before or after an object, the number of
  // child objects in |container_object_| that come before the position.
  // If this is a text position, the number of characters in the canonical text
  // of |container_object_| before the position. The canonical text is the DOM
  // node's text but with, e.g., whitespace collapsed and any transformations
  // applied.
  base::Optional<int> text_offset_or_child_index_;

  // When the same character offset could correspond to two possible caret
  // positions, upstream means it's on the previous line rather than the next
  // line.
  TextAffinity affinity_;

#if DCHECK_IS_ON()
  // TODO(ax-dev): Use layout tree version in place of DOM and style versions.
  uint64_t dom_tree_version_;
  uint64_t style_version_;
#endif

  // For access to our constructor for use when creating empty AX selections.
  // There is no sense in creating empty positions in other circomstances so we
  // disallow it.
  friend class AXSelection;
};

MODULES_EXPORT bool operator==(const AXPosition&, const AXPosition&);
MODULES_EXPORT bool operator!=(const AXPosition&, const AXPosition&);
MODULES_EXPORT std::ostream& operator<<(std::ostream&, const AXPosition&);

}  // namespace blink

#endif  // AXPosition_h
