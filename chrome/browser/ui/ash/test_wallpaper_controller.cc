// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/test_wallpaper_controller.h"

TestWallpaperController::TestWallpaperController() : binding_(this) {}

TestWallpaperController::~TestWallpaperController() = default;

void TestWallpaperController::ClearCounts() {
  remove_user_wallpaper_count_ = 0;
}

ash::mojom::WallpaperControllerPtr
TestWallpaperController::CreateInterfacePtr() {
  ash::mojom::WallpaperControllerPtr ptr;
  binding_.Bind(mojo::MakeRequest(&ptr));
  return ptr;
}

void TestWallpaperController::Init(
    ash::mojom::WallpaperControllerClientPtr client,
    const base::FilePath& user_data_path,
    const base::FilePath& chromeos_wallpapers_path,
    const base::FilePath& chromeos_custom_wallpapers_path,
    bool is_device_wallpaper_policy_enforced) {
  was_client_set_ = true;
}

void TestWallpaperController::SetCustomWallpaper(
    ash::mojom::WallpaperUserInfoPtr user_info,
    const std::string& wallpaper_files_id,
    const std::string& file_name,
    wallpaper::WallpaperLayout layout,
    const SkBitmap& image,
    bool preview_mode) {
  set_custom_wallpaper_count_++;
}

void TestWallpaperController::SetOnlineWallpaper(
    ash::mojom::WallpaperUserInfoPtr user_info,
    const SkBitmap& image,
    const std::string& url,
    wallpaper::WallpaperLayout layout,
    bool preview_mode) {
  NOTIMPLEMENTED();
}

void TestWallpaperController::SetDefaultWallpaper(
    ash::mojom::WallpaperUserInfoPtr user_info,
    const std::string& wallpaper_files_id,
    bool show_wallpaper) {
  set_default_wallpaper_count_++;
}

void TestWallpaperController::SetCustomizedDefaultWallpaperPaths(
    const base::FilePath& customized_default_small_path,
    const base::FilePath& customized_default_large_path) {
  NOTIMPLEMENTED();
}

void TestWallpaperController::SetPolicyWallpaper(
    ash::mojom::WallpaperUserInfoPtr user_info,
    const std::string& wallpaper_files_id,
    const std::string& data) {
  NOTIMPLEMENTED();
}

void TestWallpaperController::SetDeviceWallpaperPolicyEnforced(bool enforced) {
  NOTIMPLEMENTED();
}

void TestWallpaperController::ConfirmPreviewWallpaper() {
  NOTIMPLEMENTED();
}

void TestWallpaperController::CancelPreviewWallpaper() {
  NOTIMPLEMENTED();
}

void TestWallpaperController::UpdateCustomWallpaperLayout(
    ash::mojom::WallpaperUserInfoPtr user_info,
    wallpaper::WallpaperLayout layout) {
  NOTIMPLEMENTED();
}

void TestWallpaperController::ShowUserWallpaper(
    ash::mojom::WallpaperUserInfoPtr user_info) {
  NOTIMPLEMENTED();
}

void TestWallpaperController::ShowSigninWallpaper() {
  NOTIMPLEMENTED();
}

void TestWallpaperController::RemoveUserWallpaper(
    ash::mojom::WallpaperUserInfoPtr user_info,
    const std::string& wallpaper_files_id) {
  remove_user_wallpaper_count_++;
}

void TestWallpaperController::RemovePolicyWallpaper(
    ash::mojom::WallpaperUserInfoPtr user_info,
    const std::string& wallpaper_files_id) {
  NOTIMPLEMENTED();
}

void TestWallpaperController::SetAnimationDuration(
    base::TimeDelta animation_duration) {
  NOTIMPLEMENTED();
}

void TestWallpaperController::OpenWallpaperPickerIfAllowed() {
  NOTIMPLEMENTED();
}

void TestWallpaperController::AddObserver(
    ash::mojom::WallpaperObserverAssociatedPtrInfo observer) {
  NOTIMPLEMENTED();
}

void TestWallpaperController::GetWallpaperColors(
    GetWallpaperColorsCallback callback) {
  NOTIMPLEMENTED();
}

void TestWallpaperController::IsActiveUserWallpaperControlledByPolicy(
    ash::mojom::WallpaperController::
        IsActiveUserWallpaperControlledByPolicyCallback callback) {
  NOTIMPLEMENTED();
}

void TestWallpaperController::ShouldShowWallpaperSetting(
    ash::mojom::WallpaperController::ShouldShowWallpaperSettingCallback
        callback) {
  NOTIMPLEMENTED();
}
