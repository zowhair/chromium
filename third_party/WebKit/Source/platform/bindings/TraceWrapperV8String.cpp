// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/bindings/TraceWrapperV8String.h"
#include "platform/bindings/V8Binding.h"

namespace blink {

void TraceWrapperV8String::Concat(v8::Isolate* isolate, const String& string) {
  DCHECK(isolate);
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::String> target_string =
      (string_.IsEmpty()) ? V8String(isolate, string)
                          : v8::String::Concat(string_.NewLocal(isolate),
                                               V8String(isolate, string));
  string_.Set(isolate, target_string);
}

String TraceWrapperV8String::Flatten(v8::Isolate* isolate) const {
  if (IsEmpty())
    return String();
  DCHECK(isolate);
  v8::HandleScope handle_scope(isolate);
  return V8StringToWebCoreString<String>(string_.NewLocal(isolate),
                                         kExternalize);
}

}  // namespace blink
