/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Liberty Global Service B.V.
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
 */
#ifndef __COBALT_RPC_SERVER_H__
#define __COBALT_RPC_SERVER_H__

#include <functional>
#include <memory>
#include <string>

#include <json/json.h>

#include "logging.h"
#include "libcobalt.h"

#include <rpcserver/IAbstractRpcServer.h>
#include <rpcserver/WsRpcServerBuilder.h>

#define RPC_METHOD_BASE "org.rdk.cobalt.1."

using namespace rpcserver;

typedef enum {
  STARTED = 0,
  STOPPED = 1,
  SUSPENDED = 2,
  UNKNOWN = 99,
} cobalt_state_t;

constexpr const char* stateStr(cobalt_state_t state) {
  switch(state) {
    case STARTED: return "started";
    case STOPPED: return "stopped";
    case SUSPENDED: return "suspended";
    case UNKNOWN: return "unknown";
  }
  return "unknown";
}

/***************************************************************
 *
 * *************************************************************/
class cobalt_rpc_wsServer
{
private:
  std::string url_;
  cobalt_state_t state_;

public:
  /***************************************************************
   *
   * *************************************************************/
  explicit cobalt_rpc_wsServer(uint16_t port) : url_(""), state_(UNKNOWN)
  {
    std::string registerMethodName(RPC_METHOD_REGISTER);
    std::string unregisterMethodName(RPC_METHOD_UNREGISTER);
    std::string getListenersMethodName(RPC_METHOD_GET_LISTENERS);

    WsRpcServerBuilder serverBuilder(port, true);
    wsRpcServer =
      std::shared_ptr<IAbstractRpcServer>(
        serverBuilder
          .enableServerEvents(registerMethodName, unregisterMethodName, getListenersMethodName)
          .numThreads(1)
          .build()
      );

    wsRpcServer->bindMethod(RPC_METHOD_RESUME,
                            [this](const Json::Value& request, Json::Value& response) {
                              DBG(RPC_METHOD_RESUME << " " << request);
                              SbRdkResume();
                              emitCobaltStateEvent(STARTED, 0);
                              response = Json::Value();
                            });

    wsRpcServer->bindMethod(RPC_METHOD_SUSPEND,
                            [this](const Json::Value& request, Json::Value& response) {
                              DBG(RPC_METHOD_SUSPEND << " " << request);
                              SbRdkSuspend();
                              emitCobaltStateEvent(SUSPENDED, 0);
                              response = Json::Value();
                            });

    wsRpcServer->bindMethod(RPC_METHOD_STOP,
                            [](const Json::Value& request, Json::Value& response) {
                              DBG(RPC_METHOD_STOP << " " << request);
                              SbRdkQuit();
                              response = Json::Value();
                            });

    wsRpcServer->bindMethod(RPC_METHOD_GETSTATE,
                            [this](const Json::Value& request, Json::Value& response) {
                              DBG(RPC_METHOD_GETSTATE << " " << request);
                              response = Json::Value();
                              response["pid"] = ::getpid();
                              response["state"] = state_;
                            });

    wsRpcServer->bindMethod(RPC_METHOD_GETURL,
                            [this](const Json::Value& request, Json::Value& response) {
                              DBG(RPC_METHOD_GETURL << " " << request);
                              response = Json::Value();
                              response["url"] = url_;
                            });

    wsRpcServer->bindMethod(RPC_METHOD_DEEPLINK,
                            [](const Json::Value& request, Json::Value& response) {
                              DBG(RPC_METHOD_DEEPLINK << " " << request);
                              auto link = request["data"].asString();
                              if (!link.empty()) {
                                SbRdkHandleDeepLink(link.c_str());
                              }
                              response = Json::Value();
                            });

  }

  void setUrl(const std::string& url) {
    url_ = url;
  }

  void emitCobaltStateEvent(cobalt_state_t state, int exitcode)  {
    Json::Value eventJson;
    state_ = state;
    eventJson["pid"] = ::getpid();
    eventJson["state"] = state_;
    eventJson["code"] = exitcode;
    wsRpcServer->onEvent("StateEvent", eventJson);
  }

  /***************************************************************
   *
   * *************************************************************/
  cobalt_rpc_wsServer& operator=(const cobalt_rpc_wsServer&) = delete;

  /***************************************************************
   *
   * *************************************************************/
  cobalt_rpc_wsServer(const cobalt_rpc_wsServer&) = delete;

  /***************************************************************
   *
   * *************************************************************/
  virtual ~cobalt_rpc_wsServer()
  {
    stop();
  }

  /***************************************************************
   *
   * *************************************************************/
  void start() { wsRpcServer->StartListening(); }

  /***************************************************************
   *
   * *************************************************************/
  void stop() { wsRpcServer->StopListening(); }

protected:

  static constexpr auto RPC_METHOD_REGISTER = RPC_METHOD_BASE "register";
  static constexpr auto RPC_METHOD_UNREGISTER = RPC_METHOD_BASE "unregister";
  static constexpr auto RPC_METHOD_GET_LISTENERS = RPC_METHOD_BASE "getListeners";
  static constexpr auto RPC_METHOD_RESUME = RPC_METHOD_BASE "resume";
  static constexpr auto RPC_METHOD_SUSPEND = RPC_METHOD_BASE "suspend";
  static constexpr auto RPC_METHOD_STOP = RPC_METHOD_BASE "stop";
  static constexpr auto RPC_METHOD_GETSTATE = RPC_METHOD_BASE "getState";
  static constexpr auto RPC_METHOD_GETURL = RPC_METHOD_BASE "getUrl";
  static constexpr auto RPC_METHOD_DEEPLINK = RPC_METHOD_BASE "deepLink";

  std::shared_ptr<IAbstractRpcServer> wsRpcServer;
};

#endif
