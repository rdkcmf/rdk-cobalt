// Copyright 2016 The Cobalt Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "starboard/system.h"
#include "starboard/string.h"

#include <core/Enumerate.h>

#include "third_party/starboard/rdk/shared/rdkservices.h"
#include "third_party/starboard/rdk/shared/log_override.h"

using namespace third_party::starboard::rdk::shared;

namespace WPEFramework {

ENUM_CONVERSION_HANDLER(SbSystemDeviceType);

ENUM_CONVERSION_BEGIN(SbSystemDeviceType)
  { kSbSystemDeviceTypeBlueRayDiskPlayer, _TXT("BlueRayDiskPlayer") },
  { kSbSystemDeviceTypeGameConsole,       _TXT("GameConsole") },
  { kSbSystemDeviceTypeOverTheTopBox,     _TXT("OverTheTopBox") },
  { kSbSystemDeviceTypeSetTopBox,         _TXT("SetTopBox") },
  { kSbSystemDeviceTypeTV,                _TXT("TV") },
  { kSbSystemDeviceTypeDesktopPC,         _TXT("DesktopPC") },
  { kSbSystemDeviceTypeAndroidTV,         _TXT("AndroidTV") },
  { kSbSystemDeviceTypeUnknown,           _TXT("Unknown") },
ENUM_CONVERSION_END(SbSystemDeviceType);

}

SbSystemDeviceType SbSystemGetDeviceType() {
  std::string prop;
  if (SystemProperties::GetDeviceType(prop)) {
    WPEFramework::Core::EnumerateType<SbSystemDeviceType> converted(prop.c_str(), false);
    if (converted.IsSet() == true) {
      SB_LOG(INFO) << "DeviceType: '" << converted.Data() << "'";
      return converted.Value();
    } else {
      SB_LOG(ERROR) << "Failed to parse device type from '" << prop << "', fallback to report STB";
    }
  }
  // try to deduce from experience flag
  if (AuthService::GetExperience(prop)) {
    if (prop == "Flex") {
      SB_LOG(INFO) << "DeviceType: 'OverTheTopBox' for '" << prop << "'";
      return kSbSystemDeviceTypeOverTheTopBox;
    }
  }
  SB_LOG(INFO) << "DeviceType: 'SetTopBox'";
  return kSbSystemDeviceTypeSetTopBox;
}
