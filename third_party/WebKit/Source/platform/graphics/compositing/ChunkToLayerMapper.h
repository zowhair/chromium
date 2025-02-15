// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ChunkToLayerMapper_h
#define ChunkToLayerMapper_h

#include "platform/graphics/paint/FloatClipRect.h"
#include "platform/graphics/paint/PropertyTreeState.h"

namespace blink {

struct PaintChunk;

// Maps geometry from PaintChunks to the containing composited layer.
// It provides higher performance than GeometryMapper by reusing computed
// transforms and clips for unchanged states within or across paint chunks.
class PLATFORM_EXPORT ChunkToLayerMapper {
  DISALLOW_NEW_EXCEPT_PLACEMENT_NEW();

 public:
  ChunkToLayerMapper(const PropertyTreeState& layer_state,
                     const gfx::Vector2dF& layer_offset)
      : layer_state_(layer_state),
        layer_offset_(layer_offset),
        chunk_state_(nullptr, nullptr, nullptr) {}

  // This class can map from multiple chunks. Before mapping from a chunk, this
  // method must be called to prepare for the chunk.
  void SwitchToChunk(const PaintChunk&);

  // Maps a visual rectangle in the current chunk space into the layer space.
  IntRect MapVisualRect(const FloatRect&) const;

  // Returns the combined transform from the current chunk to the layer.
  const TransformationMatrix& Transform() const { return transform_; }

  // Returns the combined clip from the current chunk to the layer if it can
  // be calculated (there is no filter that moves pixels), or infinite loose
  // clip rect otherwise.
  const FloatClipRect& ClipRect() const { return clip_rect_; }

 private:
  friend class ChunkToLayerMapperTest;

  IntRect MapUsingGeometryMapper(const FloatRect&) const;

  const PropertyTreeState layer_state_;
  const gfx::Vector2dF layer_offset_;

  // The following fields are chunk-specific which are updated in
  // SwitchToChunk().
  PropertyTreeState chunk_state_;
  float outset_for_raster_effects_ = 0.f;
  TransformationMatrix transform_;
  FloatClipRect clip_rect_;
  // True if there is any pixel-moving filter between chunk state and layer
  // state, and we will call GeometryMapper for each mapping.
  bool has_filter_that_moves_pixels_ = false;
};

}  // namespace blink

#endif  // PaintArtifactCompositor_h
