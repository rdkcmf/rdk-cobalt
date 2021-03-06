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

//
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

#ifndef THIRD_PARTY_STARBOARD_RDK_SHARED_WINDOW_INTERNAL_H_
#define THIRD_PARTY_STARBOARD_RDK_SHARED_WINDOW_INTERNAL_H_

#include "base/macros.h"
#include "starboard/event.h"
#include "starboard/time.h"
#include "starboard/window.h"

struct SbWindowPrivate {
  SbWindowPrivate(const SbWindowOptions* options);
  ~SbWindowPrivate();

  int Width() const;
  int Height() const;
  void* Native() const;
  float VideoPixelRatio() const;
  float DiagonalSizeInInches() const;
};

#endif  // THIRD_PARTY_STARBOARD_RDK_SHARED_WINDOW_INTERNAL_H_
