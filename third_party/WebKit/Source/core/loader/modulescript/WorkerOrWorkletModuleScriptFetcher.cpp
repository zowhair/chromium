// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/loader/modulescript/WorkerOrWorkletModuleScriptFetcher.h"

#include "platform/CrossThreadFunctional.h"

namespace blink {

WorkerOrWorkletModuleScriptFetcher::WorkerOrWorkletModuleScriptFetcher(
    WorkerOrWorkletModuleFetchCoordinatorProxy* coordinator_proxy)
    : coordinator_proxy_(coordinator_proxy) {
  DCHECK(coordinator_proxy_);
}

void WorkerOrWorkletModuleScriptFetcher::Trace(blink::Visitor* visitor) {
  visitor->Trace(coordinator_proxy_);
  ModuleScriptFetcher::Trace(visitor);
}

void WorkerOrWorkletModuleScriptFetcher::Fetch(
    FetchParameters& fetch_params,
    ModuleScriptFetcher::Client* client) {
  SetClient(client);
  coordinator_proxy_->Fetch(fetch_params, this);
}

void WorkerOrWorkletModuleScriptFetcher::OnFetched(
    const ModuleScriptCreationParams& params) {
  HeapVector<Member<ConsoleMessage>> error_messages;
  Finalize(params, error_messages);
}

void WorkerOrWorkletModuleScriptFetcher::OnFailed() {
  HeapVector<Member<ConsoleMessage>> error_messages;
  Finalize(WTF::nullopt, error_messages);
}

void WorkerOrWorkletModuleScriptFetcher::Finalize(
    const WTF::Optional<ModuleScriptCreationParams>& params,
    const HeapVector<Member<ConsoleMessage>>& error_messages) {
  NotifyFetchFinished(params, error_messages);
}

}  // namespace blink
