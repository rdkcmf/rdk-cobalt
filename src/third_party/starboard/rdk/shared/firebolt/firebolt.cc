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
#ifndef MODULE_NAME
#define MODULE_NAME CobaltRDKServices
#endif

#include "third_party/starboard/rdk/shared/firebolt/firebolt.h"

#include <mutex>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <websocket/WebSocketLink.h>
#include <websocket/URL.h>

#include "third_party/starboard/rdk/shared/log_override.h"

using namespace WPEFramework;

namespace third_party {
namespace starboard {
namespace rdk {
namespace shared {
namespace firebolt {

static const auto kDefaultTimeout = std::chrono::milliseconds(50);

namespace {

template<typename T>
T valueOr(const Core::OptionalType<T>& opt, T defaultValue = {}) {
  return opt.IsSet() ? opt.Value() : defaultValue;
};

// Simplefied version of WPEFramework::JSONRPC::Link
class Link {
private:
  class CommunicationChannel {
  private:
    class FactoryImpl {
    private:
      friend Core::SingletonType<FactoryImpl>;
      FactoryImpl(const FactoryImpl&) = delete;
      FactoryImpl& operator=(const FactoryImpl&) = delete;
      FactoryImpl() = default;

      Core::ProxyPoolType<Core::JSONRPC::Message> _jsonRPCFactory { 2 };
    public:
      ~FactoryImpl() = default;

      static FactoryImpl& Instance()
      {
        static FactoryImpl& _singleton = Core::SingletonType<FactoryImpl>::Instance();
        return (_singleton);
      }

      Core::ProxyType<Core::JSONRPC::Message> Element(const string&)
      {
        return (_jsonRPCFactory.Element());
      }
    };

    class ChannelImpl : public Core::StreamJSONType<Web::WebSocketClientType<Core::SocketStream>, FactoryImpl&, Core::JSON::IElement> {
    private:
      ChannelImpl(const ChannelImpl&) = delete;
      ChannelImpl& operator=(const ChannelImpl&) = delete;

      typedef Core::StreamJSONType<Web::WebSocketClientType<Core::SocketStream>, FactoryImpl&, Core::JSON::IElement> BaseClass;

    public:
      ChannelImpl(CommunicationChannel* parent, const Core::NodeId& remoteNode, const string& path, const string& query)
        : BaseClass(5, FactoryImpl::Instance(), path, "", query, "", false, true, false, remoteNode.AnyInterface(), remoteNode, 4096, 4096)
        , _parent(*parent)
      {
      }
      virtual ~ChannelImpl() = default;
      virtual void Received(Core::ProxyType<Core::JSON::IElement>& jsonObject) override
      {
        Core::ProxyType<Core::JSONRPC::Message> inbound(jsonObject);
        ASSERT(inbound.IsValid() == true);
        if (inbound.IsValid() == true) {
          _parent.Inbound(inbound);
        }
      }
      virtual void Send(Core::ProxyType<Core::JSON::IElement>& jsonObject) override
      {
#if 0
        string message;
        ToMessage(jsonObject, message);
        SB_LOG(INFO) << "sending message: " << message;
#endif
      }
      virtual void StateChange() override
      {
        _parent.StateChange();
      }
      virtual bool IsIdle() const
      {
        return (true);
      }
    private:
      void ToMessage(const Core::ProxyType<Core::JSON::IElement>& jsonObject, string& message) const
      {
        Core::ProxyType<Core::JSONRPC::Message> inbound(jsonObject);
        ASSERT(inbound.IsValid());
        if (inbound.IsValid())
          inbound->ToString(message);
      }
    private:
      CommunicationChannel& _parent;
    };

  protected:
    CommunicationChannel(const Core::NodeId& remoteNode, const string& path, const string& query)
      : _channel(this, remoteNode, path, query)
      , _sequence(0)
    {
    }

  public:
    virtual ~CommunicationChannel()
    {
      Close();
    }
    static Core::ProxyType<CommunicationChannel> Instance(const Core::NodeId& remoteNode, const string& path, const string& query)
    {
      static Core::ProxyMapType<string, CommunicationChannel> channelMap;
      string searchLine = remoteNode.HostAddress() + '@' + path + '?' + query;
      return (channelMap.template Instance<CommunicationChannel>(searchLine, remoteNode, path, query));
    }
    static Core::ProxyType<Core::JSONRPC::Message> Message()
    {
      return (FactoryImpl::Instance().Element(string()));
    }
    void Register(Link& client)
    {
      typename std::unique_lock<std::mutex> lock(_adminLock);
      ASSERT(std::find(_observers.begin(), _observers.end(), &client) == _observers.end());
      _observers.push_back(&client);
    }
    void Unregister(Link& client)
    {
      typename std::unique_lock<std::mutex> lock(_adminLock);
      auto index = std::find(_observers.begin(), _observers.end(), &client);
      if (index != _observers.end())
        _observers.erase(index);
    }
    void Submit(const Core::ProxyType<Core::JSON::IElement>& message)
    {
      _channel.Submit(message);
    }
    bool IsSuspended() const
    {
      return (_channel.IsSuspended());
    }
    bool IsOpen() const
    {
      return _channel.IsOpen();
    }
    uint32_t Initialize()
    {
      return (Open(1000));
    }
    uint32_t Sequence() const
    {
      return (++_sequence);
    }
  protected:
    void StateChange()
    {
    }
    bool Open(const uint32_t waitTime)
    {
      bool result = true;
      if (_channel.IsClosed() == true) {
        result = (_channel.Open(waitTime) == Core::ERROR_NONE);
      }
      return (result);
    }
    void Close()
    {
      _channel.Close(Core::infinite);
    }
  private:
    uint32_t Inbound(const Core::ProxyType<Core::JSONRPC::Message>& inbound)
    {
      uint32_t result = Core::ERROR_UNAVAILABLE;

      typename std::unique_lock<std::mutex> lock(_adminLock);
      auto index = _observers.begin();
      while ((result != Core::ERROR_NONE) && (index != _observers.end())) {
        result = (*index)->Inbound(inbound);
        index++;
      }

      return (result);
    }

  private:
    std::mutex _adminLock;
    ChannelImpl _channel;
    std::list< Link *> _observers;
    mutable std::atomic<uint32_t> _sequence;
  };

  static Core::ProxyType<CommunicationChannel> CommunicationChannelInstance(const Core::URL& url)
  {
    const auto& host  = valueOr(url.Host(), string("127.0.0.1"));
    const auto& path  = string("/") + valueOr(url.Path());
    const auto& port  = valueOr(url.Port(), Core::URL::Port(url.Type()));
    const auto& query = valueOr(url.Query());
    Core::NodeId nodeId = Core::NodeId(host.c_str(), port);
    return CommunicationChannel::Instance(nodeId, path, query);
  }

  Link(const Link&) = delete;
  Link& operator=(Link&) = delete;

public:
  using ErrorInfo = Core::JSONRPC::Message::Info;
  using CallbackFunction = std::function<void(const Core::JSONRPC::Message&)>;
  using CallbackMap = std::map<uint32_t, CallbackFunction>;

  explicit Link(const Core::URL& url)
    : _channel ( CommunicationChannelInstance(url) )
  {
    _channel->Register(*this);
  }

  virtual ~Link()
  {
    _channel->Unregister(*this);
  }

  uint32_t SendAsync(const char method[], const Core::JSON::IElement& parameters, CallbackFunction&& cb, uint32_t& callbackId)
  {
    uint32_t result = Core::ERROR_UNAVAILABLE;
    if ( method ) {
      if ( _channel->IsSuspended() == true || _channel->IsOpen() == false ) {
        result = Core::ERROR_ASYNC_FAILED;
      } else {
        uint32_t id;
        Core::ProxyType<Core::JSONRPC::Message> message(CommunicationChannel::Message());

        id = _channel->Sequence();
        message->Id = id;
        message->Designator = method;
        if ( parameters.IsSet() ) {
          string params;
          parameters.ToString(params);
          message->Parameters = params;
        }

        callbackId = id;
        InsertCallback(id, std::move(cb));

        _channel->Submit(Core::ProxyType<Core::JSON::IElement>(message));
        message.Release();

        result = Core::ERROR_NONE;
      }
    }
    return (result);
  }

  uint32_t Send(std::chrono::microseconds waitTime, const char method[], const Core::JSON::IElement& parameters, Core::JSON::IElement& result, ErrorInfo& errorInfo)
  {
    uint32_t rc;
    uint32_t callbackId = -1u;
    std::condition_variable signal;
    std::shared_ptr<bool> done = std::make_shared<bool>(false);

    CallbackFunction cb = [this, weak_done = std::weak_ptr<bool>(done), &result, &errorInfo, &signal](const Core::JSONRPC::Message& m) {
      auto done = weak_done.lock();
      if ( done == nullptr )
        return;
      std::unique_lock<std::mutex> lock(_adminLock);
      if ( *done == false )  // timeout check
      {
        if ( m.Result.IsSet() )
          result.FromString(m.Result.Value());
        else
          errorInfo = m.Error;
        *done = true;
        lock.unlock();
        signal.notify_one();
      }
    };

    result.Clear();
    errorInfo.Clear();

    rc = SendAsync(method, parameters, std::move(cb), callbackId);

    std::unique_lock<std::mutex> lock(_adminLock);
    if (rc == Core::ERROR_NONE) {
      if ( ! signal.wait_for(lock, waitTime, [&]{ return *done; }) ) {
        *done = true;  // set to indicate timeout
        rc = Core::ERROR_ASYNC_FAILED;
      }
    }

    RemoveCallback(lock, callbackId);

    return rc;
  }

  uint32_t RemoveCallbackById(int32_t id)
  {
    std::unique_lock<std::mutex> lock(_adminLock);
    return RemoveCallback(lock, id);
  }

private:
  friend CommunicationChannel;

  uint32_t RemoveCallback(std::unique_lock<std::mutex>&, int32_t id)
  {
    auto it = _callbackMap.find(id);
    if (it != _callbackMap.end())
      _callbackMap.erase(it);
    return Core::ERROR_NONE;
  }

  void InsertCallback(uint32_t id, CallbackFunction&& cb)
  {
    std::unique_lock<std::mutex> lock(_adminLock);

    _callbackMap.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(id),
      std::forward_as_tuple(std::move(cb)));
  }

  uint32_t Inbound(const Core::ProxyType<Core::JSONRPC::Message>& inbound)
  {
    uint32_t result = Core::ERROR_INVALID_SIGNATURE;
    if ( inbound->Id.IsSet() ) {
      if ( inbound->Result.IsSet() || inbound->Error.IsSet() ) {
        uint32_t id = inbound->Id.Value();
        CallbackFunction cb;

        _adminLock.lock();
        auto it = _callbackMap.find(id);
        if (it != _callbackMap.end()) {
          cb = std::move(it->second);
          _callbackMap.erase(it);
        }
        _adminLock.unlock();

        if (cb)
          cb(*inbound);

        result = Core::ERROR_NONE;
      }
    }
    return (result);
  }

private:
  Core::ProxyType< CommunicationChannel > _channel;
  std::mutex _adminLock;
  CallbackMap _callbackMap;
};

static Core::URL FireboltEndpoint()
{
  static std::once_flag flag;
  static Core::URL url;
  std::call_once(flag, [&](){
    string envVar;
    if ((Core::SystemInfo::GetEnvironment(_T("FIREBOLT_ENDPOINT"), envVar) == true) && (envVar.empty() == false))
      url = Core::URL(envVar.c_str());
  });
  return url;
}

struct Entitlements : Core::JSON::Container
{
  struct Entitlement : Core::JSON::Container
  {
    Entitlement()
      : Core::JSON::Container()
    {
      Init();
    }
    Entitlement(const Entitlement& other)
      : Core::JSON::Container()
      , entitlementId(other.entitlementId)
    {
      Init();
    }
    Entitlement& operator=(const Entitlement& rhs)
    {
      entitlementId = rhs.entitlementId;
      return (*this);
    }
    Core::JSON::String entitlementId;
  private:
    void Init() {
      Add(_T("entitlementId"), &entitlementId);
    }
  };
  Entitlements()
    : Core::JSON::Container()
  {
    Add(_T("entitlements"), &entitlements);
  }

  Core::JSON::ArrayType<Entitlement> entitlements;
};

}  // namespace

bool IsAvailable()
{
  auto url = FireboltEndpoint();
  return url.IsValid() && (url.Type() == Core::URL::SCHEME_WS || url.Type() == Core::URL::SCHEME_WSS);
}

bool Discovery::entitlements(const std::vector<Entitlement>& entitlements)
{
  if (IsAvailable())
  {
    Link link( FireboltEndpoint() );

    uint32_t rc;
    Entitlements params;
    Link::ErrorInfo error;
    Core::JSON::String result;

    for ( const auto& i : entitlements )
      params.entitlements.Add().entitlementId = i.entitlementId;

    rc = link.Send(kDefaultTimeout, "discovery.entitlements", params, result, error);

    if (rc != Core::ERROR_NONE || result.IsSet() == false)
    {
      SB_LOG(ERROR) << "Failed to send 'discovery.entitlements', rc=" << rc
                    << " ( " << Core::ErrorToString(rc) << " )"
                    << ", error code = " << error.Code.Value()
                    <<  " (" << error.Text.Value() << ")";
    }
    else
      return true;
  }
  return false;
}

}  // namespace firebolt
}  // namespace shared
}  // namespace rdk
}  // namespace starboard
}  // namespace third_party
