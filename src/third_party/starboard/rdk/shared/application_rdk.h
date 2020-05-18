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

#ifndef THIRD_PARTY_STARBOARD_RDK_SHARED_APPLICATION_RDK_H_
#define THIRD_PARTY_STARBOARD_RDK_SHARED_APPLICATION_RDK_H_

#include "starboard/configuration.h"
#include "starboard/input.h"
#include "starboard/shared/internal_only.h"
#include "starboard/shared/starboard/application.h"
#include "starboard/shared/starboard/queue_application.h"
#include "starboard/types.h"

#include "third_party/starboard/rdk/shared/ess_input.h"
#include "third_party/starboard/rdk/shared/system_services.h"

#include <memory>
#include <essos-app.h>

namespace third_party {
namespace starboard {
namespace rdk {
namespace shared {

class Application : public ::starboard::shared::starboard::QueueApplication {
 public:
  Application();
  ~Application() override;

  static third_party::starboard::rdk::shared::Application* Get() {
    return static_cast<third_party::starboard::rdk::shared::Application*>(
        ::starboard::shared::starboard::Application::Get());
  }

  SbWindow CreateSbWindow(const SbWindowOptions* options);
  bool DestroySbWindow(SbWindow window);
  void InjectInputEvent(SbInputData* data);

  EssCtx *GetEssCtx() const { return ctx_; }
  NativeWindowType GetNativeWindow() const { return native_window_; }
  int GetWindowWidth() const { return window_width_; }
  int GetWindowHeight() const { return window_height_; }
  ResolutionInfo GetDisplayResolution() const;
  bool DisplayHasHDRSupport() const;

 protected:
  // --- Application overrides ---
  void Initialize() override;
  void Teardown() override;
  void Inject(Event* e) override;
  void OnSuspend() override;
  void OnResume() override;

  // --- QueueApplication overrides ---
  bool MayHaveSystemEvents() override;
  Event* PollNextSystemEvent() override;
  Event* WaitForSystemEventWithTimeout(SbTime time) override;
  void WakeSystemEventWait() override;

  // --- Essos ---
  void OnTerminated();
  void OnKeyPressed(unsigned int key);
  void OnKeyReleased(unsigned int key);
  void OnDisplaySize(int width, int height);

 private:
  void MaterializeNativeWindow();
  void DestroyNativeWindow();

  static EssTerminateListener terminateListener;
  static EssKeyListener keyListener;
  static EssSettingsListener settingsListener;

  EssCtx *ctx_ { nullptr };
  std::unique_ptr<EssInput> input_handler_ { nullptr };
  SbWindow window_ { kSbWindowInvalid };
  NativeWindowType native_window_ { 0 };
  int window_width_ { 0 };
  int window_height_ { 0 };
  bool resize_pending_ { false };

  std::unique_ptr<DisplayInfo> display_info_ { nullptr };
};

}  // namespace shared
}  // namespace rdk
}  // namespace starboard
}  // namespace third_party

#endif  // THIRD_PARTY_STARBOARD_RDK_SHARED_APPLICATION_RDK_H_
