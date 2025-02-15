// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/u2f_discovery.h"

#include <utility>

#include "base/logging.h"
#include "build/build_config.h"
#include "device/fido/u2f_ble_discovery.h"
#include "device/fido/u2f_device.h"

// HID is not supported on Android.
#if !defined(OS_ANDROID)
#include "device/fido/u2f_hid_discovery.h"
#endif  // !defined(OS_ANDROID)

namespace device {

namespace {

std::unique_ptr<U2fDiscovery> CreateU2fDiscoveryImpl(
    U2fTransportProtocol transport,
    service_manager::Connector* connector) {
  std::unique_ptr<U2fDiscovery> discovery;
  switch (transport) {
    case U2fTransportProtocol::kUsbHumanInterfaceDevice:
#if !defined(OS_ANDROID)
      DCHECK(connector);
      discovery = std::make_unique<U2fHidDiscovery>(connector);
#else
      NOTREACHED() << "USB HID not supported on Android.";
#endif  // !defined(OS_ANDROID)
      break;
    case U2fTransportProtocol::kBluetoothLowEnergy:
      discovery = std::make_unique<U2fBleDiscovery>();
      break;
  }

  DCHECK(discovery);
  DCHECK_EQ(discovery->GetTransportProtocol(), transport);
  return discovery;
}

}  // namespace

U2fDiscovery::Observer::~Observer() = default;

// static
U2fDiscovery::FactoryFuncPtr U2fDiscovery::g_factory_func_ =
    &CreateU2fDiscoveryImpl;

// static
std::unique_ptr<U2fDiscovery> U2fDiscovery::Create(
    U2fTransportProtocol transport,
    service_manager::Connector* connector) {
  return (*g_factory_func_)(transport, connector);
}

U2fDiscovery::U2fDiscovery() = default;
U2fDiscovery::~U2fDiscovery() = default;

void U2fDiscovery::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void U2fDiscovery::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void U2fDiscovery::NotifyDiscoveryStarted(bool success) {
  for (auto& observer : observers_)
    observer.DiscoveryStarted(this, success);
}

void U2fDiscovery::NotifyDiscoveryStopped(bool success) {
  for (auto& observer : observers_)
    observer.DiscoveryStopped(this, success);
}

void U2fDiscovery::NotifyDeviceAdded(U2fDevice* device) {
  for (auto& observer : observers_)
    observer.DeviceAdded(this, device);
}

void U2fDiscovery::NotifyDeviceRemoved(U2fDevice* device) {
  for (auto& observer : observers_)
    observer.DeviceRemoved(this, device);
}

std::vector<U2fDevice*> U2fDiscovery::GetDevices() {
  std::vector<U2fDevice*> devices;
  devices.reserve(devices_.size());
  for (const auto& device : devices_)
    devices.push_back(device.second.get());
  return devices;
}

std::vector<const U2fDevice*> U2fDiscovery::GetDevices() const {
  std::vector<const U2fDevice*> devices;
  devices.reserve(devices_.size());
  for (const auto& device : devices_)
    devices.push_back(device.second.get());
  return devices;
}

U2fDevice* U2fDiscovery::GetDevice(base::StringPiece device_id) {
  return const_cast<U2fDevice*>(
      static_cast<const U2fDiscovery*>(this)->GetDevice(device_id));
}

const U2fDevice* U2fDiscovery::GetDevice(base::StringPiece device_id) const {
  auto found = devices_.find(device_id);
  return found != devices_.end() ? found->second.get() : nullptr;
}

bool U2fDiscovery::AddDevice(std::unique_ptr<U2fDevice> device) {
  std::string device_id = device->GetId();
  const auto result = devices_.emplace(std::move(device_id), std::move(device));
  if (result.second)
    NotifyDeviceAdded(result.first->second.get());
  return result.second;
}

bool U2fDiscovery::RemoveDevice(base::StringPiece device_id) {
  auto found = devices_.find(device_id);
  if (found == devices_.end())
    return false;

  auto device = std::move(found->second);
  devices_.erase(found);
  NotifyDeviceRemoved(device.get());
  return true;
}

// ScopedU2fDiscoveryFactory -------------------------------------------------

namespace internal {

ScopedU2fDiscoveryFactory::ScopedU2fDiscoveryFactory() {
  original_factory_ = std::exchange(g_current_factory, this);
  original_factory_func_ =
      std::exchange(U2fDiscovery::g_factory_func_,
                    &ForwardCreateU2fDiscoveryToCurrentFactory);
}

ScopedU2fDiscoveryFactory::~ScopedU2fDiscoveryFactory() {
  g_current_factory = original_factory_;
  U2fDiscovery::g_factory_func_ = original_factory_func_;
}

// static
std::unique_ptr<U2fDiscovery>
ScopedU2fDiscoveryFactory::ForwardCreateU2fDiscoveryToCurrentFactory(
    U2fTransportProtocol transport,
    ::service_manager::Connector* connector) {
  DCHECK(g_current_factory);
  return g_current_factory->CreateU2fDiscovery(transport, connector);
}

// static
ScopedU2fDiscoveryFactory* ScopedU2fDiscoveryFactory::g_current_factory =
    nullptr;

}  // namespace internal
}  // namespace device
