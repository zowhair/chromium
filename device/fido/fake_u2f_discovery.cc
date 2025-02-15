// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/fake_u2f_discovery.h"

#include <utility>

#include "base/logging.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace device {
namespace test {

// FakeU2fDiscovery ----------------------------------------------------------

FakeU2fDiscovery::FakeU2fDiscovery(U2fTransportProtocol transport,
                                   StartStopMode mode)
    : transport_(transport),
      mode_(mode),
      start_called_callback_(wait_for_start_loop_.QuitClosure()),
      stop_called_callback_(wait_for_stop_loop_.QuitClosure()) {}
FakeU2fDiscovery::~FakeU2fDiscovery() = default;

void FakeU2fDiscovery::WaitForCallToStart() {
  wait_for_start_loop_.Run();
  ASSERT_FALSE(start_called_callback_);
}

void FakeU2fDiscovery::WaitForCallToStop() {
  wait_for_stop_loop_.Run();
  ASSERT_FALSE(stop_called_callback_);
}

void FakeU2fDiscovery::SimulateStarted(bool success) {
  ASSERT_FALSE(is_running_);
  is_running_ = success;
  NotifyDiscoveryStarted(success);
}

void FakeU2fDiscovery::SimulateStopped(bool success) {
  ASSERT_TRUE(is_running_);
  is_running_ = !success;
  NotifyDiscoveryStopped(success);
}

void FakeU2fDiscovery::WaitForCallToStartAndSimulateSuccess() {
  WaitForCallToStart();
  SimulateStarted(true /* success */);
}

void FakeU2fDiscovery::WaitForCallToStopAndSimulateSuccess() {
  WaitForCallToStop();
  SimulateStopped(true /* success */);
}

U2fTransportProtocol FakeU2fDiscovery::GetTransportProtocol() const {
  return transport_;
}

void FakeU2fDiscovery::Start() {
  if (is_running_)
    return;

  ASSERT_TRUE(start_called_callback_) << "Start called twice.";
  std::move(start_called_callback_).Run();

  if (mode_ == StartStopMode::kAutomatic)
    SimulateStarted(true /* success */);
}

void FakeU2fDiscovery::Stop() {
  if (!is_running_)
    return;

  ASSERT_TRUE(stop_called_callback_) << "Stop called twice.";
  std::move(stop_called_callback_).Run();

  if (mode_ == StartStopMode::kAutomatic)
    SimulateStopped(true /* success */);
}

// ScopedFakeU2fDiscoveryFactory ---------------------------------------------

ScopedFakeU2fDiscoveryFactory::ScopedFakeU2fDiscoveryFactory() = default;
ScopedFakeU2fDiscoveryFactory::~ScopedFakeU2fDiscoveryFactory() = default;

FakeU2fDiscovery* ScopedFakeU2fDiscoveryFactory::ForgeNextHidDiscovery(
    FakeU2fDiscovery::StartStopMode mode) {
  next_hid_discovery_ = std::make_unique<FakeU2fDiscovery>(
      U2fTransportProtocol::kUsbHumanInterfaceDevice, mode);
  return next_hid_discovery_.get();
}

FakeU2fDiscovery* ScopedFakeU2fDiscoveryFactory::ForgeNextBleDiscovery(
    FakeU2fDiscovery::StartStopMode mode) {
  next_ble_discovery_ = std::make_unique<FakeU2fDiscovery>(
      U2fTransportProtocol::kBluetoothLowEnergy, mode);
  return next_ble_discovery_.get();
}

std::unique_ptr<U2fDiscovery> ScopedFakeU2fDiscoveryFactory::CreateU2fDiscovery(
    U2fTransportProtocol transport,
    ::service_manager::Connector* connector) {
  switch (transport) {
    case U2fTransportProtocol::kBluetoothLowEnergy:
      DCHECK(next_ble_discovery_);
      return std::move(next_ble_discovery_);
    case U2fTransportProtocol::kUsbHumanInterfaceDevice:
      DCHECK(next_hid_discovery_);
      return std::move(next_hid_discovery_);
  }
  NOTREACHED();
  return nullptr;
}

}  // namespace test
}  // namespace device
