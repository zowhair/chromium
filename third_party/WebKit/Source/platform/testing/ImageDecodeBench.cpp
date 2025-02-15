// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Provides a minimal wrapping of the Blink image decoders. Used to perform
// a non-threaded, memory-to-memory image decode using micro second accuracy
// clocks to measure image decode time. Basic usage:
//
//   % ninja -C out/Release image_decode_bench &&
//      ./out/Release/image_decode_bench file [iterations]
//
// TODO(noel): Consider adding md5 checksum support to WTF. Use it to compute
// the decoded image frame md5 and output that value.
//
// TODO(noel): Consider integrating this tool in Chrome telemetry for realz,
// using the image corpora used to assess Blink image decode performance. See
// http://crbug.com/398235#c103 and http://crbug.com/258324#c5

#include <fstream>

#include "base/command_line.h"
#include "base/memory/scoped_refptr.h"
#include "base/message_loop/message_loop.h"
#include "mojo/edk/embedder/embedder.h"
#include "platform/SharedBuffer.h"
#include "platform/image-decoders/ImageDecoder.h"
#include "platform/wtf/Time.h"
#include "platform/wtf/Vector.h"
#include "public/platform/Platform.h"

namespace blink {

namespace {

scoped_refptr<SharedBuffer> ReadFile(const char* name) {
  std::ifstream file(name, std::ios::in | std::ios::binary);
  if (!file) {
    fprintf(stderr, "Cannot open file %s\n", name);
    exit(2);
  }

  file.seekg(0, std::ios::end);
  int file_size = file.tellg();
  file.seekg(0, std::ios::beg);

  if (!file || file_size <= 0) {
    fprintf(stderr, "Error seeking file %s\n", name);
    exit(2);
  }

  Vector<char> buffer(file_size);
  if (!file.read(buffer.data(), file_size)) {
    fprintf(stderr, "Error reading file %s\n", name);
    exit(2);
  }

  return SharedBuffer::AdoptVector(buffer);
}

struct ImageMeta {
  char* name;
  int width;
  int height;
  int frames;
  // Cumulative time in seconds to decode all frames.
  double time;
};

void DecodeFailure(ImageMeta* image) {
  fprintf(stderr, "Failed to decode image %s\n", image->name);
  exit(3);
}

void DecodeImageData(SharedBuffer* data, ImageMeta* image) {
  std::unique_ptr<ImageDecoder> decoder = ImageDecoder::Create(
      data, true, ImageDecoder::kAlphaPremultiplied, ColorBehavior::Ignore());

  auto start = CurrentTimeTicks();

  bool all_data_received = true;
  decoder->SetData(data, all_data_received);

  size_t frame_count = decoder->FrameCount();
  for (size_t index = 0; index < frame_count; ++index) {
    if (!decoder->DecodeFrameBufferAtIndex(index))
      DecodeFailure(image);
  }

  image->time += (CurrentTimeTicks() - start).InSecondsF();

  if (!frame_count || decoder->Failed())
    DecodeFailure(image);

  image->width = decoder->Size().Width();
  image->height = decoder->Size().Height();
  image->frames = frame_count;
}

}  // namespace

int ImageDecodeBenchMain(int argc, char* argv[]) {
  base::CommandLine::Init(argc, argv);
  const char* program = argv[0];

  if (argc < 2) {
    fprintf(stderr, "Usage: %s file [iterations]\n", program);
    exit(1);
  }

  // Control bench decode iterations.

  size_t decode_iterations = 1;
  if (argc >= 3) {
    char* end = nullptr;
    decode_iterations = strtol(argv[2], &end, 10);
    if (*end != '\0' || !decode_iterations) {
      fprintf(stderr,
              "Second argument should be number of iterations. "
              "The default is 1. You supplied %s\n",
              argv[2]);
      exit(1);
    }
  }

  // Create a web platform. blink::Platform can't be used directly because it
  // has a protected constructor.

  class WebPlatform : public Platform {};
  Platform::Initialize(new WebPlatform());

  // Read entire file content into |data| (a contiguous block of memory) then
  // decode it to verify the image and record its ImageMeta data.

  ImageMeta image = {argv[1], 0, 0, 0, 0};
  scoped_refptr<SharedBuffer> data = ReadFile(argv[1]);
  DecodeImageData(data.get(), &image);

  // Image decode bench for decode_iterations.

  double total_time = 0.0;
  for (size_t i = 0; i < decode_iterations; ++i) {
    image.time = 0.0;
    DecodeImageData(data.get(), &image);
    total_time += image.time;
  }

  // Results to stdout.

  double average_time = total_time / decode_iterations;
  printf("%f %f\n", total_time, average_time);
  return 0;
}

}  // namespace blink

int main(int argc, char* argv[]) {
  base::MessageLoop message_loop;
  mojo::edk::Init();
  return blink::ImageDecodeBenchMain(argc, argv);
}
