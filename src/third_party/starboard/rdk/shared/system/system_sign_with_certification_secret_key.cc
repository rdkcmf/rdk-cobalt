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
// Copyright 2019 The Cobalt Authors. All Rights Reserved.
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

#include <memory>
#include <vector>

#include "starboard/system.h"
#include "third_party/starboard/rdk/shared/log_override.h"

#if defined(HAS_CRYPTOGRAPHY)
#include <cryptography/cryptography.h>
#endif

namespace {

bool ReadFile(const char* filename, std::vector<uint8_t> &buf) {
  long sz = -1;
  FILE* f = filename ? fopen(filename, "r") : nullptr;
  if ( f ) {
    fseek(f, 0, SEEK_END);
    sz =  ftell(f);
    fseek(f, 0, SEEK_SET);
    if ( sz > 0 ) {
      buf.resize(sz);
      fread(buf.data(), 1, buf.size(), f);
    }
    fclose(f);
  }
  return sz > 0;
}

template<typename T>
struct RefDeleter {
  void operator()(T* ref) { if ( ref ) ref->Release(); }
};

template<typename T>
using ScopedRef = std::unique_ptr<T, RefDeleter<T>>;

}

bool SbSystemSignWithCertificationSecretKey(const uint8_t* message,
                                            size_t message_size_in_bytes,
                                            uint8_t* digest,
                                            size_t digest_size_in_bytes) {
  bool result = false;

#if defined(HAS_CRYPTOGRAPHY)
  using namespace WPEFramework::Cryptography;

  const char *key_name = std::getenv("COBALT_CERT_KEY_NAME");
  const char *key_file = std::getenv("COBALT_CERT_KEY_FILE");
  if ( key_name == nullptr && key_file == nullptr )
    return false;

  ScopedRef<ICryptography> icrypto;
  ScopedRef<IVault> vault;
  ScopedRef<IHash> hash;

  icrypto.reset( ICryptography::Instance(EMPTY_STRING) );
  if ( !icrypto ) {
    SB_LOG(ERROR) << "Failed to create ICryptography instance";
    return false;
  }

  vault.reset( icrypto->Vault(cryptographyvault::CRYPTOGRAPHY_VAULT_DEFAULT) );
  if ( !vault ) {
    SB_LOG(ERROR) << "Failed to get default vault";
    return false;
  }

  auto ImportKey = [](IVault *vault, const char* key_name, const char* key_file) -> uint32_t
  {
    if ( key_name )
      return vault->ImportNamedKey(key_name);

    std::vector<uint8_t> key_buf;
    if ( !ReadFile( key_file, key_buf ) ) {
      SB_LOG(ERROR) << "Failed to read key file: " << key_file;
      return 0u;
    }
    return vault->Import(key_buf.size(), key_buf.data());
  };

  uint32_t key_id, rc;

  if ( (key_id = ImportKey( vault.get(), key_name, key_file )) == 0 ) {
    SB_LOG(ERROR) << "Failed to import key";
    return false;
  } else {
    hash.reset( vault->HMAC(hashtype::SHA256, key_id) );
  }

  if ( !hash ) {
    SB_LOG(ERROR) << "Vault returned null HMAC for key id: 0x" << std::hex << key_id;
  }
  else if ( (rc = hash->Ingest( message_size_in_bytes, message )) != message_size_in_bytes ) {
    SB_LOG(ERROR) << "HMAC 'Ingest' failed, rc: " << rc << " message size: " << message_size_in_bytes;
  }
  else if ( (rc = hash->Calculate( digest_size_in_bytes, digest )) != digest_size_in_bytes ) {
    SB_LOG(ERROR) << "HMAC 'Calculate' failed, rc: " << rc << " digest size: " << digest_size_in_bytes;
  }
  else {
    result = true;
  }
#endif

  return result;
}
