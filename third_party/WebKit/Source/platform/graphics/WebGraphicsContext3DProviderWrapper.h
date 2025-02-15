// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebGraphicsContext3DProviderWrapper_h
#define WebGraphicsContext3DProviderWrapper_h

#include <memory>
#include <utility>

#include "base/memory/ptr_util.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "platform/PlatformExport.h"
#include "platform/graphics/gpu/GraphicsContext3DUtils.h"
#include "public/platform/WebGraphicsContext3DProvider.h"

namespace blink {

class PLATFORM_EXPORT WebGraphicsContext3DProviderWrapper {
 public:
  class DestructionObserver {
   public:
    virtual ~DestructionObserver() {}
    virtual void OnContextDestroyed() = 0;
  };

  WebGraphicsContext3DProviderWrapper(
      std::unique_ptr<WebGraphicsContext3DProvider> provider)
      : context_provider_(std::move(provider)), weak_ptr_factory_(this) {
    DCHECK(context_provider_);
    utils_ = base::WrapUnique(new GraphicsContext3DUtils(GetWeakPtr()));
  }
  ~WebGraphicsContext3DProviderWrapper();

  base::WeakPtr<WebGraphicsContext3DProviderWrapper> GetWeakPtr() {
    return weak_ptr_factory_.GetWeakPtr();
  }
  WebGraphicsContext3DProvider* ContextProvider() {
    return context_provider_.get();
  }

  GraphicsContext3DUtils* Utils() { return utils_.get(); }

  void AddObserver(DestructionObserver*);
  void RemoveObserver(DestructionObserver*);

 private:
  std::unique_ptr<GraphicsContext3DUtils> utils_;
  std::unique_ptr<WebGraphicsContext3DProvider> context_provider_;
  base::ObserverList<DestructionObserver> observers_;
  base::WeakPtrFactory<WebGraphicsContext3DProviderWrapper> weak_ptr_factory_;
};

}  // namespace blink

#endif  // WebGraphicsContext3DProviderWrapper_h
