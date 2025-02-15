// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gl/gl_image_io_surface_egl.h"

#include "ui/gl/gl_surface_egl.h"

// Enums for the EGL_ANGLE_iosurface_client_buffer extension
#define EGL_IOSURFACE_ANGLE 0x3454
#define EGL_IOSURFACE_PLANE_ANGLE 0x345A
#define EGL_TEXTURE_RECTANGLE_ANGLE 0x345B
#define EGL_TEXTURE_TYPE_ANGLE 0x345C
#define EGL_TEXTURE_INTERNAL_FORMAT_ANGLE 0x345D

namespace gl {

namespace {

struct InternalFormatType {
  InternalFormatType(GLenum format, GLenum type) : format(format), type(type) {}

  GLenum format;
  GLenum type;
};

// Convert a gfx::BufferFormat to a (internal format, type) combination from the
// EGL_ANGLE_iosurface_client_buffer extension spec.
InternalFormatType BufferFormatToInternalFormatType(gfx::BufferFormat format) {
  switch (format) {
    case gfx::BufferFormat::R_8:
      return {GL_RED, GL_UNSIGNED_BYTE};
    case gfx::BufferFormat::R_16:
      return {GL_RED_INTEGER, GL_UNSIGNED_SHORT};
    case gfx::BufferFormat::RG_88:
      return {GL_RG, GL_UNSIGNED_BYTE};
    case gfx::BufferFormat::BGRA_8888:
    case gfx::BufferFormat::BGRX_8888:  // See https://crbug.com/595948.
    case gfx::BufferFormat::RGBA_8888:
      return {GL_BGRA_EXT, GL_UNSIGNED_BYTE};
    case gfx::BufferFormat::RGBA_F16:
      return {GL_RGBA, GL_HALF_FLOAT};
    case gfx::BufferFormat::UYVY_422:
    case gfx::BufferFormat::YUV_420_BIPLANAR:
    case gfx::BufferFormat::BGRX_1010102:
      NOTIMPLEMENTED();
      return {GL_NONE, GL_NONE};
    case gfx::BufferFormat::ATC:
    case gfx::BufferFormat::ATCIA:
    case gfx::BufferFormat::DXT1:
    case gfx::BufferFormat::DXT5:
    case gfx::BufferFormat::ETC1:
    case gfx::BufferFormat::BGR_565:
    case gfx::BufferFormat::RGBA_4444:
    case gfx::BufferFormat::RGBX_8888:
    case gfx::BufferFormat::RGBX_1010102:
    case gfx::BufferFormat::YVU_420:
      NOTREACHED();
      return {GL_NONE, GL_NONE};
  }

  NOTREACHED();
  return {GL_NONE, GL_NONE};
}

}  // anonymous namespace

GLImageIOSurfaceEGL::GLImageIOSurfaceEGL(const gfx::Size& size,
                                         unsigned internalformat)
    : GLImageIOSurface(size, internalformat),
      display_(GLSurfaceEGL::GetHardwareDisplay()),
      pbuffer_(EGL_NO_SURFACE),
      dummy_config_(nullptr),
      texture_bound_(false) {
  DCHECK(display_ != EGL_NO_DISPLAY);

  // When creating a pbuffer we need to supply an EGLConfig. On ANGLE and
  // Swiftshader on Mac, there's only ever one config. Query it from EGL.
  EGLint numConfigs = 0;
  EGLBoolean result =
      eglChooseConfig(display_, nullptr, &dummy_config_, 1, &numConfigs);
  DCHECK(result == EGL_TRUE);
  DCHECK(numConfigs = 1);
  DCHECK(dummy_config_ != nullptr);
}

GLImageIOSurfaceEGL::~GLImageIOSurfaceEGL() {
  if (pbuffer_ != EGL_NO_SURFACE) {
    EGLBoolean result = eglDestroySurface(display_, pbuffer_);
    DCHECK(result == EGL_TRUE);
  }
}

void GLImageIOSurfaceEGL::ReleaseTexImage(unsigned target) {
  DCHECK(target == GL_TEXTURE_RECTANGLE_ARB);
  DCHECK(pbuffer_ != EGL_NO_SURFACE);
  DCHECK(texture_bound_);

  EGLBoolean result = eglReleaseTexImage(display_, pbuffer_, EGL_BACK_BUFFER);
  DCHECK(result == EGL_TRUE);
  texture_bound_ = false;
}

bool GLImageIOSurfaceEGL::BindTexImageImpl(unsigned internalformat) {
  // TODO(cwallez@chromium.org): internalformat is used by Blink's
  // DrawingBuffer::SetupRGBEmulationForBlitFramebuffer to bind an RGBA
  // IOSurface as RGB. We should support this.
  if (internalformat != 0) {
    LOG(ERROR) << "GLImageIOSurfaceEGL doesn't support binding with a custom "
                  "internal format yet.";
    return false;
  }

  // Create the pbuffer representing this IOSurface lazily because we don't know
  // in the constructor if we're going to be used to bind plane 0 to a texture,
  // or to transform YUV to RGB.
  if (pbuffer_ == EGL_NO_SURFACE) {
    InternalFormatType formatType = BufferFormatToInternalFormatType(format_);

    // clang-format off
    const EGLint attribs[] = {
      EGL_WIDTH,                         size_.width(),
      EGL_HEIGHT,                        size_.height(),
      EGL_IOSURFACE_PLANE_ANGLE,         0,
      EGL_TEXTURE_TARGET,                EGL_TEXTURE_RECTANGLE_ANGLE,
      EGL_TEXTURE_INTERNAL_FORMAT_ANGLE, formatType.format,
      EGL_TEXTURE_FORMAT,                EGL_TEXTURE_RGBA,
      EGL_TEXTURE_TYPE_ANGLE,            formatType.type,
      EGL_NONE,                          EGL_NONE,
    };
    // clang-format off

    pbuffer_ = eglCreatePbufferFromClientBuffer(display_, EGL_IOSURFACE_ANGLE,
        io_surface_.get(), dummy_config_, attribs);
    if (pbuffer_ == EGL_NO_SURFACE) {
      LOG(ERROR) << "eglCreatePbufferFromClientBuffer failed, EGL error is "
                 << eglGetError();
      return false;
    }
  }

  DCHECK(!texture_bound_);
  EGLBoolean result = eglBindTexImage(display_, pbuffer_, EGL_BACK_BUFFER);

  if (result != EGL_TRUE) {
    LOG(ERROR) << "eglBindTexImage failed, EGL error is "
               << eglGetError();
    return false;
  }

  texture_bound_ = true;
  return true;
}

}  // namespace gl
