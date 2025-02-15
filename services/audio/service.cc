// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/audio/service.h"

#include <utility>

#include "base/logging.h"
#include "base/macros.h"
#include "base/single_thread_task_runner.h"
#include "build/build_config.h"
#include "media/audio/audio_manager.h"
#include "services/audio/debug_recording.h"
#include "services/audio/system_info.h"
#include "services/service_manager/public/cpp/service_context.h"

namespace audio {

Service::Service(std::unique_ptr<AudioManagerAccessor> audio_manager_accessor)
    : audio_manager_accessor_(std::move(audio_manager_accessor)) {
  DCHECK(audio_manager_accessor_);
}

Service::~Service() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
}

void Service::OnStart() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DVLOG(4) << "audio::Service::OnStart";
  registry_.AddInterface<mojom::SystemInfo>(base::BindRepeating(
      &Service::BindSystemInfoRequest, base::Unretained(this)));
  registry_.AddInterface<mojom::DebugRecording>(base::BindRepeating(
      &Service::BindDebugRecordingRequest, base::Unretained(this)));
}

void Service::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DVLOG(4) << "audio::Service::OnBindInterface";
  registry_.BindInterface(interface_name, std::move(interface_pipe));
}

bool Service::OnServiceManagerConnectionLost() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  // Reset |debug_recording_| to disable debug recording before AudioManager
  // shutdown.
  debug_recording_.reset();
  audio_manager_accessor_->Shutdown();
  return true;
}

void Service::BindSystemInfoRequest(mojom::SystemInfoRequest request) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (!system_info_) {
    DVLOG(4)
        << "audio::Service::BindSystemInfoRequest: lazy SystemInfo creation";
    system_info_ = std::make_unique<SystemInfo>(
        audio_manager_accessor_->GetAudioManager());
  }
  system_info_->Bind(std::move(request));
}

void Service::BindDebugRecordingRequest(mojom::DebugRecordingRequest request) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  // Accept only one bind request at a time. Old request is overwritten.
  // |debug_recording_| must be reset first to disable debug recording, and then
  // create a new DebugRecording instance to enable debug recording.
  debug_recording_.reset();
  debug_recording_ = std::make_unique<DebugRecording>(
      std::move(request), audio_manager_accessor_->GetAudioManager());
}

}  // namespace audio
