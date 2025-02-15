// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PublicKeyCredential_h
#define PublicKeyCredential_h

#include "core/typed_arrays/DOMArrayBuffer.h"
#include "modules/credentialmanager/AuthenticationExtensionsClientOutputs.h"
#include "modules/credentialmanager/AuthenticatorResponse.h"
#include "modules/credentialmanager/Credential.h"
#include "modules/ModulesExport.h"
#include "platform/heap/Handle.h"

namespace blink {

class AuthenticatorResponse;
class ScriptPromise;
class ScriptState;

class MODULES_EXPORT PublicKeyCredential final : public Credential {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static PublicKeyCredential* Create(
      const String& id,
      DOMArrayBuffer* raw_id,
      AuthenticatorResponse*,
      const AuthenticationExtensionsClientOutputs&);

  DOMArrayBuffer* rawId() const { return raw_id_.Get(); }
  AuthenticatorResponse* response() const { return response_.Get(); }
  static ScriptPromise isUserVerifyingPlatformAuthenticatorAvailable(
      ScriptState*);
  void getClientExtensionResults(AuthenticationExtensionsClientOutputs&) const;

  // Credential:
  void Trace(blink::Visitor*) override;
  bool IsPublicKeyCredential() const override;

 private:
  explicit PublicKeyCredential(
      const String& id,
      DOMArrayBuffer* raw_id,
      AuthenticatorResponse*,
      const AuthenticationExtensionsClientOutputs& extension_outputs);

  const Member<DOMArrayBuffer> raw_id_;
  const Member<AuthenticatorResponse> response_;
  AuthenticationExtensionsClientOutputs extension_outputs_;
};

}  // namespace blink

#endif  // PublicKeyCredential_h
