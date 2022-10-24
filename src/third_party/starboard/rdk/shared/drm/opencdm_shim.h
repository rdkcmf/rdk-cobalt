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
#pragma once

#include <stdint.h>
#include <string.h>
#include <stdio.h>

// #include <opencdm/open_cdm.h>
// #include <opencdm/open_cdm_adapter.h>

#ifdef __cplusplus
extern "C" {
#endif

struct OpenCDMSystem;
struct OpenCDMSession;

typedef enum {
    Temporary = 0,
    PersistentUsageRecord,
    PersistentLicense
} LicenseType;

typedef enum {
    Usable = 0,
    Expired,
    Released,
    OutputRestricted,
    OutputRestrictedHDCP22,
    OutputDownscaled,
    StatusPending,
    InternalError,
    HWError
} KeyStatus;

typedef enum {
    ERROR_NONE = 0,
    ERROR_UNKNOWN = 1,
    ERROR_MORE_DATA_AVAILBALE=2,
    ERROR_INVALID_ACCESSOR = 0x80000001,
    ERROR_KEYSYSTEM_NOT_SUPPORTED = 0x80000002,
    ERROR_INVALID_SESSION = 0x80000003,
    ERROR_INVALID_DECRYPT_BUFFER = 0x80000004,
    ERROR_OUT_OF_MEMORY = 0x80000005,
    ERROR_FAIL = 0x80004005,
    ERROR_INVALID_ARG = 0x80070057,
    ERROR_SERVER_INTERNAL_ERROR = 0x8004C600,
    ERROR_SERVER_INVALID_MESSAGE = 0x8004C601,
    ERROR_SERVER_SERVICE_SPECIFIC = 0x8004C604,
} OpenCDMError;

typedef enum {
    OPENCDM_BOOL_FALSE = 0,
    OPENCDM_BOOL_TRUE = 1
} OpenCDMBool;

typedef struct {
    void (*process_challenge_callback)(struct OpenCDMSession* session, void* userData, const char url[], const uint8_t challenge[], const uint16_t challengeLength);

    void (*key_update_callback)(struct OpenCDMSession* session, void* userData, const uint8_t keyId[], const uint8_t length);

    void (*error_message_callback)(struct OpenCDMSession* session, void* userData, const char message[]);

    void (*keys_updated_callback)(const struct OpenCDMSession* session, void* userData);
} OpenCDMSessionCallbacks;

struct OpenCDMSystem* opencdm_create_system(const char keySystem[]);

OpenCDMError opencdm_destruct_system(struct OpenCDMSystem* system);

OpenCDMError opencdm_is_type_supported(const char keySystem[],
    const char mimeType[]);

struct OpenCDMSession* opencdm_get_system_session(struct OpenCDMSystem* system, const uint8_t keyId[],
    const uint8_t length, const uint32_t waitTime);

OpenCDMError opencdm_system_set_server_certificate(
    struct OpenCDMSystem* system,
    const uint8_t serverCertificate[], const uint16_t serverCertificateLength);

OpenCDMError opencdm_construct_session(struct OpenCDMSystem* system, const LicenseType licenseType,
    const char initDataType[], const uint8_t initData[], const uint16_t initDataLength,
    const uint8_t CDMData[], const uint16_t CDMDataLength, OpenCDMSessionCallbacks* callbacks, void* userData,
    struct OpenCDMSession** session);

OpenCDMError opencdm_destruct_session(struct OpenCDMSession* session);

OpenCDMError opencdm_session_update(struct OpenCDMSession* session,
    const uint8_t keyMessage[],
    const uint16_t keyLength);

const char* opencdm_session_id(const struct OpenCDMSession* session);

KeyStatus opencdm_session_status(const struct OpenCDMSession* session,
    const uint8_t keyId[], const uint8_t length);

OpenCDMError opencdm_session_close(struct OpenCDMSession* session);

struct _GstBuffer;
typedef struct _GstBuffer GstBuffer;
struct _GstCaps;
typedef struct _GstCaps GstCaps;

OpenCDMError opencdm_gstreamer_session_decrypt_ex(
  struct OpenCDMSession* session, GstBuffer* buffer, GstBuffer* subSample, const uint32_t subSampleCount,
  GstBuffer* IV, GstBuffer* keyID, uint32_t initWithLast15, GstCaps* caps);

#ifdef __cplusplus
}
#endif
