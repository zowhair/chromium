// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_ANDROID_SYNCHRONOUS_COMPOSITOR_PROXY_H_
#define CONTENT_RENDERER_ANDROID_SYNCHRONOUS_COMPOSITOR_PROXY_H_

#include <stddef.h>
#include <stdint.h>

#include "base/callback.h"
#include "base/macros.h"
#include "base/optional.h"
#include "content/common/input/synchronous_compositor.mojom.h"
#include "content/public/common/input_event_ack_state.h"
#include "content/renderer/android/synchronous_layer_tree_frame_sink.h"
#include "mojo/public/cpp/bindings/associated_binding.h"
#include "ui/events/blink/synchronous_input_handler_proxy.h"
#include "ui/gfx/geometry/scroll_offset.h"
#include "ui/gfx/geometry/size_f.h"

namespace viz {
class CompositorFrame;
}  // namespace viz

namespace content {

class SynchronousLayerTreeFrameSink;
struct SyncCompositorCommonRendererParams;
struct SyncCompositorDemandDrawHwParams;
struct SyncCompositorDemandDrawSwParams;
struct SyncCompositorSetSharedMemoryParams;

class SynchronousCompositorProxy : public ui::SynchronousInputHandler,
                                   public SynchronousLayerTreeFrameSinkClient,
                                   public mojom::SynchronousCompositor {
 public:
  SynchronousCompositorProxy(
      ui::SynchronousInputHandlerProxy* input_handler_proxy);
  ~SynchronousCompositorProxy() override;

  void Init();

  // ui::SynchronousInputHandler overrides.
  void SetNeedsSynchronousAnimateInput() final;
  void UpdateRootLayerState(const gfx::ScrollOffset& total_scroll_offset,
                            const gfx::ScrollOffset& max_scroll_offset,
                            const gfx::SizeF& scrollable_size,
                            float page_scale_factor,
                            float min_page_scale_factor,
                            float max_page_scale_factor) final;

  // SynchronousLayerTreeFrameSinkClient overrides.
  void DidActivatePendingTree() final;
  void Invalidate() final;
  void SubmitCompositorFrame(uint32_t layer_tree_frame_sink_id,
                             viz::CompositorFrame frame) final;
  void SetNeedsBeginFrames(bool needs_begin_frames) final;
  void SinkDestroyed() final;

  void SetLayerTreeFrameSink(
      SynchronousLayerTreeFrameSink* layer_tree_frame_sink);
  void PopulateCommonParams(SyncCompositorCommonRendererParams* params);

  void SendSetNeedsBeginFramesIfNeeded();

  // mojom::SynchronousCompositor overrides.
  void ComputeScroll(base::TimeTicks animation_time) final;
  void DemandDrawHwAsync(
      const SyncCompositorDemandDrawHwParams& draw_params) final;
  void DemandDrawHw(const SyncCompositorDemandDrawHwParams& params,
                    DemandDrawHwCallback callback) final;
  void SetSharedMemory(const SyncCompositorSetSharedMemoryParams& params,
                       SetSharedMemoryCallback callback) final;
  void DemandDrawSw(const SyncCompositorDemandDrawSwParams& params,
                    DemandDrawSwCallback callback) final;
  void ZeroSharedMemory() final;
  void ZoomBy(float zoom_delta, const gfx::Point& anchor, ZoomByCallback) final;
  void SetMemoryPolicy(uint32_t bytes_limit) final;
  void ReclaimResources(
      uint32_t layer_tree_frame_sink_id,
      const std::vector<viz::ReturnedResource>& resources) final;
  void SetScroll(const gfx::ScrollOffset& total_scroll_offset) final;
  void BeginFrame(const viz::BeginFrameArgs& args) final;
  void SetBeginFrameSourcePaused(bool paused) final;

 protected:
  virtual void SendSetNeedsBeginFrames(bool needs_begin_frames) = 0;
  virtual void SendAsyncRendererStateIfNeeded() = 0;
  virtual void LayerTreeFrameSinkCreated() = 0;
  virtual void SendBeginFrameResponse(
      const content::SyncCompositorCommonRendererParams&) = 0;
  virtual void SendDemandDrawHwAsyncReply(
      const content::SyncCompositorCommonRendererParams&,
      uint32_t,
      base::Optional<viz::CompositorFrame>) = 0;

  DemandDrawHwCallback hardware_draw_reply_;
  DemandDrawSwCallback software_draw_reply_;
  ZoomByCallback zoom_by_reply_;
  SynchronousLayerTreeFrameSink* layer_tree_frame_sink_ = nullptr;
  bool begin_frame_paused_ = false;

 private:
  void DoDemandDrawSw(const SyncCompositorDemandDrawSwParams& params);

  struct SharedMemoryWithSize;

  ui::SynchronousInputHandlerProxy* const input_handler_proxy_;
  const bool use_in_process_zero_copy_software_draw_;

  bool compute_scroll_called_via_ipc_ = false;
  bool browser_needs_begin_frame_state_ = false;
  bool needs_begin_frame_for_frame_sink_ = false;
  bool needs_begin_frame_for_animate_input_ = false;

  // From browser.
  std::unique_ptr<SharedMemoryWithSize> software_draw_shm_;

  // To browser.
  uint32_t version_ = 0;
  gfx::ScrollOffset total_scroll_offset_;  // Modified by both.
  gfx::ScrollOffset max_scroll_offset_;
  gfx::SizeF scrollable_size_;
  float page_scale_factor_;
  float min_page_scale_factor_;
  float max_page_scale_factor_;
  bool need_animate_scroll_;
  uint32_t need_invalidate_count_;
  uint32_t did_activate_pending_tree_count_;

  DISALLOW_COPY_AND_ASSIGN(SynchronousCompositorProxy);
};

}  // namespace content

#endif  // CONTENT_RENDERER_ANDROID_SYNCHRONOUS_COMPOSITOR_PROXY_H_
