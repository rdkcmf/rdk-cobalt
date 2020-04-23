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

#ifndef THIRD_PARTY_STARBOARD_WPE_SHARED_SYSTEM_SERVICES_H_
#define THIRD_PARTY_STARBOARD_WPE_SHARED_SYSTEM_SERVICES_H_

#include "starboard/configuration.h"
#include "starboard/common/scoped_ptr.h"

namespace third_party {
namespace starboard {
namespace wpe {
namespace shared {

struct ResolutionInfo {
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

}  // namespace shared
}  // namespace wpe
}  // namespace starboard
}  // namespace third_party

#endif  // THIRD_PARTY_STARBOARD_WPE_SHARED_SYSTEM_SERVICES_H_
