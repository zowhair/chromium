/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "core/dom/ContextFeatures.h"

#include <memory>
#include "core/dom/Document.h"
#include "core/page/Page.h"
#include "platform/wtf/StdLibExtras.h"

namespace blink {

std::unique_ptr<ContextFeaturesClient> ContextFeaturesClient::Empty() {
  return std::make_unique<ContextFeaturesClient>();
}

const char ContextFeatures::kSupplementName[] = "ContextFeatures";

ContextFeatures& ContextFeatures::DefaultSwitch() {
  DEFINE_STATIC_LOCAL(
      ContextFeatures, instance,
      (ContextFeatures::Create(ContextFeaturesClient::Empty())));
  return instance;
}

bool ContextFeatures::PagePopupEnabled(Document* document) {
  if (!document)
    return false;
  return document->GetContextFeatures().IsEnabled(document, kPagePopup, false);
}

bool ContextFeatures::MutationEventsEnabled(Document* document) {
  DCHECK(document);
  if (!document)
    return true;
  return document->GetContextFeatures().IsEnabled(document, kMutationEvents,
                                                  true);
}

void ProvideContextFeaturesTo(Page& page,
                              std::unique_ptr<ContextFeaturesClient> client) {
  Supplement<Page>::ProvideTo(page, ContextFeatures::Create(std::move(client)));
}

void ProvideContextFeaturesToDocumentFrom(Document& document, Page& page) {
  ContextFeatures* provided = Supplement<Page>::From<ContextFeatures>(page);
  if (!provided)
    return;
  document.SetContextFeatures(*provided);
}

}  // namespace blink
