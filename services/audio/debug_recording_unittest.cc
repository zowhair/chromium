// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/audio/debug_recording.h"

#include <memory>

#include "base/files/file_path.h"
#include "base/test/scoped_task_environment.h"
#include "media/audio/audio_debug_recording_test.h"
#include "media/audio/mock_audio_debug_recording_manager.h"
#include "media/audio/mock_audio_manager.h"
#include "services/audio/public/cpp/debug_recording_session.h"
#include "services/audio/public/mojom/debug_recording.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"

using testing::_;

namespace audio {

namespace {

const base::FilePath::CharType kBaseFileName[] =
    FILE_PATH_LITERAL("base_file_name");

// Empty function bound and passed to DebugRecording::CreateWavFile.
void FileCreated(base::File file) {}

}  // namespace

class MockFileProvider : public mojom::DebugRecordingFileProvider {
 public:
  MockFileProvider(mojom::DebugRecordingFileProviderRequest request,
                   const base::FilePath& file_name_base)
      : binding_(this, std::move(request)) {}

  MOCK_METHOD2(DoCreateWavFile,
               void(media::AudioDebugRecordingStreamType stream_type,
                    uint32_t id));
  void CreateWavFile(media::AudioDebugRecordingStreamType stream_type,
                     uint32_t id,
                     CreateWavFileCallback reply_callback) override {
    DoCreateWavFile(stream_type, id);
    std::move(reply_callback).Run(base::File());
  }

 private:
  mojo::Binding<mojom::DebugRecordingFileProvider> binding_;

  DISALLOW_COPY_AND_ASSIGN(MockFileProvider);
};

class DebugRecordingTest : public media::AudioDebugRecordingTest {
 public:
  DebugRecordingTest() = default;
  ~DebugRecordingTest() override = default;

  void SetUp() override {
    CreateAudioManager();
    InitializeAudioDebugRecordingManager();
  }

  void TearDown() override { ShutdownAudioManager(); }

 protected:
  void CreateDebugRecording() {
    debug_recording_ = std::make_unique<DebugRecording>(
        mojo::MakeRequest(&debug_recording_ptr_),
        static_cast<media::AudioManager*>(mock_audio_manager_.get()));
    EXPECT_TRUE(debug_recording_ptr_.is_bound());
  }

  void EnableDebugRecording() {
    mojom::DebugRecordingFileProviderPtr file_provider_ptr;
    DebugRecordingSession::DebugRecordingFileProvider file_provider(
        mojo::MakeRequest(&file_provider_ptr), base::FilePath(kBaseFileName));
    ASSERT_TRUE(file_provider_ptr.is_bound());
    debug_recording_ptr_->Enable(std::move(file_provider_ptr));
  }

  void DestroyDebugRecording() { debug_recording_ptr_.reset(); }

  std::unique_ptr<DebugRecording> debug_recording_;
  mojom::DebugRecordingPtr debug_recording_ptr_;

 private:
  DISALLOW_COPY_AND_ASSIGN(DebugRecordingTest);
};

TEST_F(DebugRecordingTest, EnableResetEnablesDisablesDebugRecording) {
  CreateDebugRecording();

  EXPECT_CALL(*mock_debug_recording_manager_, EnableDebugRecording(_));
  EnableDebugRecording();

  EXPECT_CALL(*mock_debug_recording_manager_, DisableDebugRecording());
  DestroyDebugRecording();
}

TEST_F(DebugRecordingTest, ResetWithoutEnableDoesNotDisableDebugRecording) {
  CreateDebugRecording();

  EXPECT_CALL(*mock_debug_recording_manager_, DisableDebugRecording()).Times(0);
  DestroyDebugRecording();
}

TEST_F(DebugRecordingTest, CreateWavFileCallsFileProviderCreateWavFile) {
  CreateDebugRecording();

  mojom::DebugRecordingFileProviderPtr file_provider_ptr;
  MockFileProvider mock_file_provider(mojo::MakeRequest(&file_provider_ptr),
                                      base::FilePath(kBaseFileName));

  EXPECT_CALL(*mock_debug_recording_manager_, EnableDebugRecording(_));
  debug_recording_ptr_->Enable(std::move(file_provider_ptr));
  scoped_task_environment_.RunUntilIdle();

  const int id = 1;
  EXPECT_CALL(
      mock_file_provider,
      DoCreateWavFile(media::AudioDebugRecordingStreamType::kInput, id));
  debug_recording_->CreateWavFile(media::AudioDebugRecordingStreamType::kInput,
                                  id, base::BindOnce(&FileCreated));
  scoped_task_environment_.RunUntilIdle();

  EXPECT_CALL(*mock_debug_recording_manager_, DisableDebugRecording());
  DestroyDebugRecording();
}

}  // namespace audio
