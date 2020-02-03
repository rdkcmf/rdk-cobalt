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

#include <fcntl.h>

#include <poll.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include "starboard/common/log.h"
#include "starboard/event.h"
#include "starboard/shared/starboard/audio_sink/audio_sink_internal.h"

#include "third_party/starboard/wpe/shared/window/window_internal.h"

namespace third_party {
namespace starboard {
namespace wpe {
namespace shared {

Application::Application() {}

Application::~Application() {}

void Application::Initialize() {
  // Open wakeup event
  wakeup_fd_ = eventfd(0, 0);
  if (wakeup_fd_ == -1)
    SB_DLOG(ERROR) << "wakeup_fd_ creation failed";

  SbAudioSinkPrivate::Initialize();
}

void Application::Teardown() {
  SbAudioSinkPrivate::TearDown();

  // Close wakeup event
  close(wakeup_fd_);
}

bool Application::MayHaveSystemEvents() {
  return true;
}

// This is a bit of the shortcut. We could implement PollNextSystemEvent(),
// WaitForSystemEventWithTimeout() and WakeSystemEventWait() properly
// but instead PollNextSystemEvent() injects a new event which will be processed
// just after PollNextSystemEvent() if it returns nullptr
::starboard::shared::starboard::Application::Event*
Application::PollNextSystemEvent() {
  auto* display = window::GetDisplay();
  int fd = display->FileDescriptor();
  int flags = fcntl(fd, F_GETFL, 0);
  fcntl(fd, F_SETFL, flags | O_NONBLOCK);
  display->Process(1);
  return NULL;
}

::starboard::shared::starboard::Application::Event*
Application::WaitForSystemEventWithTimeout(SbTime time) {
  struct pollfd fds[2];
  struct timespec timeout_ts;
  int ret;

  timeout_ts.tv_sec = time / kSbTimeSecond;
  timeout_ts.tv_nsec =
      (time % kSbTimeSecond) * kSbTimeNanosecondsPerMicrosecond;

  // wait wayland event
  auto* display = window::GetDisplay();
  fds[0].fd = display->FileDescriptor();
  fds[0].events = POLLIN;
  fds[0].revents = 0;

  // wait wakeup event by event injection
  fds[1].fd = wakeup_fd_;
  fds[1].events = POLLIN;
  fds[1].revents = 0;

  ret = ppoll(fds, 2, &timeout_ts, NULL);

  if (timeout_ts.tv_sec > 0)  // long-wait log
    SB_DLOG(INFO) << "WaitForSystemEventWithTimeout : wakeup " << ret << " 0("
                  << fds[0].revents << ") 1(" << fds[1].revents << ")";

  if (ret > 0 && fds[1].revents & POLLIN) {  // clear wakeup event
    uint64_t u;
    read(wakeup_fd_, &u, sizeof(uint64_t));
  }

  return NULL;
}

void Application::WakeSystemEventWait() {
  uint64_t u = 1;
  write(wakeup_fd_, &u, sizeof(uint64_t));
}

SbWindow Application::CreateWindow(const SbWindowOptions* options) {
  SbWindow window = new SbWindowPrivate(options);
  return window;
}

bool Application::DestroyWindow(SbWindow window) {
  delete window;
  return true;
}

void Application::InjectInputEvent(SbInputData* data) {
  Inject(new Event(kSbEventTypeInput, data,
                   &Application::DeleteDestructor<SbInputData>));
}

void Application::Inject(Event* e) {
  if (e->event->type == kSbEventTypeSuspend) {
    e->destructor = nullptr;
  }

  QueueApplication::Inject(e);
}

}  // namespace shared
}  // namespace wpe
}  // namespace starboard
}  // namespace third_party
