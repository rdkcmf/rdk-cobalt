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

#include "third_party/starboard/wpe/shared/application_wpe.h"

#include "starboard/common/log.h"
#include "starboard/event.h"
#include "starboard/shared/starboard/audio_sink/audio_sink_internal.h"

#include "third_party/starboard/wpe/shared/window/window_internal.h"

namespace third_party {
namespace starboard {
namespace wpe {
namespace shared {

EssTerminateListener Application::terminateListener = {
  //terminated
  [](void* data) { reinterpret_cast<Application*>(data)->OnTerminated(); }
};

EssKeyListener Application::keyListener = {
  // keyPressed
  [](void* data, unsigned int key) { reinterpret_cast<Application*>(data)->OnKeyPressed(key); },
  // keyReleased
  [](void* data, unsigned int key) { reinterpret_cast<Application*>(data)->OnKeyReleased(key); },
  // keyRepeat
  nullptr
};

EssSettingsListener Application::settingsListener = {
  // displaySize
  [](void *data, int width, int height ) { reinterpret_cast<Application*>(data)->OnDisplaySize(width, height); },
  // displaySafeArea
  nullptr
};

Application::Application() : input_handler_(new EssInput) {
  bool error = false;
  ctx_ = EssContextCreate();

  if ( !EssContextInit(ctx_) ) {
    error = true;
  }
  else if ( !EssContextSetTerminateListener(ctx_, this, &terminateListener) ) {
    error = true;
  }
  else if ( !EssContextSetKeyListener(ctx_, this, &keyListener) ) {
    error = true;
  }
  else if ( !EssContextSetSettingsListener(ctx_, this, &settingsListener) ) {
    error = true;
  }

  if ( error ) {
    const char *detail = EssContextGetLastErrorDetail(ctx_);
    SB_LOG(ERROR) << "Essos error: '" <<  detail << '\'';
  }
}

Application::~Application() {
  EssContextDestroy(ctx_);
}

void Application::Initialize() {
  SbAudioSinkPrivate::Initialize();
}

void Application::Teardown() {
  SbAudioSinkPrivate::TearDown();
}

bool Application::MayHaveSystemEvents() {
  return true;
}

::starboard::shared::starboard::Application::Event*
Application::PollNextSystemEvent() {
  EssContextRunEventLoopOnce( ctx_ );
  return NULL;
}

::starboard::shared::starboard::Application::Event*
Application::WaitForSystemEventWithTimeout(SbTime time) {
  return NULL;
}

void Application::WakeSystemEventWait() {
}

SbWindow Application::CreateSbWindow(const SbWindowOptions* options) {
  SB_DCHECK(window_ == nullptr);
  if (window_ != nullptr)
    return kSbWindowInvalid;
  MaterializeNativeWindow();
  window_  = new SbWindowPrivate(options);
  return window_;
}

bool Application::DestroySbWindow(SbWindow window) {
  if (!SbWindowIsValid(window))
    return false;
  window_ = nullptr;
  delete window;
  DestroyNativeWindow();
  return true;
}

void Application::InjectInputEvent(SbInputData* data) {
  data->window = window_;
  Inject(new Event(kSbEventTypeInput, data,
                   &Application::DeleteDestructor<SbInputData>));
}

void Application::Inject(Event* e) {
  if (e->event->type == kSbEventTypeSuspend) {
    e->destructor = nullptr;
  }

  QueueApplication::Inject(e);
}

void Application::OnSuspend() {
  DestroyNativeWindow();
}

void Application::OnResume() {
  MaterializeNativeWindow();
}

void Application::OnTerminated() {
  Stop(0);
}

void Application::OnKeyPressed(unsigned int key) {
  input_handler_->OnKeyPressed(key);
}

void Application::OnKeyReleased(unsigned int key) {
  input_handler_->OnKeyReleased(key);
}

void Application::OnDisplaySize(int width, int height) {
  if (window_width_ == width && window_height_ == height) {
    resize_pending_ = false;
    return;
  }

  SB_DCHECK(native_window_ == 0);
  resize_pending_ = true;
}

void Application::MaterializeNativeWindow() {
  if (native_window_ != 0)
    return;

  bool error = false;

  if ( !EssContextGetDisplaySize(ctx_, &window_width_, &window_height_) ) {
    error = true;
  }

  if ( resize_pending_ ) {
    EssContextResizeWindow(ctx_, window_width_, window_height_);
    resize_pending_ = false;
  }

  if ( !EssContextCreateNativeWindow(ctx_, window_width_, window_height_, &native_window_) ) {
    error = true;
  }
  else if ( !EssContextStart(ctx_) ) {
    error = true;
  }

  if ( error ) {
    const char *detail = EssContextGetLastErrorDetail(ctx_);
    SB_LOG(ERROR) << "Essos error: '" <<  detail << '\'';
  }
}

void Application::DestroyNativeWindow() {
  if (native_window_ == 0)
    return;

  if ( !EssContextDestroyNativeWindow(ctx_, native_window_) ) {
    const char *detail = EssContextGetLastErrorDetail(ctx_);
    SB_LOG(ERROR) << "Essos error: '" <<  detail << '\'';
  }

  native_window_ = 0;

  EssContextStop(ctx_);
}

}  // namespace shared
}  // namespace wpe
}  // namespace starboard
}  // namespace third_party
