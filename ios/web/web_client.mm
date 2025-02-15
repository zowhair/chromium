// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/public/web_client.h"

#import <Foundation/Foundation.h>

#include "ios/web/public/app/web_main_parts.h"
#include "ios/web/public/features.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace web {

static WebClient* g_client;

void SetWebClient(WebClient* client) {
  g_client = client;
}

WebClient* GetWebClient() {
  return g_client;
}

WebClient::Schemes::Schemes() = default;
WebClient::Schemes::~Schemes() = default;

WebClient::WebClient() {}

WebClient::~WebClient() {}

std::unique_ptr<WebMainParts> WebClient::CreateWebMainParts() {
  return nullptr;
}

std::string WebClient::GetApplicationLocale() const {
  return "en-US";
}

bool WebClient::IsAppSpecificURL(const GURL& url) const {
  return false;
}

base::string16 WebClient::GetPluginNotSupportedText() const {
  return base::string16();
}

std::string WebClient::GetProduct() const {
  return std::string();
}

std::string WebClient::GetUserAgent(UserAgentType type) const {
  return std::string();
}

base::string16 WebClient::GetLocalizedString(int message_id) const {
  return base::string16();
}

base::StringPiece WebClient::GetDataResource(
    int resource_id,
    ui::ScaleFactor scale_factor) const {
  return base::StringPiece();
}

base::RefCountedMemory* WebClient::GetDataResourceBytes(int resource_id) const {
  return nullptr;
}

NSString* WebClient::GetDocumentStartScriptForMainFrame(
    BrowserState* browser_state) const {
  return @"";
}

std::unique_ptr<base::Value> WebClient::GetServiceManifestOverlay(
    base::StringPiece name) {
  return nullptr;
}

void WebClient::AllowCertificateError(
    WebState* web_state,
    int cert_error,
    const net::SSLInfo& ssl_info,
    const GURL& request_url,
    bool overridable,
    const base::Callback<void(bool)>& callback) {
  callback.Run(false);
}

bool WebClient::IsSlimNavigationManagerEnabled() const {
  if (@available(iOS 11.3, *)) {
    // Starting iOS 11.3, pushState and replaceState are not allowed in file://
    // scheme which the new navigation manager relies on. So this excludes the
    // newer iOS versions until this bug is fixed.
    // TODO(crbug.com/814803): Remove this workaround.
    return false;
  } else {
    return base::FeatureList::IsEnabled(web::features::kSlimNavigationManager);
  }
}

}  // namespace web
