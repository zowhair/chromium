// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/guest_view/extensions_guest_view_message_filter.h"

#include "base/guid.h"
#include "base/macros.h"
#include "components/guest_view/browser/guest_view_base.h"
#include "components/guest_view/browser/guest_view_manager.h"
#include "components/guest_view/browser/guest_view_manager_delegate.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/stream_info.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/api/extensions_api_client.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/guest_view/mime_handler_view/mime_handler_stream_manager.h"
#include "extensions/browser/guest_view/mime_handler_view/mime_handler_view_constants.h"
#include "extensions/browser/guest_view/mime_handler_view/mime_handler_view_guest.h"
#include "extensions/browser/guest_view/web_view/web_view_content_script_manager.h"
#include "extensions/browser/guest_view/web_view/web_view_guest.h"
#include "extensions/browser/guest_view/web_view/web_view_renderer_state.h"
#include "extensions/common/guest_view/extensions_guest_view_messages.h"
#include "extensions/common/manifest_handlers/mime_types_handler.h"
#include "ipc/ipc_message_macros.h"

using content::BrowserContext;
using content::BrowserThread;
using content::RenderFrameHost;
using content::WebContents;
using guest_view::GuestViewManager;
using guest_view::GuestViewManagerDelegate;
using guest_view::GuestViewMessageFilter;

namespace extensions {

const uint32_t ExtensionsGuestViewMessageFilter::kFilteredMessageClasses[] = {
    GuestViewMsgStart, ExtensionsGuestViewMsgStart};

ExtensionsGuestViewMessageFilter::ExtensionsGuestViewMessageFilter(
    int render_process_id,
    BrowserContext* context)
    : GuestViewMessageFilter(kFilteredMessageClasses,
                             arraysize(kFilteredMessageClasses),
                             render_process_id,
                             context),
      content::BrowserAssociatedInterface<mojom::GuestView>(this, this) {}

ExtensionsGuestViewMessageFilter::~ExtensionsGuestViewMessageFilter() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
}

void ExtensionsGuestViewMessageFilter::OverrideThreadForMessage(
    const IPC::Message& message,
    BrowserThread::ID* thread) {
  switch (message.type()) {
    case ExtensionsGuestViewHostMsg_CreateMimeHandlerViewGuest::ID:
    case ExtensionsGuestViewHostMsg_ResizeGuest::ID:
      *thread = BrowserThread::UI;
      break;
    default:
      GuestViewMessageFilter::OverrideThreadForMessage(message, thread);
  }
}

bool ExtensionsGuestViewMessageFilter::OnMessageReceived(
    const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(ExtensionsGuestViewMessageFilter, message)
    IPC_MESSAGE_HANDLER(ExtensionsGuestViewHostMsg_CanExecuteContentScriptSync,
                        OnCanExecuteContentScript)
    IPC_MESSAGE_HANDLER(ExtensionsGuestViewHostMsg_CreateMimeHandlerViewGuest,
                        OnCreateMimeHandlerViewGuest)
    IPC_MESSAGE_HANDLER(ExtensionsGuestViewHostMsg_ResizeGuest, OnResizeGuest)
    IPC_MESSAGE_UNHANDLED(
        handled = GuestViewMessageFilter::OnMessageReceived(message))
  IPC_END_MESSAGE_MAP()
  return handled;
}

GuestViewManager* ExtensionsGuestViewMessageFilter::
    GetOrCreateGuestViewManager() {
  auto* manager = GuestViewManager::FromBrowserContext(browser_context_);
  if (!manager) {
    manager = GuestViewManager::CreateWithDelegate(
        browser_context_,
        ExtensionsAPIClient::Get()->CreateGuestViewManagerDelegate(
            browser_context_));
  }
  return manager;
}

void ExtensionsGuestViewMessageFilter::OnCanExecuteContentScript(
    int render_view_id,
    int script_id,
    bool* allowed) {
  WebViewRendererState::WebViewInfo info;
  WebViewRendererState::GetInstance()->GetInfo(render_process_id_,
                                               render_view_id, &info);

  *allowed =
      info.content_script_ids.find(script_id) != info.content_script_ids.end();
}

void ExtensionsGuestViewMessageFilter::OnCreateMimeHandlerViewGuest(
    int render_frame_id,
    const std::string& view_id,
    int element_instance_id,
    const gfx::Size& element_size) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  auto* manager = GetOrCreateGuestViewManager();

  auto* rfh = RenderFrameHost::FromID(render_process_id_, render_frame_id);
  auto* embedder_web_contents = WebContents::FromRenderFrameHost(rfh);
  if (!embedder_web_contents)
    return;

  GuestViewManager::WebContentsCreatedCallback callback =
      base::Bind(
          &ExtensionsGuestViewMessageFilter::
              MimeHandlerViewGuestCreatedCallback,
          this,
          element_instance_id,
          render_process_id_,
          render_frame_id,
          element_size);

  base::DictionaryValue create_params;
  create_params.SetString(mime_handler_view::kViewId, view_id);
  create_params.SetInteger(guest_view::kElementWidth, element_size.width());
  create_params.SetInteger(guest_view::kElementHeight, element_size.height());
  manager->CreateGuest(MimeHandlerViewGuest::Type,
                       embedder_web_contents,
                       create_params,
                       callback);
}

void ExtensionsGuestViewMessageFilter::OnResizeGuest(
    int render_frame_id,
    int element_instance_id,
    const gfx::Size& new_size) {
  // We should have a GuestViewManager at this point. If we don't then the
  // embedder is misbehaving.
  auto* manager = GetGuestViewManagerOrKill();
  if (!manager)
    return;

  auto* guest_web_contents =
      manager->GetGuestByInstanceID(render_process_id_, element_instance_id);
  auto* mhvg = MimeHandlerViewGuest::FromWebContents(guest_web_contents);
  if (!mhvg)
    return;

  guest_view::SetSizeParams set_size_params;
  set_size_params.enable_auto_size.reset(new bool(false));
  set_size_params.normal_size.reset(new gfx::Size(new_size));
  mhvg->SetSize(set_size_params);
}

void ExtensionsGuestViewMessageFilter::CreateEmbeddedMimeHandlerViewGuest(
    int32_t render_frame_id,
    int32_t tab_id,
    const GURL& original_url,
    int32_t element_instance_id,
    const gfx::Size& element_size,
    content::mojom::TransferrableURLLoaderPtr transferrable_url_loader) {
  if (!content::BrowserThread::CurrentlyOn(content::BrowserThread::UI)) {
    content::BrowserThread::PostTask(
        content::BrowserThread::UI, FROM_HERE,
        base::BindOnce(&ExtensionsGuestViewMessageFilter::
                           CreateEmbeddedMimeHandlerViewGuest,
                       this, render_frame_id, tab_id, original_url,
                       element_instance_id, element_size,
                       base::Passed(&transferrable_url_loader)));
    return;
  }

  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(
          content::RenderFrameHost::FromID(render_process_id_,
                                           render_frame_id));
  if (!web_contents)
    return;

  auto* browser_context = web_contents->GetBrowserContext();
  std::string extension_id = transferrable_url_loader->url.host();
  const Extension* extension = ExtensionRegistry::Get(browser_context)
                                   ->enabled_extensions()
                                   .GetByID(extension_id);
  if (!extension)
    return;

  MimeTypesHandler* handler = MimeTypesHandler::GetHandler(extension);
  if (!handler || !handler->HasPlugin()) {
    NOTREACHED();
    return;
  }

  GURL handler_url(Extension::GetBaseURLFromExtensionId(extension_id).spec() +
                   handler->handler_url());

  std::string view_id = base::GenerateGUID();
  std::unique_ptr<StreamContainer> stream_container(new StreamContainer(
      nullptr, tab_id, true /* embedded */, handler_url, extension_id,
      std::move(transferrable_url_loader), original_url));
  MimeHandlerStreamManager::Get(browser_context)
      ->AddStream(view_id, std::move(stream_container),
                  -1 /* frame_tree_node_id*/, render_process_id_,
                  render_frame_id);

  OnCreateMimeHandlerViewGuest(render_frame_id, view_id, element_instance_id,
                               element_size);
}

void ExtensionsGuestViewMessageFilter::MimeHandlerViewGuestCreatedCallback(
    int element_instance_id,
    int embedder_render_process_id,
    int embedder_render_frame_id,
    const gfx::Size& element_size,
    WebContents* web_contents) {
  auto* guest_view = MimeHandlerViewGuest::FromWebContents(web_contents);
  if (!guest_view)
    return;

  int guest_instance_id = guest_view->guest_instance_id();
  auto* rfh = RenderFrameHost::FromID(embedder_render_process_id,
                                      embedder_render_frame_id);
  if (!rfh)
    return;

  guest_view->SetEmbedderFrame(embedder_render_process_id,
                               embedder_render_frame_id);

  base::DictionaryValue attach_params;
  attach_params.SetInteger(guest_view::kElementWidth, element_size.width());
  attach_params.SetInteger(guest_view::kElementHeight, element_size.height());
  auto* manager = GuestViewManager::FromBrowserContext(browser_context_);
  CHECK(manager);
  manager->AttachGuest(embedder_render_process_id, element_instance_id,
                       guest_instance_id, attach_params);

  rfh->Send(
      new ExtensionsGuestViewMsg_CreateMimeHandlerViewGuestACK(
          element_instance_id));
}

}  // namespace extensions
