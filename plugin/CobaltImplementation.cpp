/**
* Copyright 2020 Comcast Cable Communications Management, LLC
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* SPDX-License-Identifier: Apache-2.0
*/
#include "Module.h"
#include <interfaces/IMemory.h>
#include <interfaces/IBrowser.h>
#include <interfaces/IDictionary.h>
#include <interfaces/IFocus.h>

extern "C" {

int  StarboardMain(int argc, char **argv);
void SbRdkHandleDeepLink(const char* link);
void SbRdkSuspend();
void SbRdkResume();
void SbRdkPause();
void SbRdkUnpause();
void SbRdkQuit();
void SbRdkSetSetting(const char* key, const char* json);
int  SbRdkGetSetting(const char* key, char** out_json);

}  // extern "C"

namespace WPEFramework {

namespace Plugin {

static constexpr char kDefaultContentDir[] =
  "/usr/share/content/data:"
  "/media/apps/libcobalt/usr/share/content/data:"
  "/tmp/libcobalt/usr/share/content/data";

static void SetThunderAccessPointIfNeeded() {
  const std::string envName = _T("THUNDER_ACCESS");
  std::string envVal;
  if (Core::SystemInfo::GetEnvironment(envName, envVal))
    return;

  Core::File file("/etc/WPEFramework/config.json", false);
  if (!file.Open(true))
    return;

  Core::JSON::DecUInt16 port;
  Core::JSON::String binding;
  Core::JSON::Container config;
  config.Add("port", &port);
  config.Add("binding", &binding);
  if (config.IElement::FromFile(file)) {
    envVal = binding.Value() + ":" + std::to_string(port.Value());
    Core::SystemInfo::SetEnvironment("THUNDER_ACCESS", envVal);
  }
  file.Close();
}

enum StateChangeCommand : uint16_t {
  SUSPEND = PluginHost::IStateControl::SUSPEND,
  RESUME = PluginHost::IStateControl::RESUME,
  BACKGROUND
};

class CobaltImplementation:
    public Exchange::IBrowser,
    public PluginHost::IStateControl,
    public Exchange::IDictionary,
    public Exchange::IFocus {
private:
  class Config: public Core::JSON::Container {
  private:
    Config(const Config&);
    Config& operator=(const Config&);
  public:
    Config()
      : Core::JSON::Container()
      , Url()
      , ClientIdentifier()
      , Language()
      , ContentDir()
      , PreloadEnabled()
      , AutoSuspendDelay() {
      Add(_T("url"), &Url);
      Add(_T("clientidentifier"), &ClientIdentifier);
      Add(_T("language"), &Language);
      Add(_T("contentdir"), &ContentDir);
      Add(_T("gstdebug"), &GstDebug);
      Add(_T("preload"), &PreloadEnabled);
      Add(_T("autosuspenddelay"), &AutoSuspendDelay);
      Add(_T("systemproperties"), &SystemProperties);
    }
    ~Config() {
    }

  public:
    Core::JSON::String Url;
    Core::JSON::String ClientIdentifier;
    Core::JSON::String Language;
    Core::JSON::String ContentDir;
    Core::JSON::String GstDebug;
    Core::JSON::Boolean PreloadEnabled;
    Core::JSON::DecUInt16 AutoSuspendDelay;
    Core::JSON::VariantContainer SystemProperties;
  };

  class NotificationSink: public Core::Thread {
  private:
    NotificationSink() = delete;
    NotificationSink(const NotificationSink&) = delete;
    NotificationSink& operator=(const NotificationSink&) = delete;

  public:
    NotificationSink(CobaltImplementation &parent) :
      _parent(parent), _command(
        StateChangeCommand::SUSPEND) {
    }
    virtual ~NotificationSink() {
      Stop();
      Wait(Thread::STOPPED | Thread::BLOCKED, Core::infinite);
    }

  public:
    void RequestForStateChange(
      const StateChangeCommand command) {
      _lock.Lock();
      _command = command;
      _lock.Unlock();
      Run();
    }

  private:
    virtual uint32_t Worker() {
      bool success = false;

      _lock.Lock();
      const StateChangeCommand command = _command;
      _lock.Unlock();

      if ((IsRunning() == true) && (success == false)) {
        success = _parent.RequestForStateChange(command);
      }
      Block();
      _parent.StateChangeCompleted(success, command);

      if (success == true) {
        _lock.Lock();
        bool completed = (command == _command);
        _lock.Unlock();
        // Spin one more time
        if (!completed)
          Run();
      }

      return (Core::infinite);
    }

  private:
    CobaltImplementation &_parent;
    StateChangeCommand _command;
    mutable Core::CriticalSection _lock;
  };

  class DelayedSuspend
  {
  private:
    DelayedSuspend() = delete;
    DelayedSuspend(const DelayedSuspend&) = delete;
    DelayedSuspend& operator=(const DelayedSuspend&) = delete;

  private:
    bool _scheduled;
    CobaltImplementation &_parent;

    friend Core::ThreadPool::JobType<DelayedSuspend&>;
    Core::WorkerPool::JobType<DelayedSuspend&> _worker;

    void Dispatch()
    {
      _parent.RequestDelayedSuspend();
      _scheduled = false;
    }

  public:
    DelayedSuspend(CobaltImplementation &parent)
      : _scheduled(false)
      , _parent(parent)
      , _worker(*this)
    {
    }

    bool IsScheduled() const
    {
      return _scheduled;
    }

    void Schedule(const Core::Time& time)
    {
      _scheduled = _worker.Schedule(time);
    }

    void Cancel()
    {
      _worker.Revoke();
      _scheduled = false;
    }
  };

  class CobaltWindow : public Core::Thread {
  private:
    CobaltWindow(const CobaltWindow&) = delete;
    CobaltWindow& operator=(const CobaltWindow&) = delete;

  public:
    CobaltWindow(CobaltImplementation& parent)
      : Core::Thread(0, _T("Cobalt"))
      , _url("https://www.youtube.com/tv")
      , _parent(parent)
    {
    }
    virtual ~CobaltWindow()
    {
      Block();
      SbRdkQuit();
      Wait(Thread::BLOCKED | Thread::STOPPED | Thread::STOPPING, Core::infinite);
      exit(_exitCode);
    }

    uint32_t Configure(PluginHost::IShell* service) {
      if (IsRunning() == true)
        return Core::ERROR_ILLEGAL_STATE;

      uint32_t result = Core::ERROR_NONE;

      string config_line = service->ConfigLine();
      SYSLOG(Logging::Notification, (_T("Cobalt config line: --- %s ---\n"), config_line.c_str()));

      Config config;
      config.FromString(config_line);

      Core::Directory(service->PersistentPath().c_str()).CreatePath();
      Core::SystemInfo::SetEnvironment(_T("HOME"), service->PersistentPath());
      Core::SystemInfo::SetEnvironment(_T("COBALT_TEMP"), service->VolatilePath());
      if (config.ClientIdentifier.IsSet() == true) {
        string value(service->Callsign() + ',' + config.ClientIdentifier.Value());
        Core::SystemInfo::SetEnvironment(_T("CLIENT_IDENTIFIER"), value);
        Core::SystemInfo::SetEnvironment(_T("WAYLAND_DISPLAY"), config.ClientIdentifier.Value());
      } else {
        Core::SystemInfo::SetEnvironment(_T("CLIENT_IDENTIFIER"), service->Callsign());
      }

      SetThunderAccessPointIfNeeded();

      if (config.Url.IsSet() == true) {
        _url = config.Url.Value();
      }

      if (config.Language.IsSet() == true) {
        string lang = config.Language.Value();
        Core::SystemInfo::SetEnvironment(_T("LANG"), lang);
      }

      if (config.ContentDir.IsSet() == true) {
        string contentdir = config.ContentDir.Value();
        Core::SystemInfo::SetEnvironment(_T("COBALT_CONTENT_DIR"), contentdir);
      } else {
        Core::SystemInfo::SetEnvironment(_T("COBALT_CONTENT_DIR"), kDefaultContentDir);
      }

      {
        string envVal, gstDebug = "gstplayer:4,2";
        if (Core::SystemInfo::GetEnvironment(_T("GST_DEBUG"), envVal) && !envVal.empty())
          gstDebug = "," + envVal;
        if (config.GstDebug.IsSet() == true)
          gstDebug += "," + config.GstDebug.Value();
        Core::SystemInfo::SetEnvironment(_T("GST_DEBUG"), gstDebug);
      }

      if (config.PreloadEnabled.IsSet() == true) {
        _preloadEnabled = config.PreloadEnabled.Value();
      }

      if (config.AutoSuspendDelay.IsSet() == true) {
        _autoSuspendDelayInSeconds = config.AutoSuspendDelay.Value();
      }

      if (config.SystemProperties.IsSet() == true) {
        std::string properties;
        if (config.SystemProperties.ToString(properties))
          SbRdkSetSetting("systemproperties", properties.c_str());
      }

      SYSLOG(Logging::Notification, (_T("Preload is set to: %s\n"), _preloadEnabled ? "true" : "false"));

      Run();
      return result;
    }

    bool Suspend(const bool suspend)
    {
      if (suspend == true) {
        SbRdkSuspend();
      }
      else {
        SbRdkResume();
      }
      return (true);
    }

    bool Pause(const bool paused)
    {
      if (paused == true) {
        SbRdkPause();
      }
      else {
        SbRdkUnpause();
      }
      return (true);
    }

    string Url() const { return _url; }

    bool IsPreloadEnabled() const { return _preloadEnabled; }

    uint16_t AutoSuspendDelayInSeconds() const { return _autoSuspendDelayInSeconds; }

  private:
    bool Initialize() override
    {
      sigset_t mask;
      sigemptyset(&mask);
      sigaddset(&mask, SIGQUIT);
      sigaddset(&mask, SIGUSR1);
      sigaddset(&mask, SIGCONT);
      pthread_sigmask(SIG_UNBLOCK, &mask, nullptr);
      return (true);
    }
    uint32_t Worker() override
    {
      const std::string cmdURL = "--url=" + _url;
      std::vector<const char*> argv;

      argv.push_back("Cobalt");
      if (!_url.empty())
        argv.push_back(cmdURL.c_str());
      if (IsPreloadEnabled())
        argv.push_back("--preload");

      {
        std::string args_str;
        for (const char* arg: argv) {
          if (!args_str.empty())
            args_str += " ";
          args_str += arg;
        }
        SYSLOG(Logging::Notification, (_T("StarboardMain args: %s\n"), args_str.c_str()));
      }

      if (IsRunning() == true)
          _exitCode = StarboardMain(argv.size(), const_cast<char**>(argv.data()));
      if (IsRunning() == true) // app initiated exit
          _parent.StateChange(PluginHost::IStateControl::EXITED);
      Block();
      return (Core::infinite);
    }

    int _exitCode { 0 };
    string _url;
    CobaltImplementation &_parent;
    bool _preloadEnabled { false };
    uint16_t _autoSuspendDelayInSeconds { 30 };
  };

private:
  CobaltImplementation(const CobaltImplementation&) = delete;
  CobaltImplementation& operator=(const CobaltImplementation&) = delete;

public:
  CobaltImplementation() :
    _window(*this),
    _adminLock(),
    _state(PluginHost::IStateControl::UNINITIALIZED),
    _statePending(PluginHost::IStateControl::UNINITIALIZED),
    _cobaltClients(),
    _stateControlClients(),
    _sink(*this),
    _delayedSuspend(*this) {
  }

  virtual ~CobaltImplementation() {
  }

  virtual uint32_t Configure(PluginHost::IShell *service) {
    uint32_t result = _window.Configure(service);
    if (_window.IsPreloadEnabled()) {
      _state = PluginHost::IStateControl::SUSPENDED;
      _statePending = PluginHost::IStateControl::SUSPENDED;
      _delayedSuspend.Schedule(Core::Time::Now().Add(_window.AutoSuspendDelayInSeconds() * 1000));
    } else {
      _state = PluginHost::IStateControl::RESUMED;
    }
    return (result);
  }

  virtual void SetURL(const string &URL) override {
    SYSLOG(Logging::Notification, (_T("deeplink=%s\n"), URL.c_str()));
    SbRdkHandleDeepLink(URL.c_str());
  }

  virtual string GetURL() const override {
    return _window.Url();
  }

  virtual uint32_t GetFPS() const override {
    return 0;
  }

  virtual void Hide(const bool hidden) {
  }

  virtual void Register(Exchange::IBrowser::INotification *sink) {
    _adminLock.Lock();

    // Make sure a sink is not registered multiple times.
    ASSERT(
      std::find(_cobaltClients.begin(), _cobaltClients.end(), sink)
      == _cobaltClients.end());

    _cobaltClients.push_back(sink);
    sink->AddRef();

    _adminLock.Unlock();
  }

  virtual void Unregister(Exchange::IBrowser::INotification *sink) {
    _adminLock.Lock();

    std::list<Exchange::IBrowser::INotification*>::iterator index(
      std::find(_cobaltClients.begin(), _cobaltClients.end(), sink));

    // Make sure you do not unregister something you did not register !!!
    ASSERT(index != _cobaltClients.end());

    if (index != _cobaltClients.end()) {
      (*index)->Release();
      _cobaltClients.erase(index);
    }

    _adminLock.Unlock();
  }

  virtual void Register(PluginHost::IStateControl::INotification *sink) {
    _adminLock.Lock();

    // Make sure a sink is not registered multiple times.
    ASSERT(
      std::find(_stateControlClients.begin(),
                _stateControlClients.end(), sink)
      == _stateControlClients.end());

    _stateControlClients.push_back(sink);
    sink->AddRef();

    _adminLock.Unlock();
  }

  virtual void Unregister(PluginHost::IStateControl::INotification *sink) {
    _adminLock.Lock();

    std::list<PluginHost::IStateControl::INotification*>::iterator index(
      std::find(_stateControlClients.begin(),
                _stateControlClients.end(), sink));

    // Make sure you do not unregister something you did not register !!!
    ASSERT(index != _stateControlClients.end());

    if (index != _stateControlClients.end()) {
      (*index)->Release();
      _stateControlClients.erase(index);
    }

    _adminLock.Unlock();
  }

  virtual PluginHost::IStateControl::state State() const {
    return (_state);
  }

  virtual uint32_t Request(const PluginHost::IStateControl::command command) {
    uint32_t result = Core::ERROR_ILLEGAL_STATE;

    _adminLock.Lock();

    if (_state == PluginHost::IStateControl::UNINITIALIZED || _state == PluginHost::IStateControl::EXITED) {
      ASSERT(false);
    } else {
      switch (command) {
        case PluginHost::IStateControl::SUSPEND:
          if (_state != PluginHost::IStateControl::SUSPENDED && _statePending != PluginHost::IStateControl::SUSPENDED) {
            _statePending = PluginHost::IStateControl::SUSPENDED;
            _sink.RequestForStateChange(
              StateChangeCommand::SUSPEND);
          }
          result = Core::ERROR_NONE;
          break;
        case PluginHost::IStateControl::RESUME:
          if (_state != PluginHost::IStateControl::RESUMED && _statePending != PluginHost::IStateControl::RESUMED) {
            _statePending = PluginHost::IStateControl::RESUMED;
            _sink.RequestForStateChange(
              StateChangeCommand::RESUME);
          }
          if (_delayedSuspend.IsScheduled()) {
            _delayedSuspend.Cancel();
          }
          result = Core::ERROR_NONE;
          break;
        default:
          break;
      }
    }

    _adminLock.Unlock();

    return result;
  }

  void StateChangeCompleted(bool success,
                            const StateChangeCommand request) {
    if (success) {
      switch (request) {
        case StateChangeCommand::RESUME:

          _adminLock.Lock();

          if (_state != PluginHost::IStateControl::RESUMED) {
            StateChange(PluginHost::IStateControl::RESUMED);
          }

          _adminLock.Unlock();
          break;
        case StateChangeCommand::SUSPEND:

          _adminLock.Lock();

          if (_state != PluginHost::IStateControl::SUSPENDED) {
            StateChange(PluginHost::IStateControl::SUSPENDED);
          }

          _adminLock.Unlock();
          break;
        case StateChangeCommand::BACKGROUND:
          break;
        default:
          ASSERT(false);
          break;
      }
    } else {
      StateChange(PluginHost::IStateControl::EXITED);
    }
  }

  void RequestDelayedSuspend() {
    _adminLock.Lock();
    if (_state == PluginHost::IStateControl::SUSPENDED && _statePending == PluginHost::IStateControl::SUSPENDED) {
      _sink.RequestForStateChange(StateChangeCommand::SUSPEND);
    }
    _adminLock.Unlock();
  }

  // IFocus
  uint32_t Focused(const bool focused) override {
    _adminLock.Lock();
    if (_state == PluginHost::IStateControl::RESUMED || _statePending == PluginHost::IStateControl::RESUMED) {
      if (focused)
        _sink.RequestForStateChange(StateChangeCommand::RESUMED);
      else
        _sink.RequestForStateChange(StateChangeCommand::BACKGROUND);
    }
    _adminLock.Unlock();
    return Core::ERROR_NONE;
  };

  // IDictionary iface
  void Register(const string& nameSpace, struct IDictionary::INotification* sink) override  {  }
  void Unregister(const string& nameSpace, struct IDictionary::INotification* sink) override  {  }
  IIterator* Get(const string& nameSpace) const override { return nullptr; }
  bool Get(const string& nameSpace, const string& key, string& value /* @out */) const override {
    if (nameSpace == "settings") {
      if (key == "accessibility") {
        char* json = nullptr;
        if (SbRdkGetSetting(key.c_str(), &json) == 0) {
          value.assign(json);
          free(json);
          return true;
        }
      }
    }
    return false;
  }
  bool Set(const string& nameSpace, const string& key, const string& value) override {
    if (nameSpace == "settings") {
      if (key == "accessibility") {
        SbRdkSetSetting(key.c_str(), value.c_str());
        return true;
      }
    }
    return false;
  }

  BEGIN_INTERFACE_MAP (CobaltImplementation)
  INTERFACE_ENTRY (Exchange::IBrowser)
  INTERFACE_ENTRY (PluginHost::IStateControl)
  INTERFACE_ENTRY (Exchange::IDictionary)
  INTERFACE_ENTRY (Exchange::IFocus)
  END_INTERFACE_MAP

private:
  static const char* ToString(const StateChangeCommand command) {
    switch(command) {
      case StateChangeCommand::SUSPEND:
        return "suspend";
      case StateChangeCommand::RESUME:
        return "resume";
      case StateChangeCommand::BACKGROUND:
        return "background";
      default:
        break;
    }
    return "unknown";
  }

  inline bool RequestForStateChange(
    const StateChangeCommand command) {
    bool result = false;

    SYSLOG(Logging::Notification, (_T("Cobalt request state change -> %s\n"), ToString(command)));

    switch (command) {
      case StateChangeCommand::SUSPEND: {
        if (_window.Suspend(true) == true) {
          result = true;
        }
        break;
      }
      case StateChangeCommand::RESUME: {
        // implies unpause
        if (_window.Suspend(false) == true) {
          result = true;
        }
        break;
      }
      case StateChangeCommand::BACKGROUND: {
        if (_window.Pause(true) == true) {
          result = true;
        }
        break;
      }
      default:
        ASSERT(false);
        break;
    }
    return result;
  }

  void StateChange(const PluginHost::IStateControl::state newState) {
    _adminLock.Lock();

    _state = newState;
    _statePending = PluginHost::IStateControl::UNINITIALIZED;

    std::list<PluginHost::IStateControl::INotification*>::iterator index(
      _stateControlClients.begin());

    while (index != _stateControlClients.end()) {
      (*index)->StateChange(newState);
      index++;
    }

    _adminLock.Unlock();
  }

private:
  CobaltWindow _window;
  mutable Core::CriticalSection _adminLock;
  PluginHost::IStateControl::state _state;
  PluginHost::IStateControl::state _statePending;
  std::list<Exchange::IBrowser::INotification*> _cobaltClients;
  std::list<PluginHost::IStateControl::INotification*> _stateControlClients;
  NotificationSink _sink;
  DelayedSuspend _delayedSuspend;
};

SERVICE_REGISTRATION(CobaltImplementation, 1, 0);

}  // namespace Plugin

namespace Cobalt {

class MemoryObserverImpl: public Exchange::IMemory {
private:
  MemoryObserverImpl();
  MemoryObserverImpl(const MemoryObserverImpl&);
  MemoryObserverImpl& operator=(const MemoryObserverImpl&);

public:
  MemoryObserverImpl(const RPC::IRemoteConnection* connection) :
    _main(connection == nullptr ? Core::ProcessInfo().Id() : connection->RemoteId()) {
  }
  ~MemoryObserverImpl() {
  }

public:
  virtual uint64_t Resident() const {
    return _main.Resident();
  }
  virtual uint64_t Allocated() const {
    return _main.Allocated();
  }
  virtual uint64_t Shared() const {
    return _main.Shared();
  }
  virtual uint8_t Processes() const {
    return (IsOperational() ? 1 : 0);
  }

  virtual const bool IsOperational() const {
    return _main.IsActive();
  }

  BEGIN_INTERFACE_MAP (MemoryObserverImpl)
  INTERFACE_ENTRY (Exchange::IMemory)
  END_INTERFACE_MAP

  private:
  Core::ProcessInfo _main;
};

Exchange::IMemory* MemoryObserver(const RPC::IRemoteConnection* connection) {
  ASSERT(connection != nullptr);
  Exchange::IMemory* result = Core::Service<MemoryObserverImpl>::Create<Exchange::IMemory>(connection);
  return (result);
}

}  // namespace Cobalt

}  // namespace WPEFramework
