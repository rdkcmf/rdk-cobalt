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

#include <gst/gst.h>

#include <signal.h>

#include "starboard/configuration.h"
#include "starboard/shared/signal/crash_signals.h"
#include "starboard/shared/signal/suspend_signals.h"

#include "third_party/starboard/rdk/shared/application_rdk.h"

#if SB_IS(EVERGREEN_COMPATIBLE)
#include "third_party/crashpad/wrapper/wrapper.h"
#endif

namespace third_party {
namespace starboard {
namespace rdk {
namespace shared {

static struct sigaction old_actions[2];

static void RequestStop(int signal_id) {
  SbSystemRequestStop(0);
}

static void InstallStopSignalHandlers() {
  struct sigaction action = {0};
  action.sa_handler = RequestStop;
  action.sa_flags = 0;
  ::sigemptyset(&action.sa_mask);
  ::sigaction(SIGINT, &action, &old_actions[0]);
  ::sigaction(SIGTERM, &action, &old_actions[1]);
}

static void UninstallStopSignalHandlers() {
  ::sigaction(SIGINT, &old_actions[0], NULL);
  ::sigaction(SIGTERM, &old_actions[1], NULL);
}

}  // namespace shared
}  // namespace rdk
}  // namespace starboard
}  // namespace third_party


extern "C" SB_EXPORT_PLATFORM int main(int argc, char** argv) {
  tzset();

  GError* error = NULL;
  gst_init_check(NULL, NULL, &error);
  g_free(error);

  starboard::shared::signal::InstallSuspendSignalHandlers();
  third_party::starboard::rdk::shared::InstallStopSignalHandlers();

#if SB_IS(EVERGREEN_COMPATIBLE)
  third_party::crashpad::wrapper::InstallCrashpadHandler(true);
#endif

  int result = 0;
  {
    third_party::starboard::rdk::shared::Application application;
    result = application.Run(argc, argv);
  }

  third_party::starboard::rdk::shared::UninstallStopSignalHandlers();
  starboard::shared::signal::UninstallSuspendSignalHandlers();

  gst_deinit();
  return result;
}
