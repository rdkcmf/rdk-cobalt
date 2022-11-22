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
// SPDX-License-Identifier: Apache-2.0//
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

#include "third_party/starboard/rdk/shared/platform_service.h"

#include <memory>
#include <string>

#include "cobalt/extension/platform_service.h"
#include "starboard/common/log.h"
#include "starboard/common/string.h"
#include "starboard/configuration.h"
#include "starboard/shared/starboard/application.h"

#include "third_party/starboard/rdk/shared/firebolt/firebolt.h"
#include "third_party/starboard/rdk/shared/log_override.h"

#include <core/JSON.h>

const char kCobaltExtensionContentEntitlementName[] = "com.google.youtube.tv.ContentEntitlement";

typedef struct CobaltExtensionPlatformServicePrivate {
  void* context;
  ReceiveMessageCallback receive_callback;
  CobaltExtensionPlatformServicePrivate(void* context, ReceiveMessageCallback receive_callback)
    : context(context), receive_callback(receive_callback) {};
  virtual ~CobaltExtensionPlatformServicePrivate() = default;
  virtual void* Send(void* data, uint64_t data_length, uint64_t* output_length, bool* invalid_state) = 0;
} CobaltExtensionPlatformServicePrivate;

namespace third_party {
namespace starboard {
namespace rdk {
namespace shared {

namespace {

using namespace WPEFramework;

struct ContentEntitlementCobaltExtensionPlatformService : public CobaltExtensionPlatformServicePrivate {
  struct ContentEntitlementUpdate : Core::JSON::Container {
    struct Entitlement : Core::JSON::Container {
      Entitlement()
        : Core::JSON::Container() {
        Init();
      }
      Entitlement(const Entitlement& other)
        : Core::JSON::Container()
        , id(other.id) {
        Init();
      }
      Entitlement& operator=(const Entitlement& rhs) {
        id = rhs.id;
        return (*this);
      }
      Core::JSON::String id;
    private:
      void Init() {
        Add(_T("id"), &id);
      }
    };
    struct Payload : Core::JSON::Container {
      Payload()
        : Core::JSON::Container() {
        Add(_T("entitlements"), &entitlements);
      }
      Core::JSON::ArrayType<Entitlement> entitlements;
    };
    ContentEntitlementUpdate()
      : Core::JSON::Container() {
      Add(_T("message"), &message);
      Add(_T("payload"), &payload);
    }
    Core::JSON::String message;
    Payload payload;
  };

  ContentEntitlementCobaltExtensionPlatformService(void* context, ReceiveMessageCallback callback)
    : CobaltExtensionPlatformServicePrivate(context, callback) {
  }

  void* Send(void* data, uint64_t length, uint64_t* output_length, bool* invalid_state) override {
    bool result = false;

    if (data && length) {
      ContentEntitlementUpdate update;
      std::string json_string(static_cast<const char*>(data), length);

      if(!update.FromString(json_string)) {
        SB_LOG(ERROR) << "Failed to parse ContentEntitlement message";
      }
      else if(update.message == "updateEntitlements") {
        const auto &payload_entitlements = update.payload.entitlements;

        std::vector<firebolt::Entitlement> entitlements;
        for(int i = 0; i < payload_entitlements.Length(); ++i)
          entitlements.push_back({payload_entitlements[i].id.Value()});

        firebolt::Discovery discovery;
        result = discovery.entitlements(entitlements);
      }
    }

    bool* ptr = reinterpret_cast<bool*>(SbMemoryAllocate(sizeof(bool)));
    *ptr = result;
    return static_cast<void*>(ptr);
  }
};

bool Has(const char* name) {
  // Check if platform has service name.
  bool result = strcmp(name, kCobaltExtensionContentEntitlementName) == 0 && firebolt::IsAvailable();
  SB_LOG(INFO) << "Entitlement Has service called " << name << " result = " << result;
  return result;
}

CobaltExtensionPlatformService Open(void* context,
                                    const char* name,
                                    ReceiveMessageCallback receive_callback) {
  SB_DCHECK(context);

  CobaltExtensionPlatformService service;

  if (strcmp(name, kCobaltExtensionContentEntitlementName) == 0 && firebolt::IsAvailable()) {
    SB_LOG(INFO) << "Open() service created: " << name;
    service =
      new ContentEntitlementCobaltExtensionPlatformService(context, receive_callback);
  } else {
    SB_LOG(ERROR) << "Open() service name does not exist: " << name;
    service = kCobaltExtensionPlatformServiceInvalid;
  }

  delete[] name;

  return service;
}

void Close(CobaltExtensionPlatformService service) {
  SB_LOG(INFO) << "Close() Service.";
  delete static_cast<CobaltExtensionPlatformServicePrivate*>(service);
}

void* Send(CobaltExtensionPlatformService service,
           void* data,
           uint64_t length,
           uint64_t* output_length,
           bool* invalid_state) {
  SB_DCHECK(data);
  SB_DCHECK(length);
  SB_DCHECK(output_length);
  SB_DCHECK(invalid_state);
  return static_cast<CobaltExtensionPlatformServicePrivate*>(service)->Send(data, length, output_length, invalid_state);
}

const CobaltExtensionPlatformServiceApi kPlatformServiceApi = {
  kCobaltExtensionPlatformServiceName,
  1,  // API version that's implemented.
  &Has,
  &Open,
  &Close,
  &Send
};

}  // namespace

const void* GetPlatformServiceApi() {
  SB_LOG(INFO) << "GetContentEntitlementPlatformServiceApi return ";
  return &kPlatformServiceApi;
}

}  // namespace thirdpary
}  // namespace starboard
}  // namespace rdk
}  // namespace shared
