// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/internal/background_service/in_memory_download_driver.h"

#include <memory>

#include "components/download/internal/background_service/test/mock_download_driver_client.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "storage/browser/blob/blob_data_handle.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;

namespace download {
namespace {

MATCHER_P(DriverEntryEqual, entry, "") {
  return entry.guid == arg.guid && entry.state == arg.state &&
         entry.bytes_downloaded == arg.bytes_downloaded;
}

// Test in memory download that doesn't do complex IO.
class TestInMemoryDownload : public InMemoryDownload {
 public:
  TestInMemoryDownload(const std::string& guid,
                       InMemoryDownload::Delegate* delegate)
      : InMemoryDownload(guid), delegate_(delegate) {
    DCHECK(delegate_) << "Delegate can't be nullptr.";
  }

  void SimulateDownloadProgress() {
    state_ = InMemoryDownload::State::IN_PROGRESS;
    delegate_->OnDownloadProgress(this);
  }

  void SimulateDownloadComplete(bool success) {
    state_ = success ? InMemoryDownload::State::COMPLETE
                     : InMemoryDownload::State::FAILED;
    delegate_->OnDownloadComplete(this);
  }

  // InMemoryDownload implementation.
  void Start() override {}
  void Pause() override {}
  void Resume() override {}
  std::unique_ptr<storage::BlobDataHandle> ResultAsBlob() const override {
    return nullptr;
  }
  size_t EstimateMemoryUsage() const override { return 0u; }

 private:
  InMemoryDownload::Delegate* delegate_;
  DISALLOW_COPY_AND_ASSIGN(TestInMemoryDownload);
};

// Factory that injects to InMemoryDownloadDriver and only creates fake objects.
class TestInMemoryDownloadFactory : public InMemoryDownload::Factory {
 public:
  TestInMemoryDownloadFactory() = default;
  ~TestInMemoryDownloadFactory() override = default;

  // InMemoryDownload::Factory implementation.
  std::unique_ptr<InMemoryDownload> Create(
      const std::string& guid,
      const RequestParams& request_params,
      const net::NetworkTrafficAnnotationTag& traffic_annotation,
      InMemoryDownload::Delegate* delegate) override {
    auto download = std::make_unique<TestInMemoryDownload>(guid, delegate);
    download_ = download.get();
    return download;
  }

  // Returns the last created download, if the driver remove the object, this
  // can be invalid memory, so need to use with caution.
  TestInMemoryDownload* last_created_download() { return download_; }

 private:
  TestInMemoryDownload* download_ = nullptr;
  DISALLOW_COPY_AND_ASSIGN(TestInMemoryDownloadFactory);
};

class InMemoryDownloadDriverTest : public testing::Test {
 public:
  InMemoryDownloadDriverTest() = default;
  ~InMemoryDownloadDriverTest() override = default;

  // Helper method to call public method on |driver_|.
  DownloadDriver* driver() {
    return static_cast<DownloadDriver*>(driver_.get());
  }

  MockDriverClient* driver_client() { return &driver_client_; }

  TestInMemoryDownloadFactory* factory() { return factory_; }

  void SetUp() override {
    auto factory = std::make_unique<TestInMemoryDownloadFactory>();
    factory_ = factory.get();
    driver_ = std::make_unique<InMemoryDownloadDriver>(std::move(factory));
    driver()->Initialize(&driver_client_);
  }

  void TearDown() override {
    // Driver should be teared down before its owner.
    driver_.reset();
  }

  void Start(const std::string& guid) {
    RequestParams params;
    base::FilePath path;
    driver()->Start(params, guid, path, TRAFFIC_ANNOTATION_FOR_TESTS);
  }

 private:
  testing::NiceMock<MockDriverClient> driver_client_;
  std::unique_ptr<InMemoryDownloadDriver> driver_;
  TestInMemoryDownloadFactory* factory_;
  DISALLOW_COPY_AND_ASSIGN(InMemoryDownloadDriverTest);
};

// Verifies in memory download success and remove API.
TEST_F(InMemoryDownloadDriverTest, DownloadSuccessAndRemove) {
  // General states check.
  EXPECT_TRUE(driver()->IsReady());

  const std::string guid = "1234";
  EXPECT_CALL(*driver_client(), OnDownloadCreated(_)).Times(1);
  Start(guid);

  // After starting a download, we should be able to find a record in the
  // driver.
  base::Optional<DriverEntry> entry = driver()->Find(guid);
  EXPECT_TRUE(entry.has_value());
  EXPECT_EQ(guid, entry->guid);
  EXPECT_EQ(DriverEntry::State::IN_PROGRESS, entry->state);

  EXPECT_CALL(*driver_client(), OnDownloadUpdated(_));
  factory()->last_created_download()->SimulateDownloadProgress();

  DriverEntry match_entry;
  match_entry.state = DriverEntry::State::COMPLETE;
  match_entry.guid = guid;
  EXPECT_CALL(*driver_client(),
              OnDownloadSucceeded(DriverEntryEqual(match_entry)))
      .Times(1);
  EXPECT_CALL(*driver_client(), OnDownloadFailed(_, _)).Times(0);

  // Trigger download complete.
  factory()->last_created_download()->SimulateDownloadComplete(
      true /* success*/);
  EXPECT_TRUE(entry.has_value());
  entry = driver()->Find(guid);
  EXPECT_EQ(guid, entry->guid);
  EXPECT_EQ(DriverEntry::State::COMPLETE, entry->state);

  driver()->Remove(guid);
  entry = driver()->Find(guid);
  EXPECT_FALSE(entry.has_value());
}

// Verifies in memory download failure.
TEST_F(InMemoryDownloadDriverTest, DownloadFailure) {
  // General states check.
  EXPECT_TRUE(driver()->IsReady());

  EXPECT_CALL(*driver_client(), OnDownloadCreated(_)).Times(1);
  EXPECT_CALL(*driver_client(), OnDownloadUpdated(_)).Times(0);

  const std::string guid = "1234";
  Start(guid);

  DriverEntry match_entry;
  match_entry.state = DriverEntry::State::INTERRUPTED;
  match_entry.guid = guid;
  EXPECT_CALL(*driver_client(),
              OnDownloadFailed(DriverEntryEqual(match_entry), _))
      .Times(1);
  EXPECT_CALL(*driver_client(), OnDownloadSucceeded(_)).Times(0);

  // Trigger download complete.
  factory()->last_created_download()->SimulateDownloadComplete(
      false /* success*/);
  base::Optional<DriverEntry> entry = driver()->Find(guid);
  EXPECT_TRUE(entry.has_value());
  EXPECT_EQ(guid, entry->guid);
  EXPECT_EQ(DriverEntry::State::INTERRUPTED, entry->state);
}

}  // namespace
}  // namespace download
