// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/client/hit_test_data_provider_draw_quad.h"

#include <memory>

#include "base/bind.h"
#include "base/memory/scoped_refptr.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread.h"
#include "cc/test/fake_layer_tree_frame_sink_client.h"
#include "components/viz/client/local_surface_id_provider.h"
#include "components/viz/common/quads/surface_draw_quad.h"
#include "components/viz/test/compositor_frame_helpers.h"
#include "components/viz/test/test_context_provider.h"
#include "components/viz/test/test_gpu_memory_buffer_manager.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "services/viz/public/interfaces/compositing/compositor_frame_sink.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace viz {

namespace {

SurfaceId CreateChildSurfaceId(uint32_t id) {
  LocalSurfaceId child_local_surface_id(id, base::UnguessableToken::Create());
  FrameSinkId frame_sink_id(id, 0);
  SurfaceId child_surface_id(frame_sink_id, child_local_surface_id);
  return child_surface_id;
}

std::unique_ptr<RenderPass> CreateRenderPassWithChildSurface(
    const SurfaceId& child_surface_id,
    const gfx::Rect& rect,
    const gfx::Rect& child_rect,
    const gfx::Transform& render_pass_transform,
    const gfx::Transform& shared_state_transform) {
  auto pass = RenderPass::Create();
  pass->SetNew(1, rect, rect, render_pass_transform);

  auto* shared_state = pass->CreateAndAppendSharedQuadState();
  shared_state->SetAll(shared_state_transform, rect, rect, rect, false, false,
                       1, SkBlendMode::kSrcOver, 0);

  auto* surface_quad = pass->CreateAndAppendDrawQuad<SurfaceDrawQuad>();
  surface_quad->SetNew(pass->shared_quad_state_list.back(), child_rect,
                       child_rect, child_surface_id, base::nullopt,
                       SK_ColorWHITE, false);

  return pass;
}

}  // namespace

// Test to ensure that hit test data is created correctly from CompositorFrame
// and its RenderPassList. kHitTestAsk is only set for OOPIFs.
TEST(HitTestDataProviderDrawQuad, HitTestDataRenderer) {
  std::unique_ptr<HitTestDataProvider> hit_test_data_provider =
      std::make_unique<HitTestDataProviderDrawQuad>(
          true /* should_ask_for_child_region */);

  constexpr gfx::Rect kFrameRect(0, 0, 1024, 768);

  // Ensure that a CompositorFrame without a child surface sets kHitTestMine.
  CompositorFrame compositor_frame =
      CompositorFrameBuilder().AddRenderPass(kFrameRect, kFrameRect).Build();
  mojom::HitTestRegionListPtr hit_test_region_list =
      hit_test_data_provider->GetHitTestData(compositor_frame);

  EXPECT_EQ(mojom::kHitTestMouse | mojom::kHitTestTouch | mojom::kHitTestMine,
            hit_test_region_list->flags);
  EXPECT_EQ(kFrameRect, hit_test_region_list->bounds);
  EXPECT_FALSE(hit_test_region_list->regions.size());

  // Ensure that a CompositorFrame with a child surface only set kHitTestAsk
  // for its child surface.
  SurfaceId child_surface_id = CreateChildSurfaceId(2);
  gfx::Rect child_rect(200, 100);
  gfx::Transform render_pass_transform;
  render_pass_transform.Translate(-50, -100);
  render_pass_transform.Skew(2, 3);
  gfx::Transform shared_state_transform;
  shared_state_transform.Translate(-200, -100);
  auto pass = CreateRenderPassWithChildSurface(
      child_surface_id, kFrameRect, child_rect, render_pass_transform,
      shared_state_transform);
  compositor_frame =
      CompositorFrameBuilder().AddRenderPass(std::move(pass)).Build();
  hit_test_region_list =
      hit_test_data_provider->GetHitTestData(compositor_frame);

  EXPECT_EQ(mojom::kHitTestMouse | mojom::kHitTestTouch | mojom::kHitTestMine,
            hit_test_region_list->flags);
  EXPECT_EQ(kFrameRect, hit_test_region_list->bounds);
  EXPECT_EQ(1u, hit_test_region_list->regions.size());
  EXPECT_EQ(child_surface_id.frame_sink_id(),
            hit_test_region_list->regions[0]->frame_sink_id);
  EXPECT_EQ(child_surface_id.local_surface_id(),
            hit_test_region_list->regions[0]->local_surface_id);
  EXPECT_EQ(mojom::kHitTestMouse | mojom::kHitTestTouch |
                mojom::kHitTestChildSurface | mojom::kHitTestAsk,
            hit_test_region_list->regions[0]->flags);
  EXPECT_EQ(child_rect, hit_test_region_list->regions[0]->rect);
  gfx::Transform render_pass_transform_inverse;
  EXPECT_TRUE(render_pass_transform.GetInverse(&render_pass_transform_inverse));
  gfx::Transform shared_state_transform_inverse;
  EXPECT_TRUE(
      shared_state_transform.GetInverse(&shared_state_transform_inverse));
  EXPECT_EQ(shared_state_transform_inverse * render_pass_transform_inverse,
            hit_test_region_list->regions[0]->transform);
}

// Test to ensure that we skip regions with a non-invertible transform when
// preparing hit-test data.
TEST(HitTestDataProviderDrawQuad, HitTestDataInvertibleTransform) {
  std::unique_ptr<HitTestDataProvider> hit_test_data_provider =
      std::make_unique<HitTestDataProviderDrawQuad>(
          true /* should_ask_for_child_region */);

  constexpr gfx::Rect kFrameRect(0, 0, 1024, 768);
  gfx::Rect child_rect(200, 100);

  // A degenerate matrix of all zeros is not invertible.
  gfx::Transform not_invertible_transform;
  not_invertible_transform.matrix().set(0, 0, 0.f);
  not_invertible_transform.matrix().set(1, 1, 0.f);
  not_invertible_transform.matrix().set(2, 2, 0.f);
  not_invertible_transform.matrix().set(3, 3, 0.f);

  gfx::Transform invertible_transform;
  invertible_transform.Translate(-200, -100);

  RenderPassList pass_list;

  // A render pass that has non-invertible transform.
  SurfaceId child_surface_id1 = CreateChildSurfaceId(2);
  auto pass1 = CreateRenderPassWithChildSurface(
      child_surface_id1, kFrameRect, child_rect, not_invertible_transform,
      invertible_transform);
  pass_list.push_back(std::move(pass1));

  // A render pass with a draw quad that has non-invertible transform.
  SurfaceId child_surface_id2 = CreateChildSurfaceId(3);
  auto pass2 = CreateRenderPassWithChildSurface(
      child_surface_id2, kFrameRect, child_rect, invertible_transform,
      not_invertible_transform);
  pass_list.push_back(std::move(pass2));

  // A render pass and its draw quad both have invertible transforms
  SurfaceId child_surface_id3 = CreateChildSurfaceId(4);
  auto pass3 = CreateRenderPassWithChildSurface(
      child_surface_id3, kFrameRect, child_rect, invertible_transform,
      invertible_transform);
  pass_list.push_back(std::move(pass3));

  auto compositor_frame =
      CompositorFrameBuilder().SetRenderPassList(std::move(pass_list)).Build();
  mojom::HitTestRegionListPtr hit_test_region_list =
      hit_test_data_provider->GetHitTestData(compositor_frame);

  // Only pass3 should have a hit-test region that corresponds to
  // child_surface_id3.
  EXPECT_EQ(1u, hit_test_region_list->regions.size());
  EXPECT_EQ(child_surface_id3.frame_sink_id(),
            hit_test_region_list->regions[0]->frame_sink_id);
  EXPECT_EQ(child_surface_id3.local_surface_id(),
            hit_test_region_list->regions[0]->local_surface_id);
}

}  // namespace viz
