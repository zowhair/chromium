// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/search/arc/icon_decode_request.h"

#include "ash/app_list/model/search/search_result.h"
#include "chrome/grit/component_extension_resources.h"
#include "content/public/browser/browser_thread.h"
#include "ui/app_list/app_list_constants.h"
#include "ui/gfx/image/image_skia_operations.h"
#include "ui/gfx/image/image_skia_rep.h"
#include "ui/gfx/image/image_skia_source.h"

using content::BrowserThread;

namespace app_list {

namespace {

class IconSource : public gfx::ImageSkiaSource {
 public:
  IconSource(const SkBitmap& decoded_bitmap, int resource_size_in_dip);
  explicit IconSource(int resource_size_in_dip);
  ~IconSource() override = default;

  void SetDecodedImage(const SkBitmap& decoded_bitmap);

 private:
  gfx::ImageSkiaRep GetImageForScale(float scale) override;

  const int resource_size_in_dip_;
  gfx::ImageSkia decoded_icon_;

  DISALLOW_COPY_AND_ASSIGN(IconSource);
};

IconSource::IconSource(int resource_size_in_dip)
    : resource_size_in_dip_(resource_size_in_dip) {}

void IconSource::SetDecodedImage(const SkBitmap& decoded_bitmap) {
  decoded_icon_.AddRepresentation(gfx::ImageSkiaRep(
      decoded_bitmap, ui::GetScaleForScaleFactor(ui::SCALE_FACTOR_100P)));
}

gfx::ImageSkiaRep IconSource::GetImageForScale(float scale) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // We use the icon if it was decoded successfully, otherwise use the default
  // ARC icon.
  const gfx::ImageSkia* icon_to_scale;
  if (decoded_icon_.isNull()) {
    int resource_id =
        scale >= 1.5f ? IDR_ARC_SUPPORT_ICON_96 : IDR_ARC_SUPPORT_ICON_48;
    icon_to_scale =
        ui::ResourceBundle::GetSharedInstance().GetImageSkiaNamed(resource_id);
  } else {
    icon_to_scale = &decoded_icon_;
  }
  DCHECK(icon_to_scale);

  gfx::ImageSkia resized_image = gfx::ImageSkiaOperations::CreateResizedImage(
      *icon_to_scale, skia::ImageOperations::RESIZE_BEST,
      gfx::Size(resource_size_in_dip_, resource_size_in_dip_));
  return resized_image.GetRepresentation(scale);
}

}  // namespace

IconDecodeRequest::IconDecodeRequest(SetIconCallback set_icon_callback)
    : set_icon_callback_(std::move(set_icon_callback)) {}

IconDecodeRequest::~IconDecodeRequest() = default;

void IconDecodeRequest::OnImageDecoded(const SkBitmap& bitmap) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  const gfx::Size resource_size(kGridIconDimension, kGridIconDimension);
  auto icon_source = std::make_unique<IconSource>(kGridIconDimension);
  icon_source->SetDecodedImage(bitmap);
  const gfx::ImageSkia icon =
      gfx::ImageSkia(std::move(icon_source), resource_size);
  icon.EnsureRepsForSupportedScales();

  std::move(set_icon_callback_).Run(icon);
}

void IconDecodeRequest::OnDecodeImageFailed() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DLOG(ERROR) << "Failed to decode an icon image.";

  const gfx::Size resource_size(kGridIconDimension, kGridIconDimension);
  auto icon_source = std::make_unique<IconSource>(kGridIconDimension);
  const gfx::ImageSkia icon =
      gfx::ImageSkia(std::move(icon_source), resource_size);
  icon.EnsureRepsForSupportedScales();

  std::move(set_icon_callback_).Run(icon);
}

}  // namespace app_list
