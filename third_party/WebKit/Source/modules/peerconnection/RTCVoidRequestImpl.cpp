/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Google Inc. nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "modules/peerconnection/RTCVoidRequestImpl.h"

#include "core/dom/DOMException.h"
#include "core/dom/ExceptionCode.h"
#include "modules/peerconnection/RTCErrorUtil.h"
#include "modules/peerconnection/RTCPeerConnection.h"

namespace blink {

RTCVoidRequestImpl* RTCVoidRequestImpl::Create(
    ExecutionContext* context,
    RTCPeerConnection* requester,
    V8VoidFunction* success_callback,
    V8RTCPeerConnectionErrorCallback* error_callback) {
  return new RTCVoidRequestImpl(context, requester, success_callback,
                                error_callback);
}

RTCVoidRequestImpl::RTCVoidRequestImpl(
    ExecutionContext* context,
    RTCPeerConnection* requester,
    V8VoidFunction* success_callback,
    V8RTCPeerConnectionErrorCallback* error_callback)
    : ContextLifecycleObserver(context),
      success_callback_(ToV8PersistentCallbackFunction(success_callback)),
      error_callback_(ToV8PersistentCallbackFunction(error_callback)),
      requester_(requester) {
  DCHECK(requester_);
}

RTCVoidRequestImpl::~RTCVoidRequestImpl() = default;

void RTCVoidRequestImpl::RequestSucceeded() {
  bool should_fire_callback =
      requester_ && requester_->ShouldFireDefaultCallbacks();
  if (should_fire_callback && success_callback_)
    success_callback_->InvokeAndReportException(nullptr);

  Clear();
}

void RTCVoidRequestImpl::RequestFailed(const WebRTCError& error) {
  bool should_fire_callback =
      requester_ && requester_->ShouldFireDefaultCallbacks();
  if (should_fire_callback && error_callback_.Get()) {
    error_callback_->InvokeAndReportException(
        nullptr, CreateDOMExceptionFromWebRTCError(error));
  }

  Clear();
}

void RTCVoidRequestImpl::ContextDestroyed(ExecutionContext*) {
  Clear();
}

void RTCVoidRequestImpl::Clear() {
  success_callback_.Clear();
  error_callback_.Clear();
  requester_.Clear();
}

void RTCVoidRequestImpl::Trace(blink::Visitor* visitor) {
  visitor->Trace(success_callback_);
  visitor->Trace(error_callback_);
  visitor->Trace(requester_);
  RTCVoidRequest::Trace(visitor);
  ContextLifecycleObserver::Trace(visitor);
}

}  // namespace blink
