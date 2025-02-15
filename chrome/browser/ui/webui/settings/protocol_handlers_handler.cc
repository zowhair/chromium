// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/settings/protocol_handlers_handler.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/custom_handlers/protocol_handler_registry_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/url_constants.h"
#include "components/google/core/browser/google_util.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_ui.h"

namespace settings {

ProtocolHandlersHandler::ProtocolHandlersHandler() {
}

ProtocolHandlersHandler::~ProtocolHandlersHandler() {
}

void ProtocolHandlersHandler::OnJavascriptAllowed() {
  notification_registrar_.Add(
      this, chrome::NOTIFICATION_PROTOCOL_HANDLER_REGISTRY_CHANGED,
      content::Source<Profile>(Profile::FromWebUI(web_ui())));
}

void ProtocolHandlersHandler::OnJavascriptDisallowed() {
  notification_registrar_.RemoveAll();
}

void ProtocolHandlersHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback("observeProtocolHandlers",
      base::Bind(&ProtocolHandlersHandler::HandleObserveProtocolHandlers,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback("observeProtocolHandlersEnabledState",
      base::Bind(
          &ProtocolHandlersHandler::HandleObserveProtocolHandlersEnabledState,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback("removeHandler",
      base::Bind(&ProtocolHandlersHandler::HandleRemoveHandler,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback("setHandlersEnabled",
      base::Bind(&ProtocolHandlersHandler::HandleSetHandlersEnabled,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback("setDefault",
      base::Bind(&ProtocolHandlersHandler::HandleSetDefault,
                 base::Unretained(this)));
}

ProtocolHandlerRegistry* ProtocolHandlersHandler::GetProtocolHandlerRegistry() {
  return ProtocolHandlerRegistryFactory::GetForBrowserContext(
      Profile::FromWebUI(web_ui()));
}

static void GetHandlersAsListValue(
    const ProtocolHandlerRegistry& registry,
    const ProtocolHandlerRegistry::ProtocolHandlerList& handlers,
    base::ListValue* handler_list) {
  ProtocolHandlerRegistry::ProtocolHandlerList::const_iterator handler;
  for (handler = handlers.begin(); handler != handlers.end(); ++handler) {
    std::unique_ptr<base::DictionaryValue> handler_value(
        new base::DictionaryValue());
    handler_value->SetString("protocol", handler->protocol());
    handler_value->SetString("spec", handler->url().spec());
    handler_value->SetString("host", handler->url().host());
    handler_value->SetBoolean("is_default", registry.IsDefault(*handler));
    handler_list->Append(std::move(handler_value));
  }
}

void ProtocolHandlersHandler::GetHandlersForProtocol(
    const std::string& protocol,
    base::DictionaryValue* handlers_value) {
  ProtocolHandlerRegistry* registry = GetProtocolHandlerRegistry();
  handlers_value->SetString("protocol", protocol);

  auto handlers_list = std::make_unique<base::ListValue>();
  GetHandlersAsListValue(*registry, registry->GetHandlersFor(protocol),
                         handlers_list.get());
  handlers_value->Set("handlers", std::move(handlers_list));
}

void ProtocolHandlersHandler::GetIgnoredHandlers(base::ListValue* handlers) {
  ProtocolHandlerRegistry* registry = GetProtocolHandlerRegistry();
  ProtocolHandlerRegistry::ProtocolHandlerList ignored_handlers =
      registry->GetIgnoredHandlers();
  return GetHandlersAsListValue(*registry, ignored_handlers, handlers);
}

void ProtocolHandlersHandler::UpdateHandlerList() {
  ProtocolHandlerRegistry* registry = GetProtocolHandlerRegistry();
  std::vector<std::string> protocols;
  registry->GetRegisteredProtocols(&protocols);

  base::ListValue handlers;
  for (std::vector<std::string>::iterator protocol = protocols.begin();
       protocol != protocols.end(); protocol++) {
    std::unique_ptr<base::DictionaryValue> handler_value(
        new base::DictionaryValue());
    GetHandlersForProtocol(*protocol, handler_value.get());
    handlers.Append(std::move(handler_value));
  }

  std::unique_ptr<base::ListValue> ignored_handlers(new base::ListValue());
  GetIgnoredHandlers(ignored_handlers.get());
  FireWebUIListener("setProtocolHandlers", handlers);
  FireWebUIListener("setIgnoredProtocolHandlers", *ignored_handlers);
}

void ProtocolHandlersHandler::HandleObserveProtocolHandlers(
    const base::ListValue* args) {
  AllowJavascript();
  SendHandlersEnabledValue();
  UpdateHandlerList();
}

void ProtocolHandlersHandler::HandleObserveProtocolHandlersEnabledState(
    const base::ListValue* args) {
  AllowJavascript();
  SendHandlersEnabledValue();
}

void ProtocolHandlersHandler::SendHandlersEnabledValue() {
  FireWebUIListener("setHandlersEnabled",
                    base::Value(GetProtocolHandlerRegistry()->enabled()));
}

void ProtocolHandlersHandler::HandleRemoveHandler(const base::ListValue* args) {
  ProtocolHandler handler(ParseHandlerFromArgs(args));
  CHECK(!handler.IsEmpty());
  GetProtocolHandlerRegistry()->RemoveHandler(handler);

  // No need to call UpdateHandlerList() - we should receive a notification
  // that the ProtocolHandlerRegistry has changed and we will update the view
  // then.
}

void ProtocolHandlersHandler::HandleSetHandlersEnabled(
    const base::ListValue* args) {
  bool enabled = true;
  CHECK(args->GetBoolean(0, &enabled));
  if (enabled)
    GetProtocolHandlerRegistry()->Enable();
  else
    GetProtocolHandlerRegistry()->Disable();
}

void ProtocolHandlersHandler::HandleSetDefault(const base::ListValue* args) {
  const ProtocolHandler& handler(ParseHandlerFromArgs(args));
  CHECK(!handler.IsEmpty());
  GetProtocolHandlerRegistry()->OnAcceptRegisterProtocolHandler(handler);
}

ProtocolHandler ProtocolHandlersHandler::ParseHandlerFromArgs(
    const base::ListValue* args) const {
  base::string16 protocol;
  base::string16 url;
  bool ok = args->GetString(0, &protocol) && args->GetString(1, &url);
  if (!ok)
    return ProtocolHandler::EmptyProtocolHandler();
  return ProtocolHandler::CreateProtocolHandler(base::UTF16ToUTF8(protocol),
                                                GURL(base::UTF16ToUTF8(url)));
}

void ProtocolHandlersHandler::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  DCHECK_EQ(chrome::NOTIFICATION_PROTOCOL_HANDLER_REGISTRY_CHANGED, type);
  SendHandlersEnabledValue();
  UpdateHandlerList();
}

}  // namespace settings
