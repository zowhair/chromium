// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/service/transform_feedback_manager.h"

#include "gpu/command_buffer/service/buffer_manager.h"
#include "ui/gl/gl_version_info.h"

namespace gpu {
namespace gles2 {

TransformFeedback::TransformFeedback(TransformFeedbackManager* manager,
                                     GLuint client_id,
                                     GLuint service_id)
    : IndexedBufferBindingHost(
          manager->max_transform_feedback_separate_attribs(),
          GL_TRANSFORM_FEEDBACK_BUFFER,
          manager->needs_emulation()),
      manager_(manager),
      client_id_(client_id),
      service_id_(service_id),
      has_been_bound_(false),
      active_(false),
      paused_(false),
      primitive_mode_(GL_NONE),
      vertices_drawn_(0) {
  DCHECK_LE(0u, client_id);
  DCHECK_LT(0u, service_id);
}

TransformFeedback::~TransformFeedback() {
  if (!manager_->lost_context()) {
    if (active_)
      glEndTransformFeedback();
    glDeleteTransformFeedbacks(1, &service_id_);
  }
}

void TransformFeedback::DoBindTransformFeedback(
    GLenum target,
    TransformFeedback* last_bound_transform_feedback,
    Buffer* bound_transform_feedback_buffer) {
  DCHECK_LT(0u, service_id_);
  glBindTransformFeedback(target, service_id_);
  // GL drivers differ on whether GL_TRANSFORM_FEEDBACK_BUFFER is changed
  // when calling glBindTransformFeedback. To make them consistent, we
  // explicitly bind the buffer we think should be bound here.
  if (bound_transform_feedback_buffer &&
      !bound_transform_feedback_buffer->IsDeleted()) {
    glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER,
                 bound_transform_feedback_buffer->service_id());
  } else {
    glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, 0);
  }
  has_been_bound_ = true;
  if (active_ && !paused_) {
    // This could only happen during virtual context switching.
    // Otherwise the validation should generate a GL error without calling
    // into this function.
    glResumeTransformFeedback();
  }
  if (last_bound_transform_feedback != this) {
    if (last_bound_transform_feedback) {
      last_bound_transform_feedback->SetIsBound(false);
    }
    SetIsBound(true);
  }
}

void TransformFeedback::DoBeginTransformFeedback(GLenum primitive_mode) {
  DCHECK(!active_);
  DCHECK(primitive_mode == GL_POINTS ||
         primitive_mode == GL_LINES ||
         primitive_mode == GL_TRIANGLES);
  glBeginTransformFeedback(primitive_mode);
  active_ = true;
  primitive_mode_ = primitive_mode;
  vertices_drawn_ = 0;
}

void TransformFeedback::DoEndTransformFeedback() {
  DCHECK(active_);
  glEndTransformFeedback();
  active_ = false;
  paused_ = false;
}

void TransformFeedback::DoPauseTransformFeedback() {
  DCHECK(active_ && !paused_);
  glPauseTransformFeedback();
  paused_ = true;
}

void TransformFeedback::DoResumeTransformFeedback() {
  DCHECK(active_ && paused_);
  glResumeTransformFeedback();
  paused_ = false;
}

GLsizei TransformFeedback::VerticesNeededForDraw(GLenum mode,
                                                 GLsizei count,
                                                 GLsizei primcount) const {
  // Transform feedback only outputs complete primitives, so we need to round
  // down to the nearest complete primitive before multiplying by the number of
  // instances.
  switch (mode) {
    case GL_TRIANGLES:
      return vertices_drawn_ + primcount * (count - count % 3);
    case GL_LINES:
      return vertices_drawn_ + primcount * (count - count % 2);
    default:
      NOTREACHED();
      FALLTHROUGH;
    case GL_POINTS:
      return vertices_drawn_ + primcount * count;
  }
}

void TransformFeedback::OnVerticesDrawn(GLenum mode,
                                        GLsizei count,
                                        GLsizei primcount) {
  if (active_ && !paused_) {
    vertices_drawn_ = VerticesNeededForDraw(mode, count, primcount);
  }
}

TransformFeedbackManager::TransformFeedbackManager(
    GLuint max_transform_feedback_separate_attribs,
    bool needs_emulation)
    : max_transform_feedback_separate_attribs_(
          max_transform_feedback_separate_attribs),
      needs_emulation_(needs_emulation),
      lost_context_(false) {
  DCHECK(needs_emulation);
}

TransformFeedbackManager::~TransformFeedbackManager() {
  DCHECK(transform_feedbacks_.empty());
}

void TransformFeedbackManager::Destroy() {
  transform_feedbacks_.clear();
}

TransformFeedback* TransformFeedbackManager::CreateTransformFeedback(
    GLuint client_id, GLuint service_id) {
  scoped_refptr<TransformFeedback> transform_feedback(
      new TransformFeedback(this, client_id, service_id));
  auto result = transform_feedbacks_.insert(
      std::make_pair(client_id, transform_feedback));
  DCHECK(result.second);
  return result.first->second.get();
}

TransformFeedback* TransformFeedbackManager::GetTransformFeedback(
    GLuint client_id) {
  if (client_id == 0) {
    return nullptr;
  }
  auto it = transform_feedbacks_.find(client_id);
  return it != transform_feedbacks_.end() ? it->second.get() : nullptr;
}

void TransformFeedbackManager::RemoveTransformFeedback(GLuint client_id) {
  if (client_id) {
    transform_feedbacks_.erase(client_id);
  }
}

}  // namespace gles2
}  // namespace gpu
