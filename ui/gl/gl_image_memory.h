// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GL_GL_IMAGE_MEMORY_H_
#define UI_GL_GL_IMAGE_MEMORY_H_

#include "ui/gl/gl_image.h"

#include <stddef.h>

#include "base/macros.h"
#include "base/numerics/safe_math.h"
#include "ui/gfx/buffer_types.h"
#include "ui/gl/gl_export.h"

namespace gl {

class GL_EXPORT GLImageMemory : public GLImage {
 public:
  GLImageMemory(const gfx::Size& size, unsigned internalformat);

  bool Initialize(const unsigned char* memory,
                  gfx::BufferFormat format,
                  size_t stride);

  // Safe downcast. Returns |nullptr| on failure.
  static GLImageMemory* FromGLImage(GLImage* image);

  // Overridden from GLImage:
  gfx::Size GetSize() override;
  unsigned GetInternalFormat() override;
  bool BindTexImage(unsigned target) override;
  void ReleaseTexImage(unsigned target) override {}
  bool CopyTexImage(unsigned target) override;
  bool CopyTexSubImage(unsigned target,
                       const gfx::Point& offset,
                       const gfx::Rect& rect) override;
  bool ScheduleOverlayPlane(gfx::AcceleratedWidget widget,
                            int z_order,
                            gfx::OverlayTransform transform,
                            const gfx::Rect& bounds_rect,
                            const gfx::RectF& crop_rect) override;
  void SetColorSpace(const gfx::ColorSpace& color_space) override {}
  void Flush() override {}
  Type GetType() const override;

  static unsigned GetInternalFormatForTesting(gfx::BufferFormat format);

  const unsigned char* memory() { return memory_; }
  size_t stride() const { return stride_; }
  gfx::BufferFormat format() const { return format_; }

 protected:
  ~GLImageMemory() override;

 private:
  static bool ValidFormat(gfx::BufferFormat format);

  const gfx::Size size_;
  const unsigned internalformat_;
  const unsigned char* memory_;
  gfx::BufferFormat format_;
  size_t stride_;

  DISALLOW_COPY_AND_ASSIGN(GLImageMemory);
};

}  // namespace gl

#endif  // UI_GL_GL_IMAGE_MEMORY_H_
