// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/smb_client/discovery/network_scanner.h"

#include <map>
#include <vector>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "chrome/browser/chromeos/smb_client/discovery/host_locator.h"

namespace chromeos {
namespace smb_client {

namespace {

// Returns true if the host with |host_name| already exists in |host_map|.
bool HostExists(const HostMap& host_map, const Hostname& host_name) {
  return host_map.count(host_name);
}

}  // namespace

RequestInfo::RequestInfo(uint32_t remaining_requests,
                         FindHostsCallback callback)
    : remaining_requests(remaining_requests), callback(std::move(callback)) {}

RequestInfo::RequestInfo(RequestInfo&& other) = default;

RequestInfo::~RequestInfo() = default;

NetworkScanner::NetworkScanner() = default;

NetworkScanner::~NetworkScanner() = default;

void NetworkScanner::FindHostsInNetwork(FindHostsCallback callback) {
  if (locators_.empty()) {
    // Fire the callback immediately if there are no registered HostLocators.
    std::move(callback).Run(HostMap());
    return;
  }

  const uint32_t request_id = AddNewRequest(std::move(callback));
  for (const auto& locator : locators_) {
    locator->FindHosts(
        base::BindOnce(&NetworkScanner::OnHostsFound, AsWeakPtr(), request_id));
  }
}

void NetworkScanner::RegisterHostLocator(std::unique_ptr<HostLocator> locator) {
  locators_.push_back(std::move(locator));
}

void NetworkScanner::OnHostsFound(uint32_t request_id,
                                  const HostMap& host_map) {
  DCHECK_GT(requests_.count(request_id), 0u);

  AddHostsToResults(request_id, host_map);
  FireCallbackIfFinished(request_id);
}

void NetworkScanner::AddHostsToResults(uint32_t request_id,
                                       const HostMap& new_hosts) {
  auto request_iter = requests_.find(request_id);
  DCHECK(request_iter != requests_.end());

  HostMap& existing_hosts = request_iter->second.hosts_found;
  for (const auto& new_host : new_hosts) {
    const Hostname& new_hostname = new_host.first;
    const Address& new_ip = new_host.second;

    if (!HostExists(existing_hosts, new_hostname)) {
      existing_hosts.insert(new_host);
    } else if (existing_hosts[new_hostname] != new_ip) {
      LOG(WARNING) << "Different addresses found for host: " << new_hostname;
      LOG(WARNING) << existing_hosts[new_hostname] << ":" << new_ip;
    }
  }
}

uint32_t NetworkScanner::AddNewRequest(FindHostsCallback callback) {
  const uint32_t request_id = next_request_id_++;
  requests_.emplace(request_id,
                    RequestInfo(locators_.size(), std::move(callback)));
  return request_id;
}

void NetworkScanner::FireCallbackIfFinished(uint32_t request_id) {
  auto request_iter = requests_.find(request_id);
  DCHECK(request_iter != requests_.end());

  uint32_t& remaining_requests = request_iter->second.remaining_requests;
  DCHECK_GT(remaining_requests, 0u);

  if (--remaining_requests == 0) {
    RequestInfo info = std::move(request_iter->second);
    requests_.erase(request_iter);

    std::move(info.callback).Run(info.hosts_found);
  }
}

}  // namespace smb_client
}  // namespace chromeos
