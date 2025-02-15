// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/policy/core/common/cloud/machine_level_user_cloud_policy_store.h"

#include <utility>

#include "base/memory/ptr_util.h"
#include "components/policy/core/common/cloud/cloud_policy_constants.h"

namespace policy {
namespace {

const base::FilePath::CharType kPolicyDir[] = FILE_PATH_LITERAL("Policy");
const base::FilePath::CharType kPolicyCache[] =
    FILE_PATH_LITERAL("Machine Level User Cloud Policy");
const base::FilePath::CharType kKeyCache[] =
    FILE_PATH_LITERAL("Machine Level User Cloud Policy Signing Key");

}  // namespace

MachineLevelUserCloudPolicyStore::MachineLevelUserCloudPolicyStore(
    const std::string& machine_dm_token,
    const std::string& machine_client_id,
    const base::FilePath& policy_path,
    const base::FilePath& key_path,
    scoped_refptr<base::SequencedTaskRunner> background_task_runner)
    : DesktopCloudPolicyStore(policy_path,
                              key_path,
                              background_task_runner,
                              PolicyScope::POLICY_SCOPE_MACHINE),
      machine_dm_token_(machine_dm_token),
      machine_client_id_(machine_client_id) {}

MachineLevelUserCloudPolicyStore::~MachineLevelUserCloudPolicyStore() {}

// static
std::unique_ptr<MachineLevelUserCloudPolicyStore>
MachineLevelUserCloudPolicyStore::Create(
    const std::string& machine_dm_token,
    const std::string& machine_client_id,
    const base::FilePath& user_data_dir,
    scoped_refptr<base::SequencedTaskRunner> background_task_runner) {
  base::FilePath policy_dir = user_data_dir.Append(kPolicyDir);
  base::FilePath policy_cache_file = policy_dir.Append(kPolicyCache);
  base::FilePath key_cache_file = policy_dir.Append(kKeyCache);
  return std::make_unique<MachineLevelUserCloudPolicyStore>(
      machine_dm_token, machine_client_id, policy_cache_file, key_cache_file,
      background_task_runner);
}

void MachineLevelUserCloudPolicyStore::LoadImmediately() {
  // There is no global dm token, stop loading the policy cache. The policy will
  // be fetched in the end of enrollment process.
  if (machine_dm_token_.empty()) {
    DVLOG(1) << "LoadImmediately ignored, no DM token";
    return;
  }
  DesktopCloudPolicyStore::LoadImmediately();
}

void MachineLevelUserCloudPolicyStore::Load() {
  // There is no global dm token, stop loading the policy cache. The policy will
  // be fetched in the end of enrollment process.
  if (machine_dm_token_.empty()) {
    DVLOG(1) << "Load ignored, no DM token";
    return;
  }
  DesktopCloudPolicyStore::Load();
}

std::unique_ptr<UserCloudPolicyValidator>
MachineLevelUserCloudPolicyStore::CreateValidator(
    std::unique_ptr<enterprise_management::PolicyFetchResponse> policy,
    CloudPolicyValidatorBase::ValidateTimestampOption option) {
  std::unique_ptr<UserCloudPolicyValidator> validator =
      UserCloudPolicyValidator::Create(std::move(policy),
                                       background_task_runner());
  validator->ValidatePolicyType(
      dm_protocol::kChromeMachineLevelUserCloudPolicyType);
  validator->ValidateDMToken(machine_dm_token_,
                             CloudPolicyValidatorBase::DM_TOKEN_REQUIRED);
  validator->ValidateDeviceId(machine_client_id_,
                              CloudPolicyValidatorBase::DEVICE_ID_REQUIRED);
  if (policy_) {
    validator->ValidateTimestamp(base::Time::FromJavaTime(policy_->timestamp()),
                                 option);
  }
  validator->ValidatePayload();
  return validator;
}

void MachineLevelUserCloudPolicyStore::SetupRegistration(
    const std::string& machine_dm_token,
    const std::string& machine_client_id) {
  machine_dm_token_ = machine_dm_token;
  machine_client_id_ = machine_client_id;
}

void MachineLevelUserCloudPolicyStore::Validate(
    std::unique_ptr<enterprise_management::PolicyFetchResponse> policy,
    std::unique_ptr<enterprise_management::PolicySigningKey> key,
    bool validate_in_background,
    const UserCloudPolicyValidator::CompletionCallback& callback) {
  std::unique_ptr<UserCloudPolicyValidator> validator = CreateValidator(
      std::move(policy), CloudPolicyValidatorBase::TIMESTAMP_VALIDATED);

  ValidateKeyAndSignature(validator.get(), key.get(), std::string());

  if (validate_in_background) {
    UserCloudPolicyValidator::StartValidation(std::move(validator), callback);
  } else {
    validator->RunValidation();
    callback.Run(validator.get());
  }
}

}  // namespace policy
