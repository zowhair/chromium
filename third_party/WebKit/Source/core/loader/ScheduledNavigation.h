// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ScheduledNavigation_h
#define ScheduledNavigation_h

#include "base/macros.h"
#include "public/platform/Platform.h"

namespace blink {

class Document;
class LocalFrame;
class UserGestureIndicator;
class UserGestureToken;

class ScheduledNavigation
    : public GarbageCollectedFinalized<ScheduledNavigation> {
 public:
  enum class Reason {
    kFormSubmissionGet,
    kFormSubmissionPost,
    kHttpHeaderRefresh,
    kFrameNavigation,
    kMetaTagRefresh,
    kPageBlock,
    kReload,
  };

  ScheduledNavigation(Reason,
                      double delay,
                      Document* origin_document,
                      bool replaces_current_item,
                      bool is_location_change);
  virtual ~ScheduledNavigation();

  virtual void Fire(LocalFrame*) = 0;

  virtual KURL Url() const = 0;

  virtual bool ShouldStartTimer(LocalFrame*) { return true; }

  Reason GetReason() const { return reason_; }
  double Delay() const { return delay_; }
  Document* OriginDocument() const { return origin_document_.Get(); }
  bool ReplacesCurrentItem() const { return replaces_current_item_; }
  bool IsLocationChange() const { return is_location_change_; }
  std::unique_ptr<UserGestureIndicator> CreateUserGestureIndicator();

  virtual void Trace(blink::Visitor* visitor) {
    visitor->Trace(origin_document_);
  }

 protected:
  void ClearUserGesture() { user_gesture_token_ = nullptr; }

 private:
  Reason reason_;
  double delay_;
  Member<Document> origin_document_;
  bool replaces_current_item_;
  bool is_location_change_;
  scoped_refptr<UserGestureToken> user_gesture_token_;

  DISALLOW_COPY_AND_ASSIGN(ScheduledNavigation);
};

}  // namespace blink

#endif  // ScheduledNavigation_h
