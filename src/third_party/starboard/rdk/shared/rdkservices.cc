// If not stated otherwise in this file or this component's license file the
// following copyright and licenses apply:
//
// Copyright 2020 RDK Management
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "third_party/starboard/rdk/shared/rdkservices.h"

#include <string>

#include <websocket/JSONRPCLink.h>

#include "starboard/atomic.h"
#include "starboard/common/log.h"
#include "starboard/once.h"

using namespace  WPEFramework;

namespace third_party {
namespace starboard {
namespace rdk {
namespace shared {

namespace {

const uint32_t kDefaultTimeoutMs = 100;
const char kDisplayInfoCallsign[] = "DisplayInfo.1";
const char kDeviceIdentificationCalllsign[] = "DeviceIdentification.1";

using ClientType = JSONRPC::LinkType<Core::JSON::IElement>;
auto makeClient(const std::string& callsign) -> ::starboard::scoped_ptr<ClientType> {
  return ::starboard::scoped_ptr<ClientType>(new ClientType(callsign));
}

struct DeviceIdImpl final {
  DeviceIdImpl() {
    uint32_t rc = Core::ERROR_UNAVAILABLE;
    auto client = makeClient(kDeviceIdentificationCalllsign);
    if (client) {
      JsonObject data;
      rc = client->Get<JsonObject>(kDefaultTimeoutMs, "deviceidentification", data);
      if (Core::ERROR_NONE == rc) {
        chipset = data.Get("chipset").Value();
        firmware_version = data.Get("firmwareversion").Value();
      }
    }
    if (Core::ERROR_NONE != rc) {
      #if defined(SB_PLATFORM_CHIPSET_MODEL_NUMBER_STRING)
      chipset = SB_PLATFORM_CHIPSET_MODEL_NUMBER_STRING;
      #endif
      #if defined(SB_PLATFORM_FIRMWARE_VERSION_STRING)
      firmware_version = SB_PLATFORM_FIRMWARE_VERSION_STRING;
      #endif
    }
  }
  std::string chipset;
  std::string firmware_version;
};

SB_ONCE_INITIALIZE_FUNCTION(DeviceIdImpl, GetDeviceIdImpl);

}  // namespace

struct DisplayInfo::Impl final {
  Impl();
  ~Impl();
  ResolutionInfo GetResolution() {
    Refresh();
    return resolution_info_;
  }
  bool HasHDRSupport() {
    Refresh();
    return has_hdr_support_;
  }
private:
  void Refresh();
  void OnUpdated(const Core::JSON::String&);

  ::starboard::scoped_ptr<ClientType> client_;
  ResolutionInfo resolution_info_ { };
  bool has_hdr_support_ { false };
  ::starboard::atomic_bool needs_refresh_ { true };
};

DisplayInfo::Impl::Impl()
  : client_(makeClient(kDisplayInfoCallsign)) {

  uint32_t rc = Core::ERROR_UNAVAILABLE;

  if (client_)
    rc = client_->Subscribe<Core::JSON::String>(kDefaultTimeoutMs, "updated", &DisplayInfo::Impl::OnUpdated, this);

  if (Core::ERROR_NONE != rc) {
    needs_refresh_.store(false);
    SB_LOG(ERROR) << "Failed to subscribe to '" << kDisplayInfoCallsign << ".updated' event, rc=" << rc;
  }
  else {
    Refresh();
  }
}

DisplayInfo::Impl::~Impl() {
  if (client_)
    client_->Unsubscribe(kDefaultTimeoutMs, "updated");
}

void DisplayInfo::Impl::Refresh() {
  if (!needs_refresh_.load() || !client_)
    return;

  JsonObject data;
  uint32_t rc;

  rc = client_->Get<JsonObject>(kDefaultTimeoutMs, "displayinfo", data);

  if (Core::ERROR_NONE == rc) {
    resolution_info_.Width = data.Get("width").Number();
    resolution_info_.Height = data.Get("height").Number();
    Core::JSON::String hdrtype = data.Get("hdrtype");
    has_hdr_support_ = !hdrtype.IsNull() && !hdrtype.Value().empty() && hdrtype.Value().compare("HDROff") != 0;

    // FIXME: for some reason device info returns inverted values on Amlogic(see AMLOGIC-629)
    if (resolution_info_.Height > resolution_info_.Width)
        std::swap(resolution_info_.Height, resolution_info_.Width);
  } else {
    SB_LOG(ERROR) << "Failed to get 'displayinfo', rc=" << rc;
  }

  needs_refresh_.store(false);
}

void DisplayInfo::Impl::OnUpdated(const Core::JSON::String&) {
  needs_refresh_.store(true);
}

DisplayInfo::DisplayInfo() : impl_(new Impl) {
}

DisplayInfo::~DisplayInfo() {
}

ResolutionInfo DisplayInfo::GetResolution() const {
  return impl_->GetResolution();
}

bool DisplayInfo::HasHDRSupport() const {
  return impl_->HasHDRSupport();
}

DeviceIdentification::DeviceIdentification() = default;
DeviceIdentification::~DeviceIdentification() = default;

std::string DeviceIdentification::GetChipset() const {
  return GetDeviceIdImpl()->chipset;
}

std::string DeviceIdentification::GetFirmwareVersion() const {
  return GetDeviceIdImpl()->firmware_version;
}

}  // namespace shared
}  // namespace rdk
}  // namespace starboard
}  // namespace third_party
