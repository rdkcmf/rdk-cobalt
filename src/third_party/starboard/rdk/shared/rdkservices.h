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

#ifndef THIRD_PARTY_STARBOARD_RDK_SHARED_RDKSERVICES_H_
#define THIRD_PARTY_STARBOARD_RDK_SHARED_RDKSERVICES_H_

#include "starboard/configuration.h"
#include "starboard/common/scoped_ptr.h"

#include <string>

struct SbAccessibilityCaptionSettings;
struct SbAccessibilityDisplaySettings;

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
  ~DisplayInfo();
  ResolutionInfo GetResolution() const;
  float GetDiagonalSizeInInches() const;
  bool HasHDRSupport() const;
private:
  struct Impl;
  mutable ::starboard::scoped_ptr<Impl> impl_;
};

class DeviceIdentification {
public:
  static std::string GetChipset();
  static std::string GetFirmwareVersion();
};

class NetworkInfo {
public:
  static bool IsConnectionTypeWireless();
};

class TextToSpeech {
public:
  static bool IsEnabled();
  static void Speak(const std::string& text);
  static void Cancel();
};

class Accessibility {
public:
  static void SetSettings(const std::string& json);
  static bool GetSettings(std::string& out_json);
  static bool GetCaptionSettings(SbAccessibilityCaptionSettings* out);
  static bool GetDisplaySettings(SbAccessibilityDisplaySettings* out);
};

class SystemProperties {
public:
  static void SetSettings(const std::string& json);
  static bool GetSettings(std::string& out_json);
  static bool GetChipset(std::string &out);
  static bool GetFirmwareVersion(std::string &out);
  static bool GetIntegratorName(std::string &out);
  static bool GetBrandName(std::string &out);
  static bool GetModelName(std::string &out);
  static bool GetModelYear(std::string &out);
  static bool GetFriendlyName(std::string &out);
};

}  // namespace shared
}  // namespace rdk
}  // namespace starboard
}  // namespace third_party

#endif  // THIRD_PARTY_STARBOARD_RDK_SHARED_RDKSERVICES_H_
