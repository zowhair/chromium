// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_FIDO_U2F_DISCOVERY_H_
#define DEVICE_FIDO_U2F_DISCOVERY_H_

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/component_export.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/strings/string_piece.h"
#include "device/fido/u2f_transport_protocol.h"

namespace service_manager {
class Connector;
}

namespace device {

class U2fDevice;

namespace internal {
class ScopedU2fDiscoveryFactory;
}

class COMPONENT_EXPORT(DEVICE_FIDO) U2fDiscovery {
 public:
  class COMPONENT_EXPORT(DEVICE_FIDO) Observer {
   public:
    virtual ~Observer();
    virtual void DiscoveryStarted(U2fDiscovery* discovery, bool success) = 0;
    virtual void DiscoveryStopped(U2fDiscovery* discovery, bool success) = 0;
    virtual void DeviceAdded(U2fDiscovery* discovery, U2fDevice* device) = 0;
    virtual void DeviceRemoved(U2fDiscovery* discovery, U2fDevice* device) = 0;
  };

  // Factory function to construct an instance that discovers authenticators on
  // the given |transport| protocol.
  //
  // U2fTransportProtocol::kUsbHumanInterfaceDevice requires specifying a valid
  // |connector| on Desktop, and is not valid on Android.
  static std::unique_ptr<U2fDiscovery> Create(
      U2fTransportProtocol transport,
      ::service_manager::Connector* connector);

  virtual ~U2fDiscovery();

  virtual U2fTransportProtocol GetTransportProtocol() const = 0;
  virtual void Start() = 0;
  virtual void Stop() = 0;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  void NotifyDiscoveryStarted(bool success);
  void NotifyDiscoveryStopped(bool success);
  void NotifyDeviceAdded(U2fDevice* device);
  void NotifyDeviceRemoved(U2fDevice* device);

  std::vector<U2fDevice*> GetDevices();
  std::vector<const U2fDevice*> GetDevices() const;

  U2fDevice* GetDevice(base::StringPiece device_id);
  const U2fDevice* GetDevice(base::StringPiece device_id) const;

 protected:
  U2fDiscovery();

  virtual bool AddDevice(std::unique_ptr<U2fDevice> device);
  virtual bool RemoveDevice(base::StringPiece device_id);

  std::map<std::string, std::unique_ptr<U2fDevice>, std::less<>> devices_;
  base::ObserverList<Observer> observers_;

 private:
  friend class internal::ScopedU2fDiscoveryFactory;

  // Factory function can be overridden by tests to construct fakes.
  using FactoryFuncPtr = decltype(&Create);
  static FactoryFuncPtr g_factory_func_;

  DISALLOW_COPY_AND_ASSIGN(U2fDiscovery);
};

namespace internal {

// Base class for a scoped override of U2fDiscovery::Create, used in unit tests,
// layout tests, and when running with the Web Authn Testing API enabled.
//
// While there is a subclass instance in scope, calls to the factory method will
// be hijacked such that the derived class's CreateU2fDiscovery method will be
// invoked instead.
class COMPONENT_EXPORT(DEVICE_FIDO) ScopedU2fDiscoveryFactory {
 public:
  ScopedU2fDiscoveryFactory();
  ~ScopedU2fDiscoveryFactory();

 protected:
  virtual std::unique_ptr<U2fDiscovery> CreateU2fDiscovery(
      U2fTransportProtocol transport,
      ::service_manager::Connector* connector) = 0;

 private:
  static std::unique_ptr<U2fDiscovery>
  ForwardCreateU2fDiscoveryToCurrentFactory(
      U2fTransportProtocol transport,
      ::service_manager::Connector* connector);

  static ScopedU2fDiscoveryFactory* g_current_factory;
  ScopedU2fDiscoveryFactory* original_factory_;
  U2fDiscovery::FactoryFuncPtr original_factory_func_;

  DISALLOW_COPY_AND_ASSIGN(ScopedU2fDiscoveryFactory);
};

}  // namespace internal
}  // namespace device

#endif  // DEVICE_FIDO_U2F_DISCOVERY_H_
