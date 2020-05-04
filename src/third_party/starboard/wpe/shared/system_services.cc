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

#include "third_party/starboard/wpe/shared/system_services.h"

#include <string>

#include <WPEFramework/websocket/JSONRPCLink.h>

#include "starboard/atomic.h"
#include "starboard/common/log.h"

using namespace  WPEFramework;

namespace third_party {
namespace starboard {
namespace wpe {
namespace shared {

namespace {

const uint32_t kDefaultTimeoutMs = 100;
const char kDisplayInfoCallsign[] = "DisplayInfo.1";

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

  JSONRPC::LinkType<Core::JSON::IElement> client_;
  ResolutionInfo resolution_info_ { };
  bool has_hdr_support_ { false };
  ::starboard::atomic_bool needs_refresh_ { true };
};

DisplayInfo::Impl::Impl()
  : client_(kDisplayInfoCallsign) {

  uint32_t rc;
  rc = client_.Subscribe<Core::JSON::String>(kDefaultTimeoutMs, "updated", &DisplayInfo::Impl::OnUpdated, this);
  if (Core::ERROR_NONE != rc) {
    needs_refresh_.store(false);
    SB_LOG(ERROR) << "Failed to subscribe to '" << kDisplayInfoCallsign << ".updated' event, rc=" << rc;
  }
  else {
    Refresh();
  }
}

DisplayInfo::Impl::~Impl() {
  client_.Unsubscribe(kDefaultTimeoutMs, "updated");
}

void DisplayInfo::Impl::Refresh() {
  if (!needs_refresh_.load())
    return;

  JsonObject data;
  uint32_t rc;

  rc = client_.Get<JsonObject>(kDefaultTimeoutMs, "displayinfo", data);

  if (Core::ERROR_NONE == rc) {
    resolution_info_.Width = data.Get("width").Number();
    resolution_info_.Height = data.Get("height").Number();
    Core::JSON::String hdrtype = data.Get("hdrtype");
    has_hdr_support_ = !hdrtype.IsNull() && !hdrtype.Value().empty() && hdrtype.Value().compare("HDROff") != 0;
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

}  // namespace shared
}  // namespace wpe
}  // namespace starboard
}  // namespace third_party
