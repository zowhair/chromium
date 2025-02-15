// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_MEDIA_CMA_BACKEND_VIDEO_DECODER_FOR_MIXER_H_
#define CHROMECAST_MEDIA_CMA_BACKEND_VIDEO_DECODER_FOR_MIXER_H_

#include <memory>

#include "chromecast/public/media/media_pipeline_backend.h"
#include "chromecast/public/media/media_pipeline_device_params.h"

namespace chromecast {
namespace media {

// This class represents a video decoder that exposes additional functionality
// that allows a caller to control the rate and state of the video playback
// with enough granularity to be able to sync it to the audio.
//
// The default implementation of this is in VideoDecoderNull. On no-video
// platforms, that implementation is used.
//
// On video platforms that need to use the mixer, you may override this class
// and link in an implementation of VideoDecoderForMixer::Create.
class VideoDecoderForMixer : public MediaPipelineBackend::VideoDecoder {
 public:
  static std::unique_ptr<VideoDecoderForMixer> Create(
      const MediaPipelineDeviceParams& params);

  ~VideoDecoderForMixer() override {}

  // Initializes the VideoDecoderForMixer. Called after allocation and before
  // Start is called. Gives the implementation a chance to initialize any
  // resources.
  virtual void Initialize() = 0;

  // When called, playback is expected to start from |start_pts|.
  //
  // start_pts: the pts to start playing at.
  // need_avsync: deprecated. Don't use or implement.
  // TODO(almasrymina): remove deprecated.
  virtual bool Start(int64_t start_pts, bool need_avsync) = 0;

  // Stop playback.
  virtual void Stop() = 0;

  // Pause playback. The video decoder must retain its playback rate after
  // resume.
  virtual bool Pause() = 0;

  // Resume playback. The video decoder must resume playback at the same
  // playback rate prior to pausing.
  virtual bool Resume() = 0;

  // Get the current video PTS. This will typically be the pts of the last
  // video frame displayed.
  virtual int64_t GetCurrentPts() const = 0;

  // Set the playback rate. This is used to sync the audio to the video. This
  // call will change the rate of play of video in the following manner:
  //
  // rate = 1.0 -> 1 second of video pts is played for each 1 second of
  // wallclock time.
  // rate = 1.5 -> 1.5 seconds of video pts is played for each 1 second of
  // wallclock time.
  // etc.
  virtual bool SetPlaybackRate(float rate) = 0;

  // Sets the current pts to the provided value. If |pts| is greater than the
  // current pts, all video frames in between will be dropped. If |pts| is less
  // than the current pts, all video frames in this pts range will be repeated.
  // Implementation is encouraged to smooth out this transition, such that
  // minimal jitter in the video is shown, but that is not necessary.
  virtual bool SetCurrentPts(int64_t pts) = 0;
};

}  // namespace media
}  // namespace chromecast

#endif  // CHROMECAST_MEDIA_CMA_BACKEND_VIDEO_DECODER_FOR_MIXER_H_
