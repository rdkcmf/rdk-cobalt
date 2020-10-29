//
// Copyright 2020 Comcast Cable Communications Management, LLC
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
//
// SPDX-License-Identifier: Apache-2.0

#include "third_party/starboard/rdk/shared/rdkservices.h"

#include <string>
#include <algorithm>

#include <websocket/JSONRPCLink.h>

#include <interfaces/json/JsonData_HDRProperties.h>
#include <interfaces/json/JsonData_PlayerProperties.h>
#include <interfaces/json/JsonData_DeviceIdentification.h>

#ifdef HAS_SECURITY_AGENT
#include <securityagent/securityagent.h>
#endif

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
const char kPlayerInfoCallsign[] = "PlayerInfo.1";
const char kDeviceIdentificationCallsign[] = "DeviceIdentification.1";
const char kNetworkCallsign[] = "org.rdk.Network.1";

class ServiceLink {
  ::starboard::scoped_ptr<JSONRPC::LinkType<Core::JSON::IElement>> link_;
  std::string callsign_;

#ifdef HAS_SECURITY_AGENT
  static Core::OptionalType<std::string> getToken() {
    if (getenv("THUNDER_SECURITY_OFF") != nullptr)
      return { };

    const uint32_t kMaxBufferSize = 2 * 1024;
    const std::string payload = "https://www.youtube.com";

    Core::OptionalType<std::string> token;
    std::vector<uint8_t> buffer;
    buffer.resize(kMaxBufferSize);

    for(int i = 0; i < 5; ++i) {
      uint32_t inputLen = std::min(kMaxBufferSize, payload.length());
      ::memcpy (buffer.data(), payload.c_str(), inputLen);

      int outputLen = GetToken(kMaxBufferSize, inputLen, buffer.data());
      SB_DCHECK(outputLen != 0);

      if (outputLen > 0) {
        token = std::string(reinterpret_cast<const char*>(buffer.data()), outputLen);
        break;
      }
      else if (outputLen < 0) {
        uint32_t rc = -outputLen;
        if (rc == Core::ERROR_TIMEDOUT && i < 5) {
          SB_LOG(ERROR) << "Failed to get token, trying again. rc = " << rc << " ( " << Core::ErrorToString(rc) << " )";
          continue;
        }
        SB_LOG(ERROR) << "Failed to get token, give up. rc = " << rc << " ( " << Core::ErrorToString(rc) << " )";
      }
      break;
    }
    return token;
  }
#endif

  static std::string buildQuery() {
    std::string query;
#ifdef HAS_SECURITY_AGENT
    static const auto token = getToken();
    if (token.IsSet() && !token.Value().empty())
      query = "token=" + token.Value();
#endif
    return query;
  }

  static bool enableEnvOverrides() {
    static bool enable_env_overrides = ([]() {
      std::string envValue;
      if ((Core::SystemInfo::GetEnvironment("COBALT_ENABLE_OVERRIDES", envValue) == true) && (envValue.empty() == false)) {
        return envValue.compare("1") == 0 || envValue.compare("true") == 0;
      }
      return false;
    })();
    return enable_env_overrides;
  }

public:
  ServiceLink(const std::string callsign) : callsign_(callsign) {
    if (getenv("THUNDER_ACCESS") != nullptr)
      link_.reset(new JSONRPC::LinkType<Core::JSON::IElement>(callsign, nullptr, false, buildQuery()));
  }

  template <typename PARAMETERS>
  uint32_t Get(const uint32_t waitTime, const string& method, PARAMETERS& sendObject) {
    if (enableEnvOverrides()) {
      std::string envValue;
      std::string envName = Core::JSONRPC::Message::Callsign(callsign_) + "_" + method;
      envName.erase(std::remove(envName.begin(), envName.end(), '.'), envName.end());
      if (Core::SystemInfo::GetEnvironment(envName, envValue) == true) {
        return sendObject.FromString(envValue) ? Core::ERROR_NONE : Core::ERROR_GENERAL;
      }
    }
    if (!link_)
      return Core::ERROR_UNAVAILABLE;
    return link_->template Get<PARAMETERS>(waitTime, method, sendObject);
  }

  template <typename INBOUND, typename METHOD, typename REALOBJECT>
  uint32_t Subscribe(const uint32_t waitTime, const string& eventName, const METHOD& method, REALOBJECT* objectPtr) {
    if (!link_)
      return enableEnvOverrides() ? Core::ERROR_NONE : Core::ERROR_UNAVAILABLE;
    return link_->template Subscribe<INBOUND, METHOD, REALOBJECT>(waitTime, eventName, method, objectPtr);
  }

  void Unsubscribe(const uint32_t waitTime, const string& eventName) {
    if (!link_)
      return;
    return link_->Unsubscribe(waitTime, eventName);
  }
};

struct DeviceIdImpl {
  DeviceIdImpl() {
    JsonData::DeviceIdentification::DeviceidentificationData data;
    uint32_t rc = ServiceLink(kDeviceIdentificationCallsign)
      .Get(kDefaultTimeoutMs, "deviceidentification", data);
    if (Core::ERROR_NONE == rc) {
      chipset = data.Chipset.Value();
      firmware_version = data.Firmwareversion.Value();
      std::replace(chipset.begin(), chipset.end(), ' ', '-');
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

struct DisplayInfo::Impl {
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

  ServiceLink display_info_;
  ResolutionInfo resolution_info_ { };
  bool has_hdr_support_ { false };
  ::starboard::atomic_bool needs_refresh_ { true };
};

DisplayInfo::Impl::Impl()
  : display_info_(kDisplayInfoCallsign) {

  uint32_t rc;
  rc = display_info_.Subscribe<Core::JSON::String>(kDefaultTimeoutMs, "updated", &DisplayInfo::Impl::OnUpdated, this);
  if (Core::ERROR_NONE != rc) {
    needs_refresh_.store(false);
    SB_LOG(ERROR) << "Failed to subscribe to '" << kDisplayInfoCallsign
                  << ".updated' event, rc=" << rc
                  << " ( " << Core::ErrorToString(rc) << " )";
  }
  else {
    Refresh();
  }
}

DisplayInfo::Impl::~Impl() {
  display_info_.Unsubscribe(kDefaultTimeoutMs, "updated");
}

void DisplayInfo::Impl::Refresh() {
  if (!needs_refresh_.load())
    return;

  uint32_t rc;

  Core::JSON::EnumType<Exchange::IPlayerProperties::PlaybackResolution> resolution;
  rc = ServiceLink(kPlayerInfoCallsign).Get(kDefaultTimeoutMs, "resolution", resolution);
  if (Core::ERROR_NONE == rc) {
    switch(resolution) {
      case Exchange::IPlayerProperties::RESOLUTION_2160P30:
      case Exchange::IPlayerProperties::RESOLUTION_2160P60:
        resolution_info_ = ResolutionInfo { 3840 , 2160 };
        break;
      case Exchange::IPlayerProperties::RESOLUTION_1080I:
      case Exchange::IPlayerProperties::RESOLUTION_1080P:
      case Exchange::IPlayerProperties::RESOLUTION_UNKNOWN:
        resolution_info_ = ResolutionInfo { 1920 , 1080 };
        break;
      default:
        resolution_info_ = ResolutionInfo { 1280 , 720 };
        break;
    }
  } else {
    SB_LOG(ERROR) << "Failed to get 'resolution', rc=" << rc << " ( " << Core::ErrorToString(rc) << " )";
  }

  Core::JSON::EnumType<Exchange::IHDRProperties::HDRType> hdrsetting;
  rc = display_info_.Get(kDefaultTimeoutMs, "hdrsetting", hdrsetting);
  if (Core::ERROR_NONE == rc) {
    has_hdr_support_ = Exchange::IHDRProperties::HDR_OFF != hdrsetting;
  } else {
    SB_LOG(ERROR) << "Failed to get 'hdrsetting', rc=" << rc << " ( " << Core::ErrorToString(rc) << " )";
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

std::string DeviceIdentification::GetChipset() const {
  return GetDeviceIdImpl()->chipset;
}

std::string DeviceIdentification::GetFirmwareVersion() const {
  return GetDeviceIdImpl()->firmware_version;
}

bool NetworkInfo::IsConnectionTypeWireless() const {
  JsonObject data;
  uint32_t rc = ServiceLink(kNetworkCallsign).Get(kDefaultTimeoutMs, "getDefaultInterface", data);
  if (Core::ERROR_NONE == rc)
    return (0 == data.Get("interface").Value().compare("WIFI"));
  return false;
}

}  // namespace shared
}  // namespace rdk
}  // namespace starboard
}  // namespace third_party
