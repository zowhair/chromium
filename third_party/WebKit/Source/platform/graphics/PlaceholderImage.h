// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PlaceholderImage_h
#define PlaceholderImage_h

#include <stdint.h>

#include "base/memory/scoped_refptr.h"
#include "platform/geometry/IntSize.h"
#include "platform/graphics/Image.h"
#include "platform/graphics/ImageOrientation.h"
#include "platform/wtf/Optional.h"
#include "platform/wtf/text/WTFString.h"
#include "third_party/skia/include/core/SkImage.h"
#include "third_party/skia/include/core/SkRefCnt.h"

namespace blink {

class FloatPoint;
class FloatRect;
class FloatSize;
class GraphicsContext;
class ImageObserver;

// A generated placeholder image that shows a translucent gray rectangle.
class PLATFORM_EXPORT PlaceholderImage final : public Image {
 public:
  static scoped_refptr<PlaceholderImage> Create(
      ImageObserver* observer,
      const IntSize& size,
      int64_t original_resource_size) {
    return base::AdoptRef(
        new PlaceholderImage(observer, size, original_resource_size));
  }

  ~PlaceholderImage() override;

  IntSize Size() const override;

  void Draw(PaintCanvas*,
            const PaintFlags&,
            const FloatRect& dest_rect,
            const FloatRect& src_rect,
            RespectImageOrientationEnum,
            ImageClampingMode,
            ImageDecodingMode) override;

  void DestroyDecodedData() override;

  PaintImage PaintImageForCurrentFrame() override;

  bool IsPlaceholderImage() const override;

  const String& GetTextForTesting() const { return text_; }

 private:
  PlaceholderImage(ImageObserver*,
                   const IntSize&,
                   int64_t original_resource_size);

  bool CurrentFrameHasSingleSecurityOrigin() const override;

  bool CurrentFrameKnownToBeOpaque() override;

  void DrawPattern(GraphicsContext&,
                   const FloatRect& src_rect,
                   const FloatSize& scale,
                   const FloatPoint& phase,
                   SkBlendMode,
                   const FloatRect& dest_rect,
                   const FloatSize& repeat_spacing) override;

  // SetData does nothing, and the passed in buffer is ignored.
  SizeAvailability SetData(scoped_refptr<SharedBuffer>, bool) override;

  const IntSize size_;
  const String text_;

  class SharedFont;
  // Lazily initialized. All instances of PlaceholderImage will share the same
  // Font object, wrapped as a SharedFont.
  scoped_refptr<SharedFont> shared_font_;

  // Lazily initialized.
  Optional<float> cached_text_width_;
  sk_sp<PaintRecord> paint_record_for_current_frame_;
  PaintImage::ContentId paint_record_content_id_;
};

}  // namespace blink

#endif
