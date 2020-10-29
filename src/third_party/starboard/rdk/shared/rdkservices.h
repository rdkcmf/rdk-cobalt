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

#ifndef THIRD_PARTY_STARBOARD_RDK_SHARED_RDKSERVICES_H_
#define THIRD_PARTY_STARBOARD_RDK_SHARED_RDKSERVICES_H_

#include "starboard/configuration.h"
#include "starboard/common/scoped_ptr.h"

namespace third_party {
namespace starboard {
namespace rdk {
namespace shared {

struct ResolutionInfo {
  ResolutionInfo() {}
  ResolutionInfo(uint32_t w, uint32_t h)
    : Width(w), Height(h) {}
  uint32_t Width { 1920 };
  uint32_t Height { 1080 };
};

class DisplayInfo {
public:
  DisplayInfo();
  virtual ~DisplayInfo();
  ResolutionInfo GetResolution() const;
  bool HasHDRSupport() const;
private:
  struct Impl;
  mutable ::starboard::scoped_ptr<Impl> impl_;
};

class DeviceIdentification {
public:
  DeviceIdentification();
  virtual ~DeviceIdentification();
  std::string GetChipset() const;
  std::string GetFirmwareVersion() const;
};

}  // namespace shared
}  // namespace rdk
}  // namespace starboard
}  // namespace third_party

#endif  // THIRD_PARTY_STARBOARD_RDK_SHARED_RDKSERVICES_H_
