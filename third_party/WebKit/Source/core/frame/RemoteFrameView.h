// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef RemoteFrameView_h
#define RemoteFrameView_h

#include "core/dom/DocumentLifecycle.h"
#include "core/frame/FrameView.h"
#include "core/frame/LocalFrameView.h"
#include "platform/geometry/IntRect.h"
#include "platform/heap/Handle.h"
#include "public/platform/WebCanvas.h"

namespace blink {

class CullRect;
class ElementVisibilityObserver;
class GraphicsContext;
class RemoteFrame;

class RemoteFrameView final : public GarbageCollectedFinalized<RemoteFrameView>,
                              public FrameView {
  USING_GARBAGE_COLLECTED_MIXIN(RemoteFrameView);

 public:
  static RemoteFrameView* Create(RemoteFrame*);

  ~RemoteFrameView() override;

  void AttachToLayout() override;
  void DetachFromLayout() override;
  bool IsAttached() const override { return is_attached_; }

  RemoteFrame& GetFrame() const {
    DCHECK(remote_frame_);
    return *remote_frame_;
  }

  void Dispose() override;
  // Override to notify remote frame that its viewport size has changed.
  void FrameRectsChanged() override;
  void InvalidateRect(const IntRect&);
  void SetFrameRect(const IntRect&) override;
  IntRect FrameRect() const override;
  void Paint(GraphicsContext&,
             const GlobalPaintFlags,
             const CullRect&,
             const IntSize& paint_offset = IntSize()) const override;
  void UpdateGeometry() override;
  void Hide() override;
  void Show() override;
  void SetParentVisible(bool) override;

  void UpdateViewportIntersectionsForSubtree(
      DocumentLifecycle::LifecycleState) override;

  bool GetIntrinsicSizingInfo(IntrinsicSizingInfo&) const override;

  void SetIntrinsicSizeInfo(const IntrinsicSizingInfo& size_info);
  bool HasIntrinsicSizingInfo() const override;

  uint32_t Print(const IntRect&, WebCanvas*) const;

  virtual void Trace(blink::Visitor*);

 private:
  explicit RemoteFrameView(RemoteFrame*);

  LocalFrameView* ParentFrameView() const;
  IntRect ConvertFromRootFrame(const IntRect&) const;

  void UpdateRenderThrottlingStatus(bool hidden, bool subtree_throttled);
  bool CanThrottleRendering() const;
  void SetupRenderThrottling();

  // The properties and handling of the cycle between RemoteFrame
  // and its RemoteFrameView corresponds to that between LocalFrame
  // and LocalFrameView. Please see the LocalFrameView::frame_ comment for
  // details.
  Member<RemoteFrame> remote_frame_;
  bool is_attached_;
  IntRect last_viewport_intersection_;
  IntRect frame_rect_;
  bool self_visible_;
  bool parent_visible_;

  Member<ElementVisibilityObserver> visibility_observer_;
  bool subtree_throttled_ = false;
  bool hidden_for_throttling_ = false;
  IntrinsicSizingInfo intrinsic_sizing_info_;
  bool has_intrinsic_sizing_info_ = false;
};

}  // namespace blink

#endif  // RemoteFrameView_h
