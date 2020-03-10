// Copyright 2017 The Cobalt Authors. All Rights Reserved.
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

#include <cstdio>
#include <cstring>
#include <string>

#if SB_API_VERSION >= 11
#include "starboard/format_string.h"
#endif  // SB_API_VERSION >= 11
#include "starboard/common/log.h"
#include "starboard/common/string.h"

namespace {

const char kPlatformName[] = "Linux";

bool CopyStringAndTestIfSuccess(char* out_value,
                                int value_length,
                                const char* from_value) {
  if (SbStringGetLength(from_value) + 1 > value_length)
    return false;
  SbStringCopy(out_value, from_value, value_length);
  return true;
}

bool TryReadModelNameFromPropertiesFile(char* out_value, int value_length) {
  const char kPrefixStr[] = "MODEL_NUM=";
  const size_t kPrefixStrLength = SB_ARRAY_SIZE(kPrefixStr) - 1;

  FILE* properties = fopen("/etc/device.properties", "r");
  if (!properties) {
    return false;
  }

  bool result = false;
  char* buffer = nullptr;
  size_t size = 0;

  while (getline(&buffer, &size, properties) != -1) {
    if (SbStringCompare(kPrefixStr, buffer, kPrefixStrLength) == 0) {
      char* remainder = buffer + kPrefixStrLength;
      size_t remainder_length = SbStringGetLength(remainder);
      if (remainder_length > 1 && remainder_length < value_length) {
        // trim the newline character
        for(int i = remainder_length - 1; i >= 0 && !std::isalnum(remainder[i]); --i)
          remainder[i] = '\0';
        SbStringCopy(out_value, remainder, remainder_length);
        result = true;
        break;
      }
    }
  }

  free(buffer);
  fclose(properties);

  return result;
}

bool GetModelName(char* out_value, int value_length) {
  const char* env = std::getenv("COBALT_MODEL_NAME");
  if (env && CopyStringAndTestIfSuccess(out_value, value_length, env))
    return true;

  if (TryReadModelNameFromPropertiesFile(out_value, value_length))
    return true;

  return CopyStringAndTestIfSuccess(out_value, value_length, SB_PLATFORM_MODEL_NAME);
}

bool GetBrandName(char* out_value, int value_length) {
  const char* env = std::getenv("COBALT_BRAND_NAME");
  if (env && CopyStringAndTestIfSuccess(out_value, value_length, env))
    return true;
  return CopyStringAndTestIfSuccess(out_value, value_length, SB_PLATFORM_BRAND_NAME);
}

}  // namespace

bool SbSystemGetProperty(SbSystemPropertyId property_id,
                         char* out_value,
                         int value_length) {
  if (!out_value || !value_length) {
    return false;
  }

  switch (property_id) {
    case kSbSystemPropertyModelName:
      return GetModelName(out_value, value_length);

    case kSbSystemPropertyBrandName:
      return GetBrandName(out_value, value_length);

#if defined(SB_PLATFORM_CHIPSET_MODEL_NUMBER_STRING)
    case kSbSystemPropertyChipsetModelNumber:
      return CopyStringAndTestIfSuccess(
            out_value, value_length, SB_PLATFORM_CHIPSET_MODEL_NUMBER_STRING);
#endif  // defined(SB_PLATFORM_CHIPSET_MODEL_NUMBER_STRING)

#if defined(SB_PLATFORM_FIRMWARE_VERSION_STRING)
    case kSbSystemPropertyFirmwareVersion:
      return CopyStringAndTestIfSuccess(out_value, value_length,
                                        SB_PLATFORM_FIRMWARE_VERSION_STRING);
#endif  // defined(SB_PLATFORM_FIRMWARE_VERSION_STRING)

#if defined(SB_PLATFORM_MODEL_YEAR)
    case kSbSystemPropertyModelYear:
      return CopyStringAndTestIfSuccess(out_value, value_length,
          std::to_string(SB_PLATFORM_MODEL_YEAR).c_str());
#endif  // defined(SB_PLATFORM_MODEL_YEAR)

#if defined(SB_PLATFORM_MANUFACTURER_NAME)
    case kSbSystemPropertyOriginalDesignManufacturerName:
      return CopyStringAndTestIfSuccess(out_value, value_length,
                                        SB_PLATFORM_MANUFACTURER_NAME);
#endif  // defined(SB_PLATFORM_MANUFACTURER_NAME)

    case kSbSystemPropertySpeechApiKey:
      return false;

#if defined(SB_PLATFORM_FRIENDLY_NAME)
    case kSbSystemPropertyFriendlyName:
      return CopyStringAndTestIfSuccess(out_value, value_length,
                                        SB_PLATFORM_FRIENDLY_NAME);
#endif  // defined(SB_PLATFORM_FRIENDLY_NAME)

    case kSbSystemPropertyPlatformName:
      return CopyStringAndTestIfSuccess(out_value, value_length, kPlatformName);

#if SB_API_VERSION >= 11
    case kSbSystemPropertyCertificationScope:
    case kSbSystemPropertyBase64EncodedCertificationSecret:
#endif  // SB_API_VERSION >= 11

    default:
      SB_DLOG(WARNING) << __FUNCTION__
                       << ": Unrecognized property: " << property_id;
      break;
  }

  return false;
}
