// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_AUDIO_DEBUG_RECORDING_H_
#define SERVICES_AUDIO_DEBUG_RECORDING_H_

#include <utility>

#include "base/gtest_prod_util.h"
#include "base/memory/weak_ptr.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/audio/public/mojom/debug_recording.mojom.h"

namespace media {
class AudioManager;
enum class AudioDebugRecordingStreamType;
}

namespace audio {

// Implementation for controlling audio debug recording.
class DebugRecording : public mojom::DebugRecording {
 public:
  DebugRecording(mojom::DebugRecordingRequest request,
                 media::AudioManager* audio_manager);

  // Disables audio debug recording if Enable() was called before.
  ~DebugRecording() override;

  // Enables audio debug recording.
  void Enable(mojom::DebugRecordingFileProviderPtr file_provider) override;

 private:
  FRIEND_TEST_ALL_PREFIXES(DebugRecordingTest,
                           CreateWavFileCallsFileProviderCreateWavFile);
  // Called on binding connection error.
  void Disable();

  void CreateWavFile(
      media::AudioDebugRecordingStreamType stream_type,
      uint32_t id,
      mojom::DebugRecordingFileProvider::CreateWavFileCallback reply_callback);
  bool IsEnabled();

  mojo::Binding<mojom::DebugRecording> binding_;
  mojom::DebugRecordingFileProviderPtr file_provider_;
  media::AudioManager* const audio_manager_;

  base::WeakPtrFactory<DebugRecording> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(DebugRecording);
};

}  // namespace audio

#endif  // SERVICES_AUDIO_DEBUG_RECORDING_H_
