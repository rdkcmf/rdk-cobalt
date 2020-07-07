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
//
// Copyright 2018 The Cobalt Authors. All Rights Reserved.
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

#include "starboard/player.h"

#include "third_party/starboard/rdk/shared/player/player_internal.h"

#if SB_API_VERSION >= 10
void SbPlayerSeek2(SbPlayer player, SbTime seek_to_timestamp, int ticket) {
  player->player_->Seek(seek_to_timestamp, ticket);
}
#else
void SbPlayerSeek(SbPlayer /*player*/,
                  SbMediaTime /*seek_to_timestamp*/,
                  int /*ticket*/) {
  SB_NOTIMPLEMENTED();
}
#endif  // SB_API_VERSION >= 10