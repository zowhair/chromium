// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/extensions/hosted_app_browser_controller.h"

#include "base/metrics/histogram_macros.h"
#include "chrome/browser/engagement/site_engagement_service.h"
#include "chrome/browser/extensions/tab_helper.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ssl/security_state_tab_helper.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/location_bar/location_bar.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/web_applications/web_app.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/extensions/api/url_handlers/url_handlers_parser.h"
#include "chrome/common/extensions/manifest_handlers/app_launch_info.h"
#include "chrome/common/extensions/manifest_handlers/app_theme_color_info.h"
#include "components/security_state/core/security_state.h"
#include "components/url_formatter/url_formatter.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/extension.h"
#include "ui/gfx/favicon_size.h"
#include "ui/gfx/image/image_skia.h"
#include "url/gurl.h"

namespace extensions {

namespace {

// Returns true if |page_url| is both secure (https) and on the same origin as
// |app_url|. Note that even if |app_url| is http, this still returns true as
// long as |page_url| is https.
bool IsSameOriginAndSecure(const GURL& app_url, const GURL& page_url) {
  const std::string www("www.");
  return page_url.scheme_piece() == url::kHttpsScheme &&
         (app_url.host_piece() == page_url.host_piece() ||
          www + app_url.host() == page_url.host_piece()) &&
         app_url.port() == page_url.port();
}

// Gets the icon to use if the extension app icon is not available.
gfx::ImageSkia GetFallbackAppIcon(Browser* browser) {
  gfx::ImageSkia page_icon = browser->GetCurrentPageIcon().AsImageSkia();
  if (!page_icon.isNull())
    return page_icon;

  // The extension icon may be loading still. Return a transparent icon rather
  // than using a placeholder to avoid flickering.
  SkBitmap bitmap;
  bitmap.allocN32Pixels(gfx::kFaviconSize, gfx::kFaviconSize);
  bitmap.eraseColor(SK_ColorTRANSPARENT);
  return gfx::ImageSkia::CreateFrom1xBitmap(bitmap);
}

}  // namespace

const char kPwaWindowEngagementTypeHistogram[] =
    "Webapp.Engagement.EngagementType";

// static
bool HostedAppBrowserController::IsForHostedApp(const Browser* browser) {
  const std::string extension_id =
      web_app::GetExtensionIdFromApplicationName(browser->app_name());
  const Extension* extension =
      ExtensionRegistry::Get(browser->profile())->GetExtensionById(
          extension_id, ExtensionRegistry::EVERYTHING);
  return extension && extension->is_hosted_app();
}

// static
bool HostedAppBrowserController::IsForExperimentalHostedAppBrowser(
    const Browser* browser) {
  return base::FeatureList::IsEnabled(features::kDesktopPWAWindowing) &&
         IsForHostedApp(browser);
}

// static
base::string16 HostedAppBrowserController::FormatUrlOrigin(const GURL& url) {
  return url_formatter::FormatUrl(
      url.GetOrigin(),
      url_formatter::kFormatUrlOmitUsernamePassword |
          url_formatter::kFormatUrlOmitHTTPS |
          url_formatter::kFormatUrlOmitHTTP |
          url_formatter::kFormatUrlOmitTrailingSlashOnBareHostname |
          url_formatter::kFormatUrlOmitTrivialSubdomains,
      net::UnescapeRule::SPACES, nullptr, nullptr, nullptr);
}

HostedAppBrowserController::HostedAppBrowserController(Browser* browser)
    : SiteEngagementObserver(SiteEngagementService::Get(browser->profile())),
      browser_(browser),
      extension_id_(
          web_app::GetExtensionIdFromApplicationName(browser->app_name())) {}

HostedAppBrowserController::~HostedAppBrowserController() {}

bool HostedAppBrowserController::IsForInstalledPwa(
    content::WebContents* web_contents) const {
  if (!web_contents ||
      web_contents != browser_->tab_strip_model()->GetActiveWebContents()) {
    return false;
  }

  if (!browser_->is_app())
    return false;

  // If a bookmark app has a URL handler, then it is a PWA.
  // TODO(https://crbug.com/774918): Replace once there is a more explicit
  // indicator of a Bookmark App for an installable website.
  if (extensions::UrlHandlers::GetUrlHandlers(GetExtension()) == nullptr)
    return false;

  return true;
}

bool HostedAppBrowserController::ShouldShowLocationBar() const {
  const Extension* extension = GetExtension();

  const content::WebContents* web_contents =
      browser_->tab_strip_model()->GetActiveWebContents();

  // Default to not showing the location bar if either |extension| or
  // |web_contents| are null. |extension| is null for the dev tools.
  if (!extension || !web_contents)
    return false;

  if (!extension->is_hosted_app())
    return false;

  // Don't show a location bar until a navigation has occurred.
  if (web_contents->GetLastCommittedURL().is_empty())
    return false;

  const SecurityStateTabHelper* helper =
      SecurityStateTabHelper::FromWebContents(web_contents);
  if (helper) {
    security_state::SecurityInfo security_info;
    helper->GetSecurityInfo(&security_info);
    if (security_info.security_level == security_state::DANGEROUS)
      return true;
  }

  GURL launch_url = AppLaunchInfo::GetLaunchWebURL(extension);
  return !IsSameOriginAndSecure(launch_url, web_contents->GetVisibleURL()) ||
         !IsSameOriginAndSecure(launch_url,
                                web_contents->GetLastCommittedURL());
}

void HostedAppBrowserController::UpdateLocationBarVisibility(
    bool animate) const {
  browser_->window()->GetLocationBar()->UpdateLocationBarVisibility(
      ShouldShowLocationBar(), animate);
}

gfx::ImageSkia HostedAppBrowserController::GetWindowAppIcon() const {
  // TODO(calamity): Use the app name to retrieve the app icon without using the
  // extensions tab helper to make icon load more immediate.
  content::WebContents* contents =
      browser_->tab_strip_model()->GetActiveWebContents();
  if (!contents)
    return GetFallbackAppIcon(browser_);

  extensions::TabHelper* extensions_tab_helper =
      extensions::TabHelper::FromWebContents(contents);
  if (!extensions_tab_helper)
    return GetFallbackAppIcon(browser_);

  const SkBitmap* icon_bitmap = extensions_tab_helper->GetExtensionAppIcon();
  if (!icon_bitmap)
    return GetFallbackAppIcon(browser_);

  return gfx::ImageSkia::CreateFrom1xBitmap(*icon_bitmap);
}

gfx::ImageSkia HostedAppBrowserController::GetWindowIcon() const {
  if (IsForExperimentalHostedAppBrowser(browser_))
    return GetWindowAppIcon();

  return browser_->GetCurrentPageIcon().AsImageSkia();
}

base::Optional<SkColor> HostedAppBrowserController::GetThemeColor() const {
  ExtensionRegistry* registry = ExtensionRegistry::Get(browser_->profile());
  const Extension* extension =
      registry->GetExtensionById(extension_id_, ExtensionRegistry::EVERYTHING);
  return AppThemeColorInfo::GetThemeColor(extension);
}

base::string16 HostedAppBrowserController::GetTitle() const {
  content::WebContents* web_contents =
      browser_->tab_strip_model()->GetActiveWebContents();
  if (!web_contents)
    return base::string16();

  content::NavigationEntry* entry =
      web_contents->GetController().GetVisibleEntry();
  return entry ? entry->GetTitle() : base::string16();
}

const Extension* HostedAppBrowserController::GetExtension() const {
  return ExtensionRegistry::Get(browser_->profile())
      ->GetExtensionById(extension_id_, ExtensionRegistry::EVERYTHING);
}

std::string HostedAppBrowserController::GetAppShortName() const {
  return GetExtension()->short_name();
}

base::string16 HostedAppBrowserController::GetFormattedUrlOrigin() const {
  return FormatUrlOrigin(AppLaunchInfo::GetLaunchWebURL(GetExtension()));
}

void HostedAppBrowserController::OnEngagementEvent(
    content::WebContents* web_contents,
    const GURL& /*url*/,
    double /*score*/,
    SiteEngagementService::EngagementType type) {
  if (!IsForInstalledPwa(web_contents))
    return;

  UMA_HISTOGRAM_ENUMERATION(kPwaWindowEngagementTypeHistogram, type,
                            SiteEngagementService::ENGAGEMENT_LAST);
}

}  // namespace extensions
