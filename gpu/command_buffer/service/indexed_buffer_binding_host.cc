// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/service/indexed_buffer_binding_host.h"

#include "gpu/command_buffer/service/buffer_manager.h"

namespace gpu {
namespace gles2 {

IndexedBufferBindingHost::IndexedBufferBinding::IndexedBufferBinding()
    : type(kBindBufferNone),
      offset(0),
      size(0),
      effective_full_buffer_size(0) {
}

IndexedBufferBindingHost::IndexedBufferBinding::IndexedBufferBinding(
    const IndexedBufferBindingHost::IndexedBufferBinding& other)
    : type(other.type),
      buffer(other.buffer.get()),
      offset(other.offset),
      size(other.size),
      effective_full_buffer_size(other.effective_full_buffer_size) {
}

IndexedBufferBindingHost::IndexedBufferBinding::~IndexedBufferBinding() =
    default;

bool IndexedBufferBindingHost::IndexedBufferBinding::operator==(
    const IndexedBufferBindingHost::IndexedBufferBinding& other) const {
  if (type == kBindBufferNone && other.type == kBindBufferNone) {
    // This should be the most common case so an early out.
    return true;
  }
  return (type == other.type &&
          buffer.get() == other.buffer.get() &&
          offset == other.offset &&
          size == other.size &&
          effective_full_buffer_size == other.effective_full_buffer_size);
}

void IndexedBufferBindingHost::IndexedBufferBinding::SetBindBufferBase(
    Buffer* _buffer) {
  if (!_buffer) {
    Reset();
    return;
  }
  type = kBindBufferBase;
  buffer = _buffer;
  offset = 0;
  size = 0;
  effective_full_buffer_size = 0;
}

void IndexedBufferBindingHost::IndexedBufferBinding::SetBindBufferRange(
    Buffer* _buffer, GLintptr _offset, GLsizeiptr _size) {
  if (!_buffer) {
    Reset();
    return;
  }
  type = kBindBufferRange;
  buffer = _buffer;
  offset = _offset;
  size = _size;
  effective_full_buffer_size = _buffer ? _buffer->size() : 0;
}

void IndexedBufferBindingHost::IndexedBufferBinding::Reset() {
  type = kBindBufferNone;
  buffer = nullptr;
  offset = 0;
  size = 0;
  effective_full_buffer_size = 0;
}

IndexedBufferBindingHost::IndexedBufferBindingHost(uint32_t max_bindings,
                                                   GLenum target,
                                                   bool needs_emulation)
    : is_bound_(false),
      needs_emulation_(needs_emulation),
      max_non_null_binding_index_plus_one_(0u),
      target_(target) {
  DCHECK(needs_emulation);
  buffer_bindings_.resize(max_bindings);
}

IndexedBufferBindingHost::~IndexedBufferBindingHost() {
  SetIsBound(false);
}

void IndexedBufferBindingHost::DoBindBufferBase(GLuint index, Buffer* buffer) {
  DCHECK_LT(index, buffer_bindings_.size());
  GLuint service_id = buffer ? buffer->service_id() : 0;
  glBindBufferBase(target_, index, service_id);

  if (buffer_bindings_[index].buffer && is_bound_) {
    buffer_bindings_[index].buffer->OnUnbind(target_);
  }
  buffer_bindings_[index].SetBindBufferBase(buffer);
  if (buffer && is_bound_) {
    buffer->OnBind(target_);
  }
  UpdateMaxNonNullBindingIndex(index);
}

void IndexedBufferBindingHost::DoBindBufferRange(GLuint index,
                                                 Buffer* buffer,
                                                 GLintptr offset,
                                                 GLsizeiptr size) {
  DCHECK_LT(index, buffer_bindings_.size());
  GLuint service_id = buffer ? buffer->service_id() : 0;
  if (buffer && needs_emulation_) {
    DoAdjustedBindBufferRange(target_, index, service_id, offset, size,
                              buffer->size());
  } else {
    glBindBufferRange(target_, index, service_id, offset, size);
  }

  if (buffer_bindings_[index].buffer && is_bound_) {
    buffer_bindings_[index].buffer->OnUnbind(target_);
  }
  buffer_bindings_[index].SetBindBufferRange(buffer, offset, size);
  if (buffer && is_bound_) {
    buffer->OnBind(target_);
  }
  UpdateMaxNonNullBindingIndex(index);
}

// static
void IndexedBufferBindingHost::DoAdjustedBindBufferRange(
    GLenum target, GLuint index, GLuint service_id, GLintptr offset,
    GLsizeiptr size, GLsizeiptr full_buffer_size) {
  GLsizeiptr adjusted_size = size;
  if (offset >= full_buffer_size) {
    // Situation 1: We can't really call glBindBufferRange with reasonable
    // offset/size without triggering a GL error because size == 0 isn't
    // valid.
    // TODO(zmo): it's ambiguous in the GL 4.1 spec whether BindBufferBase
    // generates a GL error in such case. In reality, no error is generated on
    // MacOSX with AMD/4.1.
    glBindBufferBase(target, index, service_id);
    return;
  } else if (offset + size > full_buffer_size) {
    adjusted_size = full_buffer_size - offset;
    // size needs to be a multiple of 4.
    adjusted_size = adjusted_size & ~3;
    if (adjusted_size == 0) {
      // Situation 2: The original size is valid, but the adjusted size
      // is 0 and isn't valid. Handle it the same way as situation 1.
      glBindBufferBase(target, index, service_id);
      return;
    }
  }
  glBindBufferRange(target, index, service_id, offset, adjusted_size);
}

void IndexedBufferBindingHost::OnBufferData(Buffer* buffer) {
  DCHECK(buffer);
  if (needs_emulation_) {
    // If some bound buffers change size since last time the transformfeedback
    // is bound, we might need to reset the ranges.
    for (size_t ii = 0; ii < buffer_bindings_.size(); ++ii) {
      if (buffer_bindings_[ii].buffer.get() != buffer)
        continue;
      if (buffer_bindings_[ii].type == kBindBufferRange &&
          buffer_bindings_[ii].effective_full_buffer_size != buffer->size()) {
        DoAdjustedBindBufferRange(target_, ii, buffer->service_id(),
                                  buffer_bindings_[ii].offset,
                                  buffer_bindings_[ii].size, buffer->size());
        buffer_bindings_[ii].effective_full_buffer_size = buffer->size();
      }
    }
  }
}

void IndexedBufferBindingHost::RemoveBoundBuffer(Buffer* buffer) {
  for (size_t ii = 0; ii < buffer_bindings_.size(); ++ii) {
    if (buffer_bindings_[ii].buffer.get() == buffer) {
      buffer_bindings_[ii].Reset();
      UpdateMaxNonNullBindingIndex(ii);
    }
  }
}

void IndexedBufferBindingHost::SetIsBound(bool is_bound) {
  if (is_bound && needs_emulation_) {
    // If some bound buffers change size since last time the transformfeedback
    // is bound, we might need to reset the ranges.
    for (size_t ii = 0; ii < buffer_bindings_.size(); ++ii) {
      Buffer* buffer = buffer_bindings_[ii].buffer.get();
      if (buffer && buffer_bindings_[ii].type == kBindBufferRange &&
          buffer_bindings_[ii].effective_full_buffer_size != buffer->size()) {
        DoAdjustedBindBufferRange(target_, ii, buffer->service_id(),
                                  buffer_bindings_[ii].offset,
                                  buffer_bindings_[ii].size, buffer->size());
        buffer_bindings_[ii].effective_full_buffer_size = buffer->size();
      }
    }
  }

  if (is_bound != is_bound_) {
    is_bound_ = is_bound;
    for (auto& bb : buffer_bindings_) {
      if (bb.buffer) {
        if (is_bound_) {
          bb.buffer->OnBind(target_);
        } else {
          bb.buffer->OnUnbind(target_);
        }
      }
    }
  }
}

Buffer* IndexedBufferBindingHost::GetBufferBinding(GLuint index) const {
  DCHECK_LT(index, buffer_bindings_.size());
  return buffer_bindings_[index].buffer.get();
}

GLsizeiptr IndexedBufferBindingHost::GetBufferSize(GLuint index) const {
  DCHECK_LT(index, buffer_bindings_.size());
  return buffer_bindings_[index].size;
}

GLsizeiptr IndexedBufferBindingHost::GetEffectiveBufferSize(
    GLuint index) const {
  DCHECK_LT(index, buffer_bindings_.size());
  const IndexedBufferBinding& binding = buffer_bindings_[index];
  if (!binding.buffer.get())
    return 0;
  GLsizeiptr full_buffer_size = binding.buffer->size();
  switch (binding.type) {
    case kBindBufferBase:
      return full_buffer_size;
    case kBindBufferRange:
      if (binding.offset + binding.size > full_buffer_size)
        return full_buffer_size - binding.offset;
      return binding.size;
    case kBindBufferNone:
      return 0;
  }
  return buffer_bindings_[index].size;
}

GLintptr IndexedBufferBindingHost::GetBufferStart(GLuint index) const {
  DCHECK_LT(index, buffer_bindings_.size());
  return buffer_bindings_[index].offset;
}

void IndexedBufferBindingHost::RestoreBindings(
    IndexedBufferBindingHost* prev) {
  // This is used only for UNIFORM_BUFFER bindings in context switching.
  DCHECK(target_ == GL_UNIFORM_BUFFER && (!prev || prev->target_ == target_));
  size_t limit = max_non_null_binding_index_plus_one_;
  if (prev && prev->max_non_null_binding_index_plus_one_ > limit) {
    limit = prev->max_non_null_binding_index_plus_one_;
  }
  for (size_t ii = 0; ii < limit; ++ii) {
    if (prev && buffer_bindings_[ii] == prev->buffer_bindings_[ii]) {
      continue;
    }
    switch (buffer_bindings_[ii].type) {
      case kBindBufferBase:
      case kBindBufferNone:
        DoBindBufferBase(ii, buffer_bindings_[ii].buffer.get());
        break;
      case kBindBufferRange:
        DoBindBufferRange(ii, buffer_bindings_[ii].buffer.get(),
                          buffer_bindings_[ii].offset,
                          buffer_bindings_[ii].size);
        break;
    }
  }
}

void IndexedBufferBindingHost::UpdateMaxNonNullBindingIndex(
    size_t changed_index) {
  size_t plus_one = changed_index + 1;
  DCHECK_LT(changed_index, buffer_bindings_.size());
  if (buffer_bindings_[changed_index].buffer.get()) {
    max_non_null_binding_index_plus_one_ =
        std::max(max_non_null_binding_index_plus_one_, plus_one);
  } else {
    if (plus_one == max_non_null_binding_index_plus_one_) {
      for (size_t ii = changed_index; ii > 0; --ii) {
        if (buffer_bindings_[ii - 1].buffer.get()) {
          max_non_null_binding_index_plus_one_ = ii;
          break;
        }
      }
    }
  }
}

bool IndexedBufferBindingHost::UsesBuffer(
    size_t used_binding_count, const Buffer* buffer) const {
  DCHECK(buffer);
  DCHECK_LE(used_binding_count, buffer_bindings_.size());
  for (size_t ii = 0; ii < used_binding_count; ++ii) {
    if (buffer == buffer_bindings_[ii].buffer)
      return true;
  }
  return false;
}

}  // namespace gles2
}  // namespace gpu
