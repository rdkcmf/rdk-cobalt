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

#include "third_party/starboard/rdk/shared/drm/opencdm_shim.h"

#include <dlfcn.h>
#include <mutex>
#include <cstring>

#include "third_party/starboard/rdk/shared/log_override.h"

using OcdmGstSessionDecryptExFn =
  OpenCDMError(*)(struct OpenCDMSession*, GstBuffer*, GstBuffer*, const uint32_t, GstBuffer*, GstBuffer*, uint32_t, GstCaps*);

using OcdmGstSessionDecryptFn =
  OpenCDMError(*)(struct OpenCDMSession*, GstBuffer*, GstBuffer*, const uint32_t, GstBuffer*, GstBuffer*, uint32_t);

using OcdmConstructSessionFn =
  OpenCDMError(*)(struct OpenCDMSystem*, const LicenseType,
    const char[], const uint8_t[], const uint16_t,
    const uint8_t[], const uint16_t , OpenCDMSessionCallbacks* , void* ,
    struct OpenCDMSession**);

using OcdmDestructSessionFn =
  OpenCDMError(*)(struct OpenCDMSession*);

using OcdmSessionStatusFn =
  KeyStatus(*)(const struct OpenCDMSession* , const uint8_t[], const uint8_t);

using OcdmSessionCloseFn =
  OpenCDMError(*)(struct OpenCDMSession*);

using OcdmSessionIdFn =
  const char* (*)(const struct OpenCDMSession*);

using OcdmSessionUpdateFn =
  OpenCDMError(*)(struct OpenCDMSession*,
                  const uint8_t [],
                  const uint16_t);

using OcdmCreateSystemFn =
  struct OpenCDMSystem*(*)(const char keySystem[]);

using OcdmDestructSystemFn =
  OpenCDMError (*)(struct OpenCDMSystem* system);

using OcdmGetSystemSessionFn =
  struct OpenCDMSession* (*)(struct OpenCDMSystem* system, const uint8_t keyId[],
                             const uint8_t length, const uint32_t waitTime);

using OsdmSystemSetServerCertificateFn =
  OpenCDMError (*)(struct OpenCDMSystem* , const uint8_t [], const uint16_t );

using OcdmIsTypeSupportedFn =
  OpenCDMError (*)(const char [], const char []);

struct OcdmShim
{
  OcdmCreateSystemFn        create_system { nullptr };
  OcdmDestructSystemFn      destruct_system { nullptr };
  OcdmSessionIdFn           session_id { nullptr };
  OcdmSessionUpdateFn       session_update { nullptr };
  OcdmSessionStatusFn       session_status { nullptr };
  OcdmSessionCloseFn        session_close { nullptr };
  OcdmConstructSessionFn    construct_session { nullptr };
  OcdmDestructSessionFn     destruct_session { nullptr };
  OcdmGetSystemSessionFn    get_system_session { nullptr };
  OcdmGstSessionDecryptFn   gstreamer_session_decrypt    { nullptr };
  OcdmGstSessionDecryptExFn gstreamer_session_decrypt_ex { nullptr };
  OcdmIsTypeSupportedFn     is_type_supported { nullptr };
  OsdmSystemSetServerCertificateFn system_set_server_certificate { nullptr };
};

static OcdmShim& ocdmShim() {
  static OcdmShim g_shim;
  static std::once_flag flag;
  std::call_once(flag, [](){
    static const char kOCDMLibName[] = "libocdm.so";

    void *handle = nullptr;

    if (getenv("OCDM_LIBRARY"))
      handle = dlopen(getenv("OCDM_LIBRARY"), RTLD_LAZY);

    if (!handle)
      handle = dlopen(kOCDMLibName, RTLD_LAZY);

#define LOAD_FN(fn_name, fn_type)                                     \
    do {                                                              \
      g_shim.fn_name = (fn_type) dlsym(handle, "opencdm_" #fn_name);  \
      if (g_shim.fn_name == nullptr) {                                \
        SB_LOG(ERROR) << "Could not load 'opencdm_" #fn_name "'" ;    \
        SB_CHECK(false);                                              \
      }                                                               \
    } while (0)

    LOAD_FN(create_system                 , OcdmCreateSystemFn);
    LOAD_FN(create_system                 , OcdmCreateSystemFn);
    LOAD_FN(destruct_system               , OcdmDestructSystemFn);
    LOAD_FN(session_id                    , OcdmSessionIdFn);
    LOAD_FN(session_update                , OcdmSessionUpdateFn);
    LOAD_FN(session_status                , OcdmSessionStatusFn);
    LOAD_FN(session_close                 , OcdmSessionCloseFn);
    LOAD_FN(construct_session             , OcdmConstructSessionFn);
    LOAD_FN(destruct_session              , OcdmDestructSessionFn);
    LOAD_FN(get_system_session            , OcdmGetSystemSessionFn);
    LOAD_FN(is_type_supported             , OcdmIsTypeSupportedFn);
    LOAD_FN(system_set_server_certificate , OsdmSystemSetServerCertificateFn);

#undef LOAD_FN

    g_shim.gstreamer_session_decrypt_ex = (OcdmGstSessionDecryptExFn) dlsym(handle, "opencdm_gstreamer_session_decrypt_ex");
    if (!g_shim.gstreamer_session_decrypt_ex) {
      SB_LOG(INFO) << "Could not load opencdm_gstreamer_session_decrypt_ex, trying opencdm_gstreamer_session_decrypt";
      g_shim.gstreamer_session_decrypt = (OcdmGstSessionDecryptFn) dlsym(handle, "opencdm_gstreamer_session_decrypt");
      if (!g_shim.gstreamer_session_decrypt) {
        SB_LOG(ERROR) << "Could not load opencdm_gstreamer_session_decrypt, giving up...";
        SB_CHECK(false);
      }
    }
    else {
      SB_LOG(INFO) << "Has opencdm_gstreamer_session_decrypt_ex";
    }

  });

  return g_shim;
}

OpenCDMError opencdm_gstreamer_session_decrypt_ex(
  struct OpenCDMSession* session, GstBuffer* buffer, GstBuffer* subSample, const uint32_t subSampleCount,
  GstBuffer* IV, GstBuffer* keyID, uint32_t initWithLast15, GstCaps* caps)
{
  if (ocdmShim().gstreamer_session_decrypt_ex)
    return ocdmShim().gstreamer_session_decrypt_ex(
      session, buffer,
      subSample, subSampleCount, IV,
      keyID, initWithLast15, caps);

  return ocdmShim().gstreamer_session_decrypt(
      session, buffer,
      subSample, subSampleCount, IV,
      keyID, initWithLast15);
}

OpenCDMError opencdm_construct_session(struct OpenCDMSystem* system, const LicenseType licenseType,
    const char initDataType[], const uint8_t initData[], const uint16_t initDataLength,
    const uint8_t CDMData[], const uint16_t CDMDataLength, OpenCDMSessionCallbacks* callbacks, void* userData,
    struct OpenCDMSession** session)
{
  return ocdmShim().construct_session(system, licenseType, initDataType, initData, initDataLength, CDMData, CDMDataLength, callbacks, userData, session);
}

KeyStatus opencdm_session_status(const struct OpenCDMSession* session,
    const uint8_t keyId[], const uint8_t length)
{
  return ocdmShim().session_status(session, keyId, length);
}

const char* opencdm_session_id(const struct OpenCDMSession* session)
{
  return ocdmShim().session_id(session);
}

OpenCDMError opencdm_session_update(struct OpenCDMSession* session,
    const uint8_t keyMessage[],
    const uint16_t keyLength)
{
  return ocdmShim().session_update(session, keyMessage, keyLength);
}

OpenCDMError opencdm_session_close(struct OpenCDMSession* session)
{
  return ocdmShim().session_close(session);
}

OpenCDMError opencdm_destruct_session(struct OpenCDMSession* session)
{
  return ocdmShim().destruct_session(session);
}

struct OpenCDMSystem* opencdm_create_system(const char keySystem[])
{
  return ocdmShim().create_system(keySystem);
}

OpenCDMError opencdm_destruct_system(struct OpenCDMSystem* system)
{
  return ocdmShim().destruct_system(system);
}

struct OpenCDMSession* opencdm_get_system_session(struct OpenCDMSystem* system, const uint8_t keyId[],
    const uint8_t length, const uint32_t waitTime)
{
  return ocdmShim().get_system_session(system, keyId, length, waitTime);
}

OpenCDMError opencdm_is_type_supported(const char keySystem[], const char mimeType[])
{
  return ocdmShim().is_type_supported(keySystem, mimeType);
}

OpenCDMError opencdm_system_set_server_certificate(
    struct OpenCDMSystem* system,
    const uint8_t serverCertificate[],
    const uint16_t serverCertificateLength)
{
  return ocdmShim().system_set_server_certificate(
    system, serverCertificate, serverCertificateLength);
}
