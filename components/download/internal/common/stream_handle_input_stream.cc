// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/public/common/stream_handle_input_stream.h"

#include "base/bind.h"
#include "components/download/public/common/download_interrupt_reasons_utils.h"
#include "mojo/public/c/system/types.h"

namespace download {

namespace {
// Data length to read from data pipe.
const int kBytesToRead = 4096;
}  // namespace

StreamHandleInputStream::StreamHandleInputStream(
    mojom::DownloadStreamHandlePtr stream_handle)
    : stream_handle_(std::move(stream_handle)),
      is_response_completed_(false),
      completion_status_(DOWNLOAD_INTERRUPT_REASON_NONE) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

StreamHandleInputStream::~StreamHandleInputStream() = default;

void StreamHandleInputStream::Initialize() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  binding_ = std::make_unique<mojo::Binding<mojom::DownloadStreamClient>>(
      this, std::move(stream_handle_->client_request));
  binding_->set_connection_error_handler(base::BindOnce(
      &StreamHandleInputStream::OnStreamCompleted, base::Unretained(this),
      mojom::NetworkRequestStatus::USER_CANCELED));
  handle_watcher_ = std::make_unique<mojo::SimpleWatcher>(
      FROM_HERE, mojo::SimpleWatcher::ArmingPolicy::AUTOMATIC);
}

bool StreamHandleInputStream::IsEmpty() {
  return !stream_handle_;
}

void StreamHandleInputStream::RegisterDataReadyCallback(
    const mojo::SimpleWatcher::ReadyCallback& callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (handle_watcher_) {
    handle_watcher_->Watch(stream_handle_->stream.get(),
                           MOJO_HANDLE_SIGNAL_READABLE, callback);
  }
}

void StreamHandleInputStream::ClearDataReadyCallback() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (handle_watcher_)
    handle_watcher_->Cancel();
}

void StreamHandleInputStream::RegisterCompletionCallback(
    base::OnceClosure callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  completion_callback_ = std::move(callback);
}

InputStream::StreamState StreamHandleInputStream::Read(
    scoped_refptr<net::IOBuffer>* data,
    size_t* length) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!handle_watcher_)
    return InputStream::EMPTY;

  *length = kBytesToRead;
  *data = new net::IOBuffer(kBytesToRead);
  MojoResult mojo_result = stream_handle_->stream->ReadData(
      (*data)->data(), (uint32_t*)length, MOJO_READ_DATA_FLAG_NONE);
  // TODO(qinmin): figure out when COMPLETE should be returned.
  switch (mojo_result) {
    case MOJO_RESULT_OK:
      return InputStream::HAS_DATA;
    case MOJO_RESULT_SHOULD_WAIT:
      return InputStream::EMPTY;
    case MOJO_RESULT_FAILED_PRECONDITION:
      if (is_response_completed_)
        return InputStream::COMPLETE;
      stream_handle_->stream.reset();
      ClearDataReadyCallback();
      return InputStream::WAIT_FOR_COMPLETION;
    case MOJO_RESULT_INVALID_ARGUMENT:
    case MOJO_RESULT_OUT_OF_RANGE:
    case MOJO_RESULT_BUSY:
      NOTREACHED();
      return InputStream::COMPLETE;
  }
  return InputStream::EMPTY;
}

DownloadInterruptReason StreamHandleInputStream::GetCompletionStatus() {
  return completion_status_;
}

void StreamHandleInputStream::OnStreamCompleted(
    mojom::NetworkRequestStatus status) {
  // This can be called before or after data pipe is completely drained.
  completion_status_ = ConvertMojoNetworkRequestStatusToInterruptReason(status);
  is_response_completed_ = true;
  if (completion_callback_)
    std::move(completion_callback_).Run();
}

}  // namespace download
