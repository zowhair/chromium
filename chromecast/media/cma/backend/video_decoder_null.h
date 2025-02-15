// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_MEDIA_CMA_BACKEND_VIDEO_DECODER_NULL_H_
#define CHROMECAST_MEDIA_CMA_BACKEND_VIDEO_DECODER_NULL_H_

#include <stdint.h>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chromecast/media/cma/backend/video_decoder_for_mixer.h"

namespace chromecast {
namespace media {

class VideoDecoderNull : public VideoDecoderForMixer {
 public:
  VideoDecoderNull();
  ~VideoDecoderNull() override;

  // MediaPipelineBackend::VideoDecoder implementation:
  void SetDelegate(Delegate* delegate) override;
  MediaPipelineBackend::BufferStatus PushBuffer(
      CastDecoderBuffer* buffer) override;
  void GetStatistics(Statistics* statistics) override;
  bool SetConfig(const VideoConfig& config) override;

  void Initialize() override;
  bool Start(int64_t start_pts, bool need_avsync) override;
  void Stop() override;
  bool Pause() override;
  bool Resume() override;
  int64_t GetCurrentPts() const override;
  bool SetPlaybackRate(float rate) override;
  bool SetCurrentPts(int64_t pts) override;

 private:
  void OnEndOfStream();

  Delegate* delegate_;
  base::WeakPtrFactory<VideoDecoderNull> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(VideoDecoderNull);
};

}  // namespace media
}  // namespace chromecast

#endif  // CHROMECAST_MEDIA_CMA_BACKEND_VIDEO_DECODER_NULL_H_
